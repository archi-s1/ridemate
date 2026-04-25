#pragma once

#include <string>

enum class UserType {
    Unknown = 0,
    Driver,
    Passenger
};

class User {
protected:
    int id_;                 // Unique ID for the user
    std::string name_;       // Display name
    UserType type_;          // Driver / Passenger / Unknown

public:
    // --- Constructors & Destructor ---

    User() noexcept
        : id_(-1), name_(), type_(UserType::Unknown) {}

    User(int id, const std::string& name, UserType type) noexcept
        : id_(id), name_(name), type_(type) {}

    virtual ~User() = default;

    int getId() const noexcept {
        return id_;
    }

    const std::string& getName() const noexcept {
        return name_;
    }

    UserType getType() const noexcept {
        return type_;
    }

    // --- Setters ---

    void setName(const std::string& name) {
        name_ = name;
    }

    void setType(UserType type) noexcept {
        type_ = type;
    }

    static std::string userTypeToString(UserType type) {
        switch (type) {
            case UserType::Driver:     return "driver";
            case UserType::Passenger:  return "passenger";
            default:                   return "unknown";
        }
    }
};
