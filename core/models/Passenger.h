#pragma once

#include "User.h"
#include <string>
#include <iostream>

class Passenger : public User {
private:
    int currentLocation_;      // Current node/location in the city graph

public:
    // --- Constructors ---
    Passenger() noexcept
        : User(), currentLocation_(0) {}

    Passenger(int id, const std::string& name, int location) noexcept
        : User(id, name, UserType::Passenger),
          currentLocation_(location) {}
    
    int getLocation() const noexcept {
        return currentLocation_;
    }

    void setLocation(int loc) noexcept {
        currentLocation_ = loc;
    }

    // Update location after completing a ride
    void updateAfterRide(int newLocation) noexcept {
        currentLocation_ = newLocation;
    }

    // For debugging or menu display
    void printInfo() const {
        std::cout << "Passenger ID: " << getId()
                  << ", Name: " << getName()
                  << ", Location: " << currentLocation_
                  << "\n";
    }
};
