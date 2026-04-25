#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <random>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <chrono>

// ---------------------------------------------------------------------------
//  AuthManager
//
//  Lightweight session-token auth that lives entirely in C++.
//  Passwords are stored as a simple djb2 hash (hex string) — good enough
//  for a project context; swap with bcrypt in production.
//
//  Session tokens are random 32-char hex strings kept in an in-memory map.
//  There is no expiry in this version; logout removes the token.
//
//  User records (id, name, role, passwordHash) are persisted through
//  FileStore — AuthManager just manages in-memory sessions and exposes
//  helper functions that CommandRouter and FileStore use.
// ---------------------------------------------------------------------------

enum class UserRole { PASSENGER, DRIVER, UNKNOWN };

struct AuthUser {
    int         userId;
    std::string name;
    std::string email;
    UserRole    role;
    std::string passwordHash;
};

class AuthManager {
private:
    // email → AuthUser (loaded from file at startup)
    std::unordered_map<std::string, AuthUser> users_;

    // token → email (live sessions only — not persisted)
    std::unordered_map<std::string, std::string> sessions_;

    int nextAuthId_ = 1;

    // ------------------------------------------------------------------
    //  Simple djb2 hash — deterministic, no external deps
    // ------------------------------------------------------------------
    static std::string hashPassword(const std::string& pw) {
        unsigned long hash = 5381;
        for (char c : pw) hash = ((hash << 5) + hash) ^ static_cast<unsigned char>(c);

        std::ostringstream oss;
        oss << std::hex << std::setw(16) << std::setfill('0') << hash;
        return oss.str();
    }

    // ------------------------------------------------------------------
    //  Generate a random 32-char hex token
    // ------------------------------------------------------------------
    static std::string generateToken() {
        std::mt19937_64 rng(
            std::chrono::steady_clock::now().time_since_epoch().count()
        );
        std::uniform_int_distribution<uint64_t> dist;

        std::ostringstream oss;
        oss << std::hex << std::setfill('0')
            << std::setw(16) << dist(rng)
            << std::setw(16) << dist(rng);
        return oss.str();
    }

public:

    // ------------------------------------------------------------------
    //  Called by FileStore after loading users.json
    // ------------------------------------------------------------------
    void loadUsers(const std::vector<AuthUser>& users, int maxId) {
        users_.clear();
        for (const auto& u : users) {
            users_[u.email] = u;
        }
        nextAuthId_ = maxId + 1;
        std::cerr << "[AuthManager] Loaded " << users_.size() << " users.\n";
    }

    // Provide read-only access to users map so FileStore can serialise it
    const std::unordered_map<std::string, AuthUser>& allUsers() const {
        return users_;
    }

    // ------------------------------------------------------------------
    //  registerUser
    //  Returns {success, message, userId}
    // ------------------------------------------------------------------
    struct RegisterResult { bool ok; std::string msg; int userId; };

    RegisterResult registerUser(const std::string& name,
                                const std::string& email,
                                const std::string& password,
                                UserRole role) {
        if (name.empty() || email.empty() || password.empty())
            return {false, "Name, email and password are required.", -1};

        if (users_.count(email))
            return {false, "Email already registered.", -1};

        if (password.size() < 4)
            return {false, "Password must be at least 4 characters.", -1};

        int id = nextAuthId_++;
        AuthUser u{id, name, email, role, hashPassword(password)};
        users_[email] = u;
        return {true, "Registration successful.", id};
    }

    // ------------------------------------------------------------------
    //  login
    //  Returns {success, message, token}
    // ------------------------------------------------------------------
    struct LoginResult { bool ok; std::string msg; std::string token; int userId; UserRole role; };

    LoginResult login(const std::string& email, const std::string& password) {
        auto it = users_.find(email);
        if (it == users_.end())
            return {false, "Email not found.", "", -1, UserRole::UNKNOWN};

        const AuthUser& u = it->second;
        if (u.passwordHash != hashPassword(password))
            return {false, "Incorrect password.", "", -1, UserRole::UNKNOWN};

        // Invalidate any existing session for this user
        for (auto sit = sessions_.begin(); sit != sessions_.end(); ) {
            if (sit->second == email) sit = sessions_.erase(sit);
            else ++sit;
        }

        std::string token = generateToken();
        sessions_[token] = email;
        return {true, "Login successful.", token, u.userId, u.role};
    }

    // ------------------------------------------------------------------
    //  logout
    // ------------------------------------------------------------------
    bool logout(const std::string& token) {
        return sessions_.erase(token) > 0;
    }

    // ------------------------------------------------------------------
    //  verifyToken
    //  Returns pointer to AuthUser or nullptr if token is invalid.
    // ------------------------------------------------------------------
    const AuthUser* verifyToken(const std::string& token) const {
        auto sit = sessions_.find(token);
        if (sit == sessions_.end()) return nullptr;

        auto uit = users_.find(sit->second);
        if (uit == users_.end()) return nullptr;

        return &uit->second;
    }

    // ------------------------------------------------------------------
    //  Helpers
    // ------------------------------------------------------------------
    static std::string roleToString(UserRole r) {
        switch (r) {
            case UserRole::DRIVER:    return "driver";
            case UserRole::PASSENGER: return "passenger";
            default:                  return "unknown";
        }
    }

    static UserRole roleFromString(const std::string& s) {
        if (s == "driver")    return UserRole::DRIVER;
        if (s == "passenger") return UserRole::PASSENGER;
        return UserRole::UNKNOWN;
    }

    int nextId() const noexcept { return nextAuthId_; }
};
