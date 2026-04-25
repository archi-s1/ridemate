#pragma once

#include <stdexcept>

template <typename T>
class CircularQueue {
private:
    T* arr;              // array to store values
    int capacity;        // max size
    int front_;          // front index
    int rear_;           // rear index
    int count;           // current size

public:
    // --- Constructor ---
    CircularQueue(int cap = 100) 
        : capacity(cap), front_(0), rear_(-1), count(0) 
    {
        arr = new T[capacity];
    }

    // --- Destructor ---
    ~CircularQueue() {
        delete[] arr;
    }

    // --- Insert element (enqueue) ---
    void enqueue(const T& value) {
        if (isFull()) {
            throw std::runtime_error("CircularQueue is full! Cannot enqueue.");
        }
        rear_ = (rear_ + 1) % capacity;
        arr[rear_] = value;
        count++;
    }

    // --- Remove and return front element (dequeue) ---
    T dequeue() {
        if (isEmpty()) {
            throw std::runtime_error("CircularQueue is empty! Cannot dequeue.");
        }
        T value = arr[front_];
        front_ = (front_ + 1) % capacity;
        count--;
        return value;
    }

    // --- Access front element ---
    T& front() {
        if (isEmpty()) {
            throw std::runtime_error("CircularQueue is empty! Cannot access front.");
        }
        return arr[front_];
    }

    const T& front() const {
        if (isEmpty()) {
            throw std::runtime_error("CircularQueue is empty! Cannot access front.");
        }
        return arr[front_];
    }

    // --- Check if queue is empty ---
    bool isEmpty() const noexcept {
        return count == 0;
    }

    // --- Check if queue is full ---
    bool isFull() const noexcept {
        return count == capacity;
    }

    // --- Return current size ---
    int size() const noexcept {
        return count;
    }

    // --- Clear queue ---
    void clear() noexcept {
        front_ = 0;
        rear_ = -1;
        count = 0;
    }
};
