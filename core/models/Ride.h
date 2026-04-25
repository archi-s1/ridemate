#pragma once

#include <string>
#include <iostream>

enum RideType { RIDE_AUTO, RIDE_MOTO, RIDE_MINI, RIDE_SEDAN };

class Ride {
private:
    int rideId_;           // Unique ride ID
    int passengerId_;      // Passenger requesting the ride
    int driverId_;         // Driver assigned (=-1 if none)
    int pickup_;           // Pickup location node
    int drop_;             // Drop location node
    double fare_;          // Fare calculated after completion
    std::string status_;   // requested / ongoing / completed / cancelled
    RideType rideType_;
   

public:
    
    Ride() noexcept
        : rideId_(-1), passengerId_(-1), driverId_(-1),
          pickup_(0), drop_(0), fare_(0.0), status_("unknown"),
          rideType_(RIDE_MINI){}

    Ride(int rideId, int passengerId, int pickup, int drop, RideType type) noexcept
        : rideId_(rideId), passengerId_(passengerId), driverId_(-1),
          pickup_(pickup), drop_(drop), fare_(0.0), status_("requested"),
          rideType_ (type){}

    
    int getRideId() const noexcept { return rideId_; }
    int getPassengerId() const noexcept { return passengerId_; }
    int getDriverId() const noexcept { return driverId_; }
    int getPickup() const noexcept { return pickup_; }
    int getDrop() const noexcept { return drop_; }
    double getFare() const noexcept { return fare_; }
    const std::string& getStatus() const noexcept { return status_; }
    RideType getRideType() const { return rideType_; }

    
    void assignDriver(int driverId) noexcept {
        driverId_ = driverId;
        status_ = "ongoing";
    }

    
    void setFare(double amount) noexcept {
        fare_ = amount;
    }

    void markCompleted() noexcept {
        status_ = "completed";
    }

    void cancelRide() noexcept {
        status_ = "cancelled";
    }

    
    void printInfo() const {
        std::cout << "Ride ID: " << rideId_
                  << ", Passenger ID: " << passengerId_
                  << ", Driver ID: " << driverId_
                  << ", Pickup: " << pickup_
                  << ", Drop: " << drop_
                  << ", Fare: " << fare_
                  << ", Status: " << status_
                  << ", RideType: " << rideType_
                  << "\n";
    }

    bool operator==(const Ride& other) const {
    return this->rideId_ == other.rideId_;
    }

};
