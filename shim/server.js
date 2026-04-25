'use strict';

/**
 * server.js
 *
 * Thin Express HTTP shim.  Every route does exactly one thing:
 *   1. Pull params from req.body / req.params / req.query
 *   2. Forward them as a JSON command to the C++ bridge
 *   3. Return the response JSON to the client
 *
 * Auth tokens are passed by the frontend in the Authorization header:
 *   Authorization: Bearer <token>
 * The middleware extracts it and injects it into every command so the
 * C++ CommandRouter can verify it.
 *
 * Routes:
 *   POST   /api/auth/register
 *   POST   /api/auth/login
 *   POST   /api/auth/logout
 *
 *   GET    /api/drivers
 *   GET    /api/drivers/available
 *   POST   /api/drivers
 *   DELETE /api/drivers/:id
 *   PATCH  /api/drivers/:id/toggle
 *   GET    /api/drivers/:id/history
 *
 *   GET    /api/passengers
 *   POST   /api/passengers
 *   GET    /api/passengers/:id/history
 *
 *   POST   /api/rides/preview
 *   POST   /api/rides/request
 *   POST   /api/rides/assign
 *   POST   /api/rides/:id/cancel
 *   POST   /api/rides/:id/complete
 *   GET    /api/rides/ongoing
 *   GET    /api/rides/recent
 *
 *   GET    /api/map
 *   GET    /api/map/search?prefix=...
 *   POST   /api/map/edge
 *   DELETE /api/map/edge
 *
 *   GET    /api/meta/algorithm
 *   POST   /api/meta/prewarm
 *   GET    /api/meta/health
 */

const express = require('express');
const cors    = require('cors');
const bridge  = require('./bridge');

const PORT = process.env.PORT || 3001;
const app  = express();

// ---------------------------------------------------------------------------
//  Middleware
// ---------------------------------------------------------------------------
app.use(cors());
app.use(express.json());

// Extract Bearer token from Authorization header and attach to req.token
app.use((req, _res, next) => {
    const header = req.headers['authorization'] || '';
    const match  = header.match(/^Bearer\s+(.+)$/i);
    req.token = match ? match[1] : null;
    next();
});

// ---------------------------------------------------------------------------
//  Helper — forward a command to C++ and send the result back
// ---------------------------------------------------------------------------
async function fw(res, cmdObject) {
    try {
        const result = await bridge.send(cmdObject);
        // Use HTTP 401 when C++ says the token is invalid
        const status = (!result.ok && result.msg &&
                        result.msg.toLowerCase().includes('token'))
                       ? 401 : 200;
        res.status(status).json(result);
    } catch (err) {
        console.error('[Server] Bridge error:', err.message);
        res.status(500).json({ ok: false, msg: err.message });
    }
}

// ---------------------------------------------------------------------------
//  AUTH
// ---------------------------------------------------------------------------
app.post('/api/auth/register', (req, res) => {
    const { name, email, password, role, location, vehicleType } = req.body;
    fw(res, { cmd: 'register', name, email, password, role,
              location: location ?? 0, vehicleType: vehicleType ?? 'mini' });
});

app.post('/api/auth/login', (req, res) => {
    const { email, password } = req.body;
    fw(res, { cmd: 'login', email, password });
});

app.post('/api/auth/logout', (req, res) => {
    fw(res, { cmd: 'logout', token: req.token });
});

// ---------------------------------------------------------------------------
//  DRIVERS
// ---------------------------------------------------------------------------
app.get('/api/drivers', (req, res) => {
    fw(res, { cmd: 'allDrivers', token: req.token });
});

app.get('/api/drivers/available', (req, res) => {
    fw(res, { cmd: 'availDrivers', token: req.token });
});

app.post('/api/drivers', (req, res) => {
    const { name, location, vehicleType } = req.body;
    fw(res, { cmd: 'addDriver', token: req.token,
              name, location: location ?? 0, vehicleType: vehicleType ?? 'mini' });
});

// Sub-routes MUST be registered before /:id so Express matches them first
app.patch('/api/drivers/:id/toggle', (req, res) => {
    fw(res, { cmd: 'toggleAvail', token: req.token,
              driverId: parseInt(req.params.id) });
});

app.get('/api/drivers/:id/history', (req, res) => {
    fw(res, { cmd: 'driverHistory', token: req.token,
              driverId: parseInt(req.params.id) });
});

app.delete('/api/drivers/:id', (req, res) => {
    fw(res, { cmd: 'removeDriver', token: req.token,
              driverId: parseInt(req.params.id) });
});

// ---------------------------------------------------------------------------
//  PASSENGERS
// ---------------------------------------------------------------------------
app.get('/api/passengers', (req, res) => {
    fw(res, { cmd: 'allPassengers', token: req.token });
});

app.post('/api/passengers', (req, res) => {
    const { name, location } = req.body;
    fw(res, { cmd: 'addPassenger', token: req.token,
              name, location: location ?? 0 });
});

app.get('/api/passengers/:id/history', (req, res) => {
    fw(res, { cmd: 'passengerHistory', token: req.token,
              passengerId: parseInt(req.params.id) });
});

// ---------------------------------------------------------------------------
//  RIDES
// ---------------------------------------------------------------------------
app.post('/api/rides/preview', (req, res) => {
    const { pickup, drop, rideType } = req.body;
    fw(res, { cmd: 'previewFare', token: req.token,
              pickup, drop, rideType: rideType ?? 'mini' });
});

app.post('/api/rides/request', (req, res) => {
    const { passengerId, pickup, drop, rideType } = req.body;
    fw(res, { cmd: 'requestRide', token: req.token,
              passengerId, pickup, drop, rideType: rideType ?? 'mini' });
});

app.post('/api/rides/assign', (req, res) => {
    fw(res, { cmd: 'assignRide', token: req.token });
});

app.post('/api/rides/:id/cancel', (req, res) => {
    fw(res, { cmd: 'cancelRide', token: req.token,
              rideId: parseInt(req.params.id) });
});

app.post('/api/rides/:id/complete', (req, res) => {
    const { rating } = req.body;
    fw(res, { cmd: 'completeRide', token: req.token,
              rideId: parseInt(req.params.id), rating: rating ?? 5 });
});

app.get('/api/rides/ongoing', (req, res) => {
    fw(res, { cmd: 'ongoingRides', token: req.token });
});

app.get('/api/rides/recent', (req, res) => {
    fw(res, { cmd: 'recentRides', token: req.token });
});

// ---------------------------------------------------------------------------
//  MAP
// ---------------------------------------------------------------------------
app.get('/api/map', (req, res) => {
    fw(res, { cmd: 'cityMap', token: req.token });
});

app.get('/api/map/search', (req, res) => {
    fw(res, { cmd: 'searchLocation', token: req.token,
              prefix: req.query.prefix ?? '' });
});

app.post('/api/map/edge', (req, res) => {
    const { u, v, weight } = req.body;
    fw(res, { cmd: 'addEdge', token: req.token, u, v, weight: weight ?? 1 });
});

app.delete('/api/map/edge', (req, res) => {
    const { u, v } = req.body;
    fw(res, { cmd: 'removeEdge', token: req.token, u, v });
});

// ---------------------------------------------------------------------------
//  META
// ---------------------------------------------------------------------------
app.get('/api/meta/algorithm', (req, res) => {
    fw(res, { cmd: 'algorithm', token: req.token });
});

app.post('/api/meta/prewarm', (req, res) => {
    fw(res, { cmd: 'preWarmCache', token: req.token });
});

app.get('/api/meta/health', (_req, res) => {
    res.json({ ok: true, bridge: bridge.isReady(), ts: Date.now() });
});

// ---------------------------------------------------------------------------
//  404 catch-all
// ---------------------------------------------------------------------------
app.use((_req, res) => {
    res.status(404).json({ ok: false, msg: 'Route not found.' });
});

// ---------------------------------------------------------------------------
//  Start
// ---------------------------------------------------------------------------
bridge.start()
    .then(() => {
        console.log('[Server] C++ bridge ready.');
        app.listen(PORT, () => {
            console.log(`[Server] Listening on http://localhost:${PORT}`);
        });
    })
    .catch((err) => {
        console.error('[Server] Could not start bridge:', err.message);
        process.exit(1);
    });

// ---------------------------------------------------------------------------
//  Graceful shutdown
// ---------------------------------------------------------------------------
function shutdown() {
    console.log('\n[Server] Shutting down...');
    bridge.shutdown();
    process.exit(0);
}

process.on('SIGINT',  shutdown);
process.on('SIGTERM', shutdown);
process.on('exit',    shutdown);
