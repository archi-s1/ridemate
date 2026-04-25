#pragma once

#include <vector>
#include <stdexcept>

template <typename T>
class MinHeap {
private:
    std::vector<T> heap;  // heap array (0-indexed)

    // Move element at index up to restore heap property
    void heapifyUp(int index) {
        while (index > 0) {
            int parent = (index - 1) / 2;

            if (heap[index] < heap[parent]) {
                std::swap(heap[index], heap[parent]);
                index = parent;
            } else {
                break;
            }
        }
    }

    // Move element at index down to restore heap property
    void heapifyDown(int index) {
        int n = heap.size();

        while (true) {
            int left = index * 2 + 1;
            int right = index * 2 + 2;
            int smallest = index;

            if (left < n && heap[left] < heap[smallest]) {
                smallest = left;
            }
            if (right < n && heap[right] < heap[smallest]) {
                smallest = right;
            }

            if (smallest != index) {
                std::swap(heap[index], heap[smallest]);
                index = smallest;
            } else {
                break;
            }
        }
    }

public:
    // --- Constructor ---
    MinHeap() = default;

    // --- Check if heap is empty ---
    bool empty() const noexcept {
        return heap.empty();
    }

    // --- Get size ---
    int size() const noexcept {
        return heap.size();
    }

    // --- Insert element into heap ---
    void push(const T& value) {
        heap.push_back(value);
        heapifyUp(heap.size() - 1);
    }

    // --- Remove smallest element (root) ---
    T pop() {
        if (empty()) {
            throw std::runtime_error("MinHeap is empty! Cannot pop.");
        }

        T minValue = heap[0];
        heap[0] = heap.back();
        heap.pop_back();

        if (!heap.empty()) {
            heapifyDown(0);
        }

        return minValue;
    }

    // --- Get smallest element without removing ---
    const T& top() const {
        if (empty()) {
            throw std::runtime_error("MinHeap is empty! Cannot access top.");
        }
        return heap[0];
    }

    // --- Clear heap ---
    void clear() noexcept {
        heap.clear();
    }
};
