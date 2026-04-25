'use strict';

// ── Fixed pixel positions for 20 nodes on the SVG canvas ─────────────────
const NODE_POS = [
  [80,  55],  // 0  MG Road
  [200, 45],  // 1  Brigade Road
  [340, 65],  // 2  Electronic City
  [460, 50],  // 3  Banashankari
  [590, 75],  // 4  Whitefield
  [110, 155], // 5  Indiranagar
  [255, 148], // 6  Ghaziabad
  [400, 142], // 7  JP Nagar
  [530, 152], // 8  Hebbal
  [665, 138], // 9  Yelahanka
  [75,  258], // 10 Koramangala
  [205, 248], // 11 Malleshwaram
  [350, 252], // 12 Rajajinagar
  [490, 258], // 13 Basavanagudi
  [625, 252], // 14 BTM Layout
  [135, 348], // 15 Silk Board
  [275, 342], // 16 Marathahalli
  [420, 348], // 17 Domlur
  [555, 342], // 18 Jayanagar
  [685, 352], // 19 Bannerghatta
];

const SVG_W = 760, SVG_H = 415;
const INF = 1e9;

// ── Algorithm metadata ────────────────────────────────────────────────────
const ALGO_META = {
  dijkstra: {
    label:       'Dijkstra',
    time:        'O((V + E) log V)',
    space:       'O(V)',
    works_on:    'Weighted graphs (non-negative weights)',
    description: 'Greedy shortest-path. Uses a min-heap to always expand the cheapest unvisited node. Gives exact shortest weighted distance.',
    color:       '#d97706',   // amber
  },
  bfs: {
    label:       'BFS',
    time:        'O(V + E)',
    space:       'O(V)',
    works_on:    'Unweighted graphs (treats all edges as weight = 1)',
    description: 'Breadth-first search. Explores layer by layer — guarantees fewest hops, ignores edge weights. Fastest on uniform graphs.',
    color:       '#3b82f6',   // blue
  },
  floyd: {
    label:       'Floyd-Warshall',
    time:        'O(V³)',
    space:       'O(V²)',
    works_on:    'All-pairs shortest paths (any weights)',
    description: 'Dynamic programming over all node triples. Computes every pair at once. Expensive but lets you query any pair in O(1) after setup.',
    color:       '#22c55e',   // green
  },
};

// ============================================================================
//  PathFinder  — runs all three algorithms in the browser on the graph data
// ============================================================================
class PathFinder {
  constructor(graphData) {
    this.N = graphData.length;
    // Build adjacency list:  adj[u] = [{v, w}, ...]
    this.adj = Array.from({ length: this.N }, () => []);
    for (const node of graphData) {
      for (const { to, weight } of node.edges) {
        this.adj[node.node].push({ v: to, w: weight });
      }
    }
    this._floydDist = null;
    this._floydNext = null;
  }

  // ── Dijkstra ─────────────────────────────────────────────────────────────
  // Returns { dist, prev } from src.
  // prev[v] = u means the shortest path to v comes from u.
  dijkstra(src) {
    const dist = new Array(this.N).fill(INF);
    const prev = new Array(this.N).fill(-1);
    dist[src] = 0;

    // Simple binary min-heap via sorted array — good enough for N=20
    const pq = [[0, src]];   // [cost, node]

    while (pq.length) {
      pq.sort((a, b) => a[0] - b[0]);
      const [d, u] = pq.shift();
      if (d > dist[u]) continue;

      for (const { v, w } of this.adj[u]) {
        if (dist[u] + w < dist[v]) {
          dist[v] = dist[u] + w;
          prev[v] = u;
          pq.push([dist[v], v]);
        }
      }
    }
    return { dist, prev };
  }

  // ── BFS ──────────────────────────────────────────────────────────────────
  // Ignores weights — finds fewest-hop path from src.
  bfs(src) {
    const dist = new Array(this.N).fill(INF);
    const prev = new Array(this.N).fill(-1);
    dist[src] = 0;
    const queue = [src];
    let head = 0;

    while (head < queue.length) {
      const u = queue[head++];
      for (const { v } of this.adj[u]) {
        if (dist[v] === INF) {
          dist[v] = dist[u] + 1;
          prev[v] = u;
          queue.push(v);
        }
      }
    }
    return { dist, prev };
  }

  // ── Floyd-Warshall ───────────────────────────────────────────────────────
  // Computes all-pairs shortest paths once, caches result.
  // Returns { dist2d, next2d } — 2D arrays indexed [src][dst].
  floydWarshall() {
    if (this._floydDist) return { dist2d: this._floydDist, next2d: this._floydNext };

    const N = this.N;
    // Initialise
    const dist = Array.from({ length: N }, (_, i) =>
      Array.from({ length: N }, (_, j) => (i === j ? 0 : INF))
    );
    const next = Array.from({ length: N }, () => new Array(N).fill(-1));

    for (let u = 0; u < N; u++) {
      for (const { v, w } of this.adj[u]) {
        if (w < dist[u][v]) {
          dist[u][v] = w;
          dist[v][u] = w;          // undirected
          next[u][v] = v;
          next[v][u] = u;
        }
      }
    }
    for (let i = 0; i < N; i++) next[i][i] = i;

    // Relax
    for (let k = 0; k < N; k++) {
      for (let i = 0; i < N; i++) {
        for (let j = 0; j < N; j++) {
          if (dist[i][k] + dist[k][j] < dist[i][j]) {
            dist[i][j] = dist[i][k] + dist[k][j];
            next[i][j] = next[i][k];
          }
        }
      }
    }

    this._floydDist = dist;
    this._floydNext = next;
    return { dist2d: dist, next2d: next };
  }

  // ── Path reconstruction ───────────────────────────────────────────────────
  // From a prev[] array (Dijkstra / BFS), rebuild the path src→dst.
  static reconstructPath(prev, src, dst) {
    const path = [];
    let cur = dst;
    while (cur !== -1) {
      path.unshift(cur);
      if (cur === src) break;
      cur = prev[cur];
      if (path.length > 50) return [];   // cycle guard
    }
    return path[0] === src ? path : [];
  }

  // From Floyd next[][] matrix, rebuild the path src→dst.
  static floydPath(next, src, dst) {
    if (next[src][dst] === -1) return [];
    const path = [src];
    let cur = src;
    while (cur !== dst) {
      cur = next[cur][dst];
      path.push(cur);
      if (path.length > 50) return [];
    }
    return path;
  }

  // ── Single entry-point for the UI ─────────────────────────────────────────
  // algo: 'dijkstra' | 'bfs' | 'floyd'
  // Returns { distance, path, hops }
  compute(algo, src, dst) {
    if (src === dst) return { distance: 0, path: [src], hops: 0 };

    if (algo === 'dijkstra') {
      const { dist, prev } = this.dijkstra(src);
      const path = PathFinder.reconstructPath(prev, src, dst);
      return { distance: dist[dst], path, hops: path.length - 1 };
    }

    if (algo === 'bfs') {
      const { dist, prev } = this.bfs(src);
      const path = PathFinder.reconstructPath(prev, src, dst);
      // For BFS, "distance" = hop count; real weighted cost computed separately
      const weightedCost = this._pathWeight(path);
      return { distance: dist[dst], weightedCost, path, hops: dist[dst] };
    }

    if (algo === 'floyd') {
      const { dist2d, next2d } = this.floydWarshall();
      const path = PathFinder.floydPath(next2d, src, dst);
      return { distance: dist2d[src][dst], path, hops: path.length - 1 };
    }

    return { distance: INF, path: [], hops: 0 };
  }

  // Actual weighted cost of a path array
  _pathWeight(path) {
    let total = 0;
    for (let i = 0; i < path.length - 1; i++) {
      const u = path[i], v = path[i + 1];
      const edge = this.adj[u].find(e => e.v === v);
      total += edge ? edge.w : INF;
    }
    return total;
  }
}

// ============================================================================
//  CityMap — SVG renderer
// ============================================================================
class CityMap {
  constructor(containerId) {
    this.container  = document.getElementById(containerId);
    this.graphData  = null;
    this.highlighted = new Set();   // edge keys "u-v"
    this.highlightColor = '#d97706';
    this.selectedNode = null;
    this.pathFinder = null;
    this.onNodeClick = null;
  }

  render(graphData) {
    this.graphData  = graphData;
    this.pathFinder = new PathFinder(graphData);
    const ns = 'http://www.w3.org/2000/svg';

    const svg = document.createElementNS(ns, 'svg');
    svg.setAttribute('viewBox', `0 0 ${SVG_W} ${SVG_H}`);
    svg.setAttribute('xmlns', ns);
    svg.style.width = '100%';

    // defs
    const defs = document.createElementNS(ns, 'defs');
    defs.innerHTML = `
      <filter id="glow-amber"><feGaussianBlur stdDeviation="3" result="b"/>
        <feMerge><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/></feMerge></filter>
      <filter id="glow-blue"><feGaussianBlur stdDeviation="3" result="b"/>
        <feMerge><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/></feMerge></filter>
      <filter id="glow-green"><feGaussianBlur stdDeviation="3" result="b"/>
        <feMerge><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/></feMerge></filter>
    `;
    svg.appendChild(defs);

    const glowId = {
      '#d97706': 'glow-amber',
      '#3b82f6': 'glow-blue',
      '#22c55e': 'glow-green',
    }[this.highlightColor] || 'glow-amber';

    // ── edges ──
    const edgeGroup = document.createElementNS(ns, 'g');
    const rendered  = new Set();

    for (const nodeInfo of graphData) {
      const u = nodeInfo.node;
      const [x1, y1] = NODE_POS[u] || [0, 0];
      for (const { to: v, weight: w } of nodeInfo.edges) {
        const key = [Math.min(u,v), Math.max(u,v)].join('-');
        if (rendered.has(key)) continue;
        rendered.add(key);

        const [x2, y2] = NODE_POS[v] || [0, 0];
        const hi = this.highlighted.has(key);

        const line = document.createElementNS(ns, 'line');
        line.setAttribute('x1', x1); line.setAttribute('y1', y1);
        line.setAttribute('x2', x2); line.setAttribute('y2', y2);
        line.setAttribute('stroke', hi ? this.highlightColor : '#262626');
        line.setAttribute('stroke-width', hi ? '3' : '1.5');
        if (hi) line.setAttribute('filter', `url(#${glowId})`);
        edgeGroup.appendChild(line);

        // weight label
        const mx = (x1+x2)/2, my = (y1+y2)/2;
        const wt = document.createElementNS(ns, 'text');
        wt.setAttribute('x', mx); wt.setAttribute('y', my - 4);
        wt.setAttribute('text-anchor', 'middle');
        wt.setAttribute('font-family', 'Space Mono, monospace');
        wt.setAttribute('font-size', '8.5');
        wt.setAttribute('fill', hi ? this.highlightColor : '#363636');
        wt.textContent = w;
        edgeGroup.appendChild(wt);
      }
    }
    svg.appendChild(edgeGroup);

    // ── nodes ──
    const nodeGroup = document.createElementNS(ns, 'g');
    for (let i = 0; i < NODE_POS.length; i++) {
      const [x, y] = NODE_POS[i];
      const isSel  = this.selectedNode === i;

      const g = document.createElementNS(ns, 'g');
      g.style.cursor = 'pointer';
      g.addEventListener('click', () => this._onNodeClick(i));

      if (isSel) {
        const ring = document.createElementNS(ns, 'circle');
        ring.setAttribute('cx', x); ring.setAttribute('cy', y); ring.setAttribute('r', '13');
        ring.setAttribute('fill', 'none');
        ring.setAttribute('stroke', this.highlightColor);
        ring.setAttribute('stroke-width', '1.5'); ring.setAttribute('opacity', '.7');
        g.appendChild(ring);
      }

      const circle = document.createElementNS(ns, 'circle');
      circle.setAttribute('cx', x); circle.setAttribute('cy', y); circle.setAttribute('r', '8');
      circle.setAttribute('fill', isSel ? this.highlightColor : '#181818');
      circle.setAttribute('stroke', isSel ? this.highlightColor : '#3a3a3a');
      circle.setAttribute('stroke-width', '1.5');
      g.appendChild(circle);

      const lbl = document.createElementNS(ns, 'text');
      lbl.setAttribute('x', x); lbl.setAttribute('y', y + 1);
      lbl.setAttribute('text-anchor', 'middle');
      lbl.setAttribute('dominant-baseline', 'middle');
      lbl.setAttribute('font-family', 'Space Mono, monospace');
      lbl.setAttribute('font-size', '7'); lbl.setAttribute('font-weight', '700');
      lbl.setAttribute('fill', isSel ? '#000' : '#777');
      lbl.textContent = i;
      g.appendChild(lbl);

      const nm = document.createElementNS(ns, 'text');
      nm.setAttribute('x', x); nm.setAttribute('y', y + 20);
      nm.setAttribute('text-anchor', 'middle');
      nm.setAttribute('font-family', 'Space Mono, monospace');
      nm.setAttribute('font-size', '7');
      nm.setAttribute('fill', isSel ? this.highlightColor : '#484848');
      nm.textContent = locationName(i).split(' ')[0];
      g.appendChild(nm);

      nodeGroup.appendChild(g);
    }
    svg.appendChild(nodeGroup);

    this.container.innerHTML = '';
    this.container.appendChild(svg);
  }

  highlightPath(path, color) {
    this.highlighted.clear();
    this.highlightColor = color || '#d97706';
    for (let i = 0; i < path.length - 1; i++) {
      const key = [Math.min(path[i], path[i+1]), Math.max(path[i], path[i+1])].join('-');
      this.highlighted.add(key);
    }
    if (this.graphData) this.render(this.graphData);
  }

  clearHighlight() {
    this.highlighted.clear();
    this.highlightColor = '#d97706';
    if (this.graphData) this.render(this.graphData);
  }

  _onNodeClick(i) {
    this.selectedNode = i;
    if (this.graphData) this.render(this.graphData);
    if (this.onNodeClick) this.onNodeClick(i);
  }
}
