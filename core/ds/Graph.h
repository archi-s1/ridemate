#pragma once

#include <vector>
#include <list>
#include <queue>
#include <limits>
#include <utility>
#include <iostream>
#include <functional>
#include <algorithm>

class Graph {
private:
    int nodes_;
    std::vector<std::list<std::pair<int, int>>> adj_;

public:
    // -----------------------------------------------------------------------
    //  Construction
    // -----------------------------------------------------------------------

    Graph(int totalNodes = 20)
        : nodes_(totalNodes), adj_(totalNodes) {}

    // -----------------------------------------------------------------------
    //  Edge management
    // -----------------------------------------------------------------------

    void addEdge(int u, int v, int weight) {
        if (u < 0 || v < 0 || u >= nodes_ || v >= nodes_) return;
        adj_[u].push_back({v, weight});
        adj_[v].push_back({u, weight});
    }

    // Remove an undirected edge between u and v.
    // Returns true if the edge existed and was removed.
    bool removeEdge(int u, int v) {
        if (u < 0 || v < 0 || u >= nodes_ || v >= nodes_) return false;

        bool found = false;

        auto& lu = adj_[u];
        for (auto it = lu.begin(); it != lu.end(); ++it) {
            if (it->first == v) { lu.erase(it); found = true; break; }
        }

        auto& lv = adj_[v];
        for (auto it = lv.begin(); it != lv.end(); ++it) {
            if (it->first == u) { lv.erase(it); break; }
        }

        return found;
    }

    // Dynamically expand the graph to support more nodes.
    // Safe to call even if newSize <= current size (no-op).
    void expandNodes(int newSize) {
        if (newSize <= nodes_) return;
        adj_.resize(newSize);
        nodes_ = newSize;
    }

    // -----------------------------------------------------------------------
    //  Queries
    // -----------------------------------------------------------------------

    const std::list<std::pair<int, int>>& getNeighbors(int u) const {
        return adj_[u];
    }

    int getNodeCount() const noexcept { return nodes_; }

    // Direct edge weight between adjacent u and v (-1 if not adjacent).
    int getDistance(int u, int v) const {
        if (u < 0 || v < 0 || u >= nodes_ || v >= nodes_) return -1;
        for (const auto& p : adj_[u]) {
            if (p.first == v) return p.second;
        }
        return -1;
    }

    // Check whether all edges in the graph carry the same weight.
    // Used by PathFinder to decide BFS vs Dijkstra.
    bool isUniformWeight() const {
        int baseline = -1;
        for (int u = 0; u < nodes_; ++u) {
            for (const auto& e : adj_[u]) {
                if (baseline == -1) {
                    baseline = e.second;
                } else if (e.second != baseline) {
                    return false;
                }
            }
        }
        return true; // empty graph also counts as uniform
    }

    // -----------------------------------------------------------------------
    //  BFS — shortest path by hop count (ignores weights).
    //  Returns a distance vector where dist[i] = hops from src to i,
    //  or INT_MAX if unreachable.
    // -----------------------------------------------------------------------
    std::vector<int> bfs(int src) const {
        std::vector<int> dist(nodes_, std::numeric_limits<int>::max());
        if (src < 0 || src >= nodes_) return dist;

        std::queue<int> q;
        dist[src] = 0;
        q.push(src);

        while (!q.empty()) {
            int u = q.front(); q.pop();
            for (const auto& nbr : adj_[u]) {
                int v = nbr.first;
                if (dist[v] == std::numeric_limits<int>::max()) {
                    dist[v] = dist[u] + 1;
                    q.push(v);
                }
            }
        }
        return dist;
    }

    // -----------------------------------------------------------------------
    //  Dijkstra — shortest weighted path from src.
    //  Returns a distance vector where dist[i] = min cost from src to i,
    //  or INT_MAX if unreachable.
    // -----------------------------------------------------------------------
    std::vector<int> dijkstra(int src) const {
        std::vector<int> dist(nodes_, std::numeric_limits<int>::max());
        if (src < 0 || src >= nodes_) return dist;

        using pii = std::pair<int, int>; // (cost, node)
        std::priority_queue<pii, std::vector<pii>, std::greater<pii>> pq;

        dist[src] = 0;
        pq.push({0, src});

        while (!pq.empty()) {
            auto [d, u] = pq.top(); pq.pop();

            if (d != dist[u]) continue; // stale entry

            for (const auto& [v, wt] : adj_[u]) {
                if (dist[u] + wt < dist[v]) {
                    dist[v] = dist[u] + wt;
                    pq.push({dist[v], v});
                }
            }
        }
        return dist;
    }

    // Convenience: weighted shortest distance between two nodes.
    int shortestDistance(int src, int dest) const {
        if (src < 0 || dest < 0 || src >= nodes_ || dest >= nodes_)
            return std::numeric_limits<int>::max();
        return dijkstra(src)[dest];
    }

    // -----------------------------------------------------------------------
    //  Debug
    // -----------------------------------------------------------------------

    void printGraph() const {
        for (int i = 0; i < nodes_; i++) {
            std::cout << "Node " << i << ": ";
            for (const auto& e : adj_[i]) {
                std::cout << "(" << e.first << ", w=" << e.second << ") ";
            }
            std::cout << "\n";
        }
    }
};
