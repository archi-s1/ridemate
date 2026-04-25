#pragma once

#include <stdexcept>
#include <iostream>

template <typename T>
class LinkedList {
private:
    struct Node {
        T data;
        Node* next;
        Node(const T& d) : data(d), next(nullptr) {}
    };

    Node* head_;     // start of the list
    int size_;

public:
    // --- Constructor ---
    LinkedList() : head_(nullptr), size_(0) {}

    // --- Destructor ---
    ~LinkedList() {
        clear();
    }

    // --- Remove all nodes ---
    void clear() {
        while (head_) {
            Node* temp = head_;
            head_ = head_->next;
            delete temp;
        }
        size_ = 0;
    }

    // --- Insert at end (append) ---
    void pushBack(const T& value) {
        Node* newNode = new Node(value);

        if (!head_) {
            head_ = newNode;
        } else {
            Node* curr = head_;
            while (curr->next) {
                curr = curr->next;
            }
            curr->next = newNode;
        }
        size_++;
    }

    // --- Remove a node by matching data (first occurrence) ---
    bool remove(const T& value) {
        if (!head_) return false;

        // if head needs removal
        if (head_->data == value) {
            Node* temp = head_;
            head_ = head_->next;
            delete temp;
            size_--;
            return true;
        }

        Node* curr = head_;
        while (curr->next && !(curr->next->data == value)) {
            curr = curr->next;
        }

        if (!curr->next) return false;

        Node* temp = curr->next;
        curr->next = curr->next->next;
        delete temp;
        size_--;
        return true;
    }

    // --- Check if empty ---
    bool empty() const noexcept {
        return size_ == 0;
    }

    // --- Return size ---
    int size() const noexcept {
        return size_;
    }

    // --- Iterator support for traversal ---
    class Iterator {
    private:
        Node* nodePtr;
    public:
        Iterator(Node* ptr) : nodePtr(ptr) {}
        bool operator!=(const Iterator& other) const { return nodePtr != other.nodePtr; }
        Iterator& operator++() { nodePtr = nodePtr->next; return *this; }
        T& operator*() { return nodePtr->data; }
    };

    Iterator begin() { return Iterator(head_); }
    Iterator end() { return Iterator(nullptr); }

    // --- Debug print ---
    void printList() const {
        Node* curr = head_;
        while (curr) {
            std::cout << curr->data.getRideId() << " -> ";
            curr = curr->next;
        }
        std::cout << "NULL\n";
    }
};
