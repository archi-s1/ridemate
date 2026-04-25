#pragma once

#include <stdexcept>

template <typename T>
class Stack {
private:
    struct Node {
        T data;
        Node* next;
        Node(const T& d) : data(d), next(nullptr) {}
    };

    Node* top_;   // pointer to top node
    int size_;    // number of elements

public:
    // --- Constructor ---
    Stack() : top_(nullptr), size_(0) {}

    // --- Destructor (free all nodes) ---
    ~Stack() {
        while (!empty()) {
            pop();
        }
    }

    // --- Push element onto stack ---
    void push(const T& value) {
        Node* newNode = new Node(value);
        newNode->next = top_;
        top_ = newNode;
        size_++;
    }

    // --- Pop element from stack ---
    T pop() {
        if (empty()) {
            throw std::runtime_error("Stack is empty! Cannot pop.");
        }

        Node* temp = top_;
        T value = temp->data;

        top_ = top_->next;
        delete temp;
        size_--;

        return value;
    }

    // --- Access top element ---
    T& top() {
        if (empty()) {
            throw std::runtime_error("Stack is empty! Cannot access top.");
        }
        return top_->data;
    }

    const T& top() const {
        if (empty()) {
            throw std::runtime_error("Stack is empty! Cannot access top.");
        }
        return top_->data;
    }

    // --- Check if stack is empty ---
    bool empty() const noexcept {
        return size_ == 0;
    }

    // --- Get the stack size ---
    int size() const noexcept {
        return size_;
    }
};
