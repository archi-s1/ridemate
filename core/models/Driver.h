#pragma once

#include "User.h"
#include <vector>
#include <string>

enum VehicleType { AUTO, MOTO, MINI, SEDAN };

class Driver : public User {
private:
    bool available_;               // Online/Offline status
    int currentLocation_;          // Current node/location in graph
    std::vector<int> ratings_;     // Dynamic rating list
    double earnings_;              // Total earned so far
    VehicleType vehicleType_;      // NEW: type of vehicle (Auto/Moto/Mini/Sedan)

public:
    Driver() noexcept
        : User(), 
        available_(true), 
        currentLocation_(0), 
        earnings_(0.0),
        vehicleType_(MINI) {}

    Driver(int id, const std::string& name, int location,VehicleType type) noexcept
        : User(id, name, UserType::Driver),
          available_(true),
          currentLocation_(location),
          earnings_(0.0),
          vehicleType_(type) {}

    bool isAvailable() const noexcept {
        return available_;
    }

    void setAvailability(bool status) noexcept {
        available_ = status;
    }

    int getLocation() const noexcept {
        return currentLocation_;
    }

    void setLocation(int loc) noexcept {
        currentLocation_ = loc;
    }

    VehicleType getVehicleType() const noexcept {
        return vehicleType_;
    }

     void setVehicleType(VehicleType type) noexcept {
        vehicleType_ = type;
    }

    void addRating(int rating) {
        if (rating >= 1 && rating <= 5) {
            ratings_.push_back(rating);
        }
    }

    double getAverageRating() const noexcept {
        if (ratings_.empty()) return 0.0;

        double sum = 0;
        for (int r : ratings_) sum += r;
        return sum / ratings_.size();
    }

    const std::vector<int>& getRatings() const noexcept {
        return ratings_;
    }

    void addEarnings(double amount) noexcept {
        earnings_ += amount;
    }

    double getEarnings() const noexcept {
        return earnings_;
    }
};
