#pragma once

#include <stdexcept>
#include <iostream>

template <typename T>
class DoublyLinkedList {
private:
    struct Node {
        T data;
        Node* next;
        Node* prev;

        Node(const T& d) : data(d), next(nullptr), prev(nullptr) {}
    };

    Node* head_;
    Node* tail_;
    int size_;

public:
    DoublyLinkedList() : head_(nullptr), tail_(nullptr), size_(0) {}

    ~DoublyLinkedList() {
        clear();
    }

    void clear() {
        Node* curr = head_;
        while (curr) {
            Node* temp = curr;
            curr = curr->next;
            delete temp;
        }
        head_ = tail_ = nullptr;
        size_ = 0;
    }

    void pushFront(const T& value) {
        Node* newNode = new Node(value);

        if (!head_) {
            head_ = tail_ = newNode;
        } else {
            newNode->next = head_;
            head_->prev = newNode;
            head_ = newNode;
        }
        size_++;
    }

    void pushBack(const T& value) {
        Node* newNode = new Node(value);

        if (!tail_) {
            head_ = tail_ = newNode;
        } else {
            tail_->next = newNode;
            newNode->prev = tail_;
            tail_ = newNode;
        }
        size_++;
    }

    bool remove(const T& value) {
        if (!head_) return false;

        Node* curr = head_;
        while (curr && !(curr->data == value)) {
            curr = curr->next;
        }

        if (!curr) return false; // not found

        if (curr == head_) head_ = curr->next;
        if (curr == tail_) tail_ = curr->prev;

        if (curr->prev) curr->prev->next = curr->next;
        if (curr->next) curr->next->prev = curr->prev;

        delete curr;
        size_--;
        return true;
    }

    // --- List size ---
    int size() const noexcept {
        return size_;
    }

    bool empty() const noexcept {
        return size_ == 0;
    }

    // --- Forward Iterator ---
    class Iterator {
    private:
        Node* node_;
    public:
        Iterator(Node* ptr) : node_(ptr) {}
        bool operator!=(const Iterator& other) const { return node_ != other.node_; }
        Iterator& operator++() { node_ = node_->next; return *this; }
        T& operator*() { return node_->data; }
    };

    Iterator begin() { return Iterator(head_); }
    Iterator end() { return Iterator(nullptr); }

    // --- Debug print ---
    void printList() const {
        Node* curr = head_;
        while (curr) {
            // For Ride objects: prints Ride ID
            std::cout << "[" << curr->data.getRideId() << "] <-> ";
            curr = curr->next;
        }
        std::cout << "NULL\n";
    }
};
