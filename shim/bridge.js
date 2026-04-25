'use strict';

/**
 * bridge.js
 *
 * Spawns the C++ binary once at startup and keeps it alive for the entire
 * server lifetime.  All communication is newline-delimited JSON over the
 * process's stdin/stdout pipe.
 *
 * Public API:
 *   bridge.send(cmdObject)  → Promise<responseObject>
 *   bridge.isReady()        → boolean
 *   bridge.shutdown()       → void  (closes stdin, lets C++ flush and exit)
 *
 * How the queue works:
 *   Commands may arrive concurrently from multiple HTTP requests.  Because
 *   the C++ process handles one command at a time (one JSON in → one JSON
 *   out), we serialise them through a FIFO queue.  Each call to send()
 *   appends a {resolve, reject} entry; the reader always resolves the head
 *   of the queue when a response line arrives.
 */

const { spawn }    = require('child_process');
const path         = require('path');
const EventEmitter = require('events');

// ---------------------------------------------------------------------------
//  Config — tweak these if your directory layout changes
// ---------------------------------------------------------------------------
const CPP_BINARY   = path.resolve(__dirname, '..', 'ridemate');   // compiled binary
const DATA_DIR     = path.resolve(__dirname, '..', 'data');        // persistent JSON files
const STARTUP_WAIT = 3000;   // ms to wait for "[RideMate] Ready" on stderr
const CMD_TIMEOUT  = 10000;  // ms before a command times out

// ---------------------------------------------------------------------------
//  Bridge singleton
// ---------------------------------------------------------------------------
class Bridge extends EventEmitter {
    constructor() {
        super();
        this._proc     = null;
        this._ready    = false;
        this._queue    = [];      // [{resolve, reject, timer}]
        this._lineBuf  = '';      // partial line accumulator for stdout
    }

    // ------------------------------------------------------------------
    //  start()
    //  Spawns the C++ process and waits until it prints "Ready" on stderr.
    //  Returns a Promise that resolves when the bridge is usable.
    // ------------------------------------------------------------------
    start() {
        return new Promise((resolve, reject) => {
            console.log(`[Bridge] Spawning: ${CPP_BINARY}`);

            this._proc = spawn(CPP_BINARY, [], {
                cwd:   path.resolve(__dirname, '..'),   // working dir = project root (data/ lives here)
                stdio: ['pipe', 'pipe', 'pipe'],
                env:   { ...process.env }
            });

            // ---- stdout → JSON response lines --------------------------
            this._proc.stdout.setEncoding('utf8');
            this._proc.stdout.on('data', (chunk) => {
                this._lineBuf += chunk;
                let idx;
                while ((idx = this._lineBuf.indexOf('\n')) !== -1) {
                    const line = this._lineBuf.slice(0, idx).trim();
                    this._lineBuf = this._lineBuf.slice(idx + 1);
                    if (line) this._handleResponse(line);
                }
            });

            // ---- stderr → diagnostic logs (also watch for Ready signal) -
            this._proc.stderr.setEncoding('utf8');
            let stderrBuf = '';
            this._proc.stderr.on('data', (chunk) => {
                stderrBuf += chunk;
                // Print to our console so the developer can see startup logs
                process.stderr.write(`[C++] ${chunk}`);

                if (!this._ready && stderrBuf.includes('Ready')) {
                    this._ready = true;
                    resolve();
                }
            });

            // ---- process exit ------------------------------------------
            this._proc.on('close', (code) => {
                console.error(`[Bridge] C++ process exited with code ${code}`);
                this._ready = false;
                // Reject all pending commands
                while (this._queue.length) {
                    const { reject: rej, timer } = this._queue.shift();
                    clearTimeout(timer);
                    rej(new Error('C++ process exited unexpectedly.'));
                }
                this.emit('exit', code);
            });

            this._proc.on('error', (err) => {
                console.error('[Bridge] Failed to spawn C++ process:', err.message);
                reject(err);
            });

            // Fallback — if we never see "Ready" in time, resolve anyway
            // so the server still starts (commands will just queue until ready)
            setTimeout(() => {
                if (!this._ready) {
                    console.warn('[Bridge] Did not see Ready signal — starting anyway.');
                    this._ready = true;
                    resolve();
                }
            }, STARTUP_WAIT);
        });
    }

    // ------------------------------------------------------------------
    //  send(cmdObject) → Promise<responseObject>
    //  Serialises cmdObject to JSON, writes it to the C++ stdin,
    //  and returns a Promise that resolves with the parsed response.
    // ------------------------------------------------------------------
    send(cmdObject) {
        return new Promise((resolve, reject) => {
            if (!this._proc || !this._ready) {
                return reject(new Error('C++ bridge is not ready.'));
            }

            let timer = setTimeout(() => {
                // Remove this entry from the queue
                this._queue = this._queue.filter(e => e.timer !== timer);
                reject(new Error(`Command timed out: ${cmdObject.cmd}`));
            }, CMD_TIMEOUT);

            this._queue.push({ resolve, reject, timer });

            const line = JSON.stringify(cmdObject) + '\n';
            this._proc.stdin.write(line, 'utf8');
        });
    }

    // ------------------------------------------------------------------
    //  _handleResponse(line)
    //  Called for every complete JSON line from C++ stdout.
    //  Resolves the oldest pending queue entry.
    // ------------------------------------------------------------------
    _handleResponse(line) {
        const entry = this._queue.shift();
        if (!entry) {
            console.warn('[Bridge] Response with no pending command:', line);
            return;
        }

        clearTimeout(entry.timer);

        let parsed;
        try {
            parsed = JSON.parse(line);
        } catch (e) {
            entry.reject(new Error(`C++ returned invalid JSON: ${line}`));
            return;
        }

        entry.resolve(parsed);
    }

    // ------------------------------------------------------------------
    //  isReady()
    // ------------------------------------------------------------------
    isReady() {
        return this._ready;
    }

    // ------------------------------------------------------------------
    //  shutdown()
    //  Closes the C++ stdin, letting it flush state and exit cleanly.
    // ------------------------------------------------------------------
    shutdown() {
        if (this._proc) {
            console.log('[Bridge] Shutting down C++ process...');
            this._proc.stdin.end();
        }
    }
}

// Export a single shared instance
module.exports = new Bridge();
