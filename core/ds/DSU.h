#pragma once

#include <vector>
#include <stdexcept>

class DSU {
private:
    std::vector<int> parent;
    std::vector<int> rank;
    int n;

public:
    DSU(int size = 50) : n(size) {
        parent.resize(size);
        rank.resize(size, 0);

        for (int i = 0; i < size; i++) {
            parent[i] = i; // each node is its own parent
        }
    }

    int find(int x) {
        if (x < 0 || x >= n)
            throw std::runtime_error("DSU: index out of bounds!");

        if (parent[x] != x)
            parent[x] = find(parent[x]); // path compression

        return parent[x];
    }

    void unite(int x, int y) {
        int rootX = find(x);
        int rootY = find(y);

        if (rootX == rootY) return;

        if (rank[rootX] < rank[rootY]) {
            parent[rootX] = rootY;
        }
        else if (rank[rootX] > rank[rootY]) {
            parent[rootY] = rootX;
        }
        else {
            parent[rootY] = rootX;
            rank[rootX]++;
        }
    }

    bool connected(int x, int y) {
        return find(x) == find(y);
    }

    void reset() {
        for (int i = 0; i < n; i++) {
            parent[i] = i;
            rank[i] = 0;
        }
    }
};
