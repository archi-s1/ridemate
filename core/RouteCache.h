#pragma once

#include <unordered_map>
#include <string>
#include <limits>
#include <iostream>
#include "ds/Graph.h"
#include "PathFinder.h"

// ---------------------------------------------------------------------------
//  RouteCache
//
//  Maintains a flat unordered_map<string, int> keyed by "src:dst" strings.
//  Because the city map is a fixed 20-node graph, preWarm() runs PathFinder
//  for every source node once at startup — after that every fare/distance
//  lookup is O(1) with zero algorithm overhead.
//
//  Invalidation:
//    Call invalidate() after any addEdge / removeEdge on the graph.
//    The next lookup that misses will re-run PathFinder for that source.
//    Full re-warm can be forced with preWarm().
// ---------------------------------------------------------------------------

class RouteCache {
private:
    std::unordered_map<std::string, int> cache_;
    bool warmed_ = false;

    static std::string makeKey(int src, int dst) {
        return std::to_string(src) + ":" + std::to_string(dst);
    }

public:
    // ------------------------------------------------------------------
    //  preWarm
    //  Runs PathFinder from every node, fills the entire distance matrix.
    //  Called once when RideService initialises.
    // ------------------------------------------------------------------
    void preWarm(const Graph& g) {
        cache_.clear();
        int n = g.getNodeCount();

        PathFinder::Algorithm algo = PathFinder::pickAlgorithm(g);
        std::cerr << "[RouteCache] Pre-warming " << n << "x" << n
                  << " distance matrix using "
                  << PathFinder::algorithmName(algo) << "...\n";

        for (int src = 0; src < n; ++src) {
            auto [dist, _] = PathFinder::fullDistanceVector(g, src);
            for (int dst = 0; dst < n; ++dst) {
                cache_[makeKey(src, dst)] = dist[dst];
            }
        }

        warmed_ = true;
        std::cerr << "[RouteCache] Done. " << cache_.size()
                  << " pairs cached.\n";
    }

    // ------------------------------------------------------------------
    //  get
    //  Returns cached distance or re-computes lazily on cache miss.
    //  A miss happens after invalidate() or if preWarm was never called.
    // ------------------------------------------------------------------
    int get(const Graph& g, int src, int dst) {
        std::string key = makeKey(src, dst);
        auto it = cache_.find(key);
        if (it != cache_.end()) return it->second;

        // Cache miss — compute row for this src and store it
        auto [dist, _] = PathFinder::fullDistanceVector(g, src);
        int n = g.getNodeCount();
        for (int d = 0; d < n; ++d) {
            cache_[makeKey(src, d)] = dist[d];
        }
        return cache_[key];
    }

    // ------------------------------------------------------------------
    //  invalidate
    //  Call after any structural change to the graph.
    //  Clears the whole cache; next access re-fills lazily.
    // ------------------------------------------------------------------
    void invalidate() {
        cache_.clear();
        warmed_ = false;
        std::cerr << "[RouteCache] Cache invalidated (graph topology changed).\n";
    }

    bool isWarmed()    const noexcept { return warmed_; }
    int  cachedPairs() const noexcept { return static_cast<int>(cache_.size()); }
};
