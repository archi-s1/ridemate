#pragma once

#include <string>
#include <vector>

class Trie {
private:
    struct TrieNode {
        bool isEnd;
        TrieNode* children[26];   // a-z

        TrieNode() : isEnd(false) {
            for (int i = 0; i < 26; i++) children[i] = nullptr;
        }
    };

    TrieNode* root;

    void collect(TrieNode* node, std::string& prefix, std::vector<std::string>& results) const {
        if (!node) return;

        if (node->isEnd) {
            results.push_back(prefix);
        }

        for (int i = 0; i < 26; i++) {
            if (node->children[i]) {
                prefix.push_back('a' + i);
                collect(node->children[i], prefix, results);
                prefix.pop_back();
            }
        }
    }

public:
    Trie() {
        root = new TrieNode();
    }

    ~Trie() {
        clear(root);
    }

    void clear(TrieNode* node) {
        if (!node) return;
        for (int i = 0; i < 26; i++) {
            clear(node->children[i]);
        }
        delete node;
    }

    // --- Insert a lowercase word ---
    void insert(const std::string& word) {
        TrieNode* curr = root;

        for (char c : word) {
            if (!isalpha(c)) continue;  
            c = tolower(c);

            int idx = c - 'a';
            if (!curr->children[idx]) {
                curr->children[idx] = new TrieNode();
            }
            curr = curr->children[idx];
        }
        curr->isEnd = true;
    }

    std::vector<std::string> searchPrefix(const std::string& prefix) const {
        TrieNode* curr = root;
        std::string cleanPrefix = "";

        // traverse down the prefix
        for (char c : prefix) {
            if (!isalpha(c)) continue;
            c = tolower(c);

            int idx = c - 'a';
            if (!curr->children[idx]) {
                return {}; // no matches
            }
            curr = curr->children[idx];
            cleanPrefix.push_back(c);
        }

        // collect all words from this point
        std::vector<std::string> results;
        collect(curr, cleanPrefix, results);
        return results;
    }

    // --- Check if word exists exactly ---
    bool contains(const std::string& word) const {
        TrieNode* curr = root;
        for (char c : word) {
            if (!isalpha(c)) return false;
            c = tolower(c);

            int idx = c - 'a';
            if (!curr->children[idx]) return false;
            curr = curr->children[idx];
        }
        return curr->isEnd;
    }
};
