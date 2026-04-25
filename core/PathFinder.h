#pragma once

#include <vector>
#include <string>
#include <limits>
#include <queue>
#include <iostream>
#include "ds/Graph.h"

// ---------------------------------------------------------------------------
//  PathFinder
//
//  Sits between RideService and Graph.  Decides at call-time whether to use
//  BFS (uniform-weight or small graph) or Dijkstra (weighted graph), so the
//  rest of the system never has to care which algorithm ran.
//
//  Decision logic:
//    1. If graph has <= SMALL_GRAPH_THRESHOLD nodes  → BFS  (fast, exact for hops)
//    2. Else if all edge weights are equal           → BFS  (BFS is optimal here)
//    3. Otherwise                                    → Dijkstra
//
//  The chosen algorithm is recorded and exposed for the UI/logs so the
//  "adaptive" nature is visible.
// ---------------------------------------------------------------------------

class PathFinder {
public:
    // Threshold below which we always prefer BFS regardless of weights.
    static constexpr int SMALL_GRAPH_THRESHOLD = 10;

    enum class Algorithm { BFS, DIJKSTRA };

    struct Result {
        int          distance;   // INT_MAX means unreachable
        Algorithm    algo;       // which algorithm was used
        std::string  algoName;   // "BFS" or "Dijkstra" — for display
        bool         reachable;
    };

    // ------------------------------------------------------------------
    //  shortestDistance
    //  Returns distance + which algorithm was used.
    // ------------------------------------------------------------------
    static Result shortestDistance(const Graph& g, int src, int dst) {
        Algorithm chosen = pickAlgorithm(g);

        std::vector<int> dist;
        if (chosen == Algorithm::BFS) {
            dist = g.bfs(src);
        } else {
            dist = g.dijkstra(src);
        }

        int d = dist[dst];
        bool ok = (d != std::numeric_limits<int>::max());

        return Result{
            ok ? d : std::numeric_limits<int>::max(),
            chosen,
            chosen == Algorithm::BFS ? "BFS" : "Dijkstra",
            ok
        };
    }

    // ------------------------------------------------------------------
    //  fullDistanceVector
    //  Returns all distances from src + which algorithm ran.
    //  Used by RouteCache to pre-warm in one pass per source node.
    // ------------------------------------------------------------------
    static std::pair<std::vector<int>, Algorithm>
    fullDistanceVector(const Graph& g, int src) {
        Algorithm chosen = pickAlgorithm(g);
        std::vector<int> dist;

        if (chosen == Algorithm::BFS) {
            dist = g.bfs(src);
        } else {
            dist = g.dijkstra(src);
        }
        return {dist, chosen};
    }

    // ------------------------------------------------------------------
    //  Which algorithm would be chosen for this graph right now?
    //  Exposed so RouteCache can log/display on startup.
    // ------------------------------------------------------------------
    static Algorithm pickAlgorithm(const Graph& g) {
        if (g.getNodeCount() <= SMALL_GRAPH_THRESHOLD) return Algorithm::BFS;
        if (g.isUniformWeight())                       return Algorithm::BFS;
        return Algorithm::DIJKSTRA;
    }

    static std::string algorithmName(Algorithm a) {
        return a == Algorithm::BFS ? "BFS" : "Dijkstra";
    }
};
