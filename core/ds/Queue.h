#pragma once

#include <stdexcept>

template <typename T>
class Queue {
private:
    struct Node {
        T data;
        Node* next;
        Node(const T& d) : data(d), next(nullptr) {}
    };

    Node* front_;   // front of queue
    Node* rear_;    // rear of queue
    int size_;      // number of elements

public:
    // --- Constructor ---
    Queue() : front_(nullptr), rear_(nullptr), size_(0) {}

    // --- Destructor (free all nodes) ---
    ~Queue() {
        while (!empty()) {
            dequeue();
        }
    }

    // --- Enqueue (push to rear) ---
    void enqueue(const T& value) {
        Node* newNode = new Node(value);

        if (empty()) {
            front_ = rear_ = newNode;
        } else {
            rear_->next = newNode;
            rear_ = newNode;
        }
        size_++;
    }

    // --- Dequeue (remove from front) ---
    T dequeue() {
        if (empty()) {
            throw std::runtime_error("Queue is empty! Cannot dequeue.");
        }

        Node* temp = front_;
        T value = temp->data;

        front_ = front_->next;
        delete temp;

        size_--;
        if (size_ == 0) rear_ = nullptr;

        return value;
    }

    // --- Get front element ---
    T& front() {
        if (empty()) {
            throw std::runtime_error("Queue is empty! Cannot access front.");
        }
        return front_->data;
    }

    const T& front() const {
        if (empty()) {
            throw std::runtime_error("Queue is empty! Cannot access front.");
        }
        return front_->data;
    }

    // --- Check if queue is empty ---
    bool empty() const noexcept {
        return size_ == 0;
    }

    // --- Get size of queue ---
    int size() const noexcept {
        return size_;
    }
};
