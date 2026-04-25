#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>

#include "models/User.h"
#include "models/Driver.h"
#include "models/Passenger.h"
#include "models/Ride.h"

#include "ds/Graph.h"
#include "ds/Queue.h"
#include "ds/Stack.h"
#include "ds/LinkedList.h"
#include "ds/DoublyLinkedList.h"
#include "ds/CircularQueue.h"
#include "ds/MinHeap.h"
#include "ds/Trie.h"
#include "ds/DSU.h"

#include "PathFinder.h"
#include "RouteCache.h"
#include "FileStore.h"

class RideService {
private:
    int nextUserId_;
    int nextRideId_;
    FileStore store_;

public:
    // Exposed so CommandRouter and main.cpp can pre-warm / inspect
    RouteCache routeCache_;

    // -----------------------------------------------------------------------
    //  Core data structures
    // -----------------------------------------------------------------------
    Graph cityMap;
    DSU   zoneSystem;

    std::unordered_map<int, Driver>    drivers;
    std::unordered_map<int, Passenger> passengers;
    std::unordered_map<int, Ride>      rides;

    Queue<int>              requestQueue;       // pending ride request IDs
    CircularQueue<int>      availableDrivers;   // online driver pool
    MinHeap<std::pair<int,int>> driverHeap;     // nearest-driver matching

    std::unordered_map<int, Stack<Ride>> passengerHistory;
    std::unordered_map<int, Stack<Ride>> driverHistory;

    LinkedList<Ride>       ongoingRides;
    DoublyLinkedList<Ride> recentRides;

    Trie locationTrie;

    // -----------------------------------------------------------------------
    //  Constructor — loads persisted data, pre-warms cache
    // -----------------------------------------------------------------------
    explicit RideService(int nodeCount = 20, const std::string& dataDir = "data");

    // -----------------------------------------------------------------------
    //  User management
    // -----------------------------------------------------------------------
    int  addDriver(const std::string& name, int location, VehicleType type);
    int  addPassenger(const std::string& name, int location);
    bool removeDriver(int driverId);            // NEW: dynamic removal
    bool toggleAvailability(int driverId);

    // -----------------------------------------------------------------------
    //  Ride requests
    // -----------------------------------------------------------------------
    int  requestRideWithType(int passengerId, int pickup, int drop, RideType type);
    bool cancelRequest(int rideId);

    // -----------------------------------------------------------------------
    //  Fare (uses RouteCache — O(1) after warmup)
    // -----------------------------------------------------------------------
    double previewFareForType(int pickup, int drop, RideType type);
    double calculateFare(int pickup, int drop);

    // -----------------------------------------------------------------------
    //  Driver assignment
    // -----------------------------------------------------------------------
    int  findNearestDriver(int pickupLocation, RideType type);
    bool assignRide();

    // -----------------------------------------------------------------------
    //  Ride completion
    // -----------------------------------------------------------------------
    bool completeRide(int rideId, int rating);

    // -----------------------------------------------------------------------
    //  Graph / city map mutations  (invalidate cache automatically)
    // -----------------------------------------------------------------------
    void addCityEdge(int u, int v, int weight);
    bool removeCityEdge(int u, int v);

    // -----------------------------------------------------------------------
    //  Display helpers
    // -----------------------------------------------------------------------
    void showPassengerHistory(int passengerId) const;
    void showDriverHistory(int driverId) const;
    void showOngoingRides() const;
    void showRecentRides() const;
    void showCityMap() const;
    void showAvailableDrivers() const;
    void searchLocation(const std::string& prefix) const;
    void showAllDrivers() const;
    void showAllPassengers() const;

    // Which algorithm is currently selected for this graph
    std::string currentAlgorithm() const;

    // -----------------------------------------------------------------------
    //  Persistence
    // -----------------------------------------------------------------------
    void saveAll() const;

    // -----------------------------------------------------------------------
    //  Internal helpers
    // -----------------------------------------------------------------------
private:
    bool isValidPassenger(int id) const;
    bool isValidDriver(int id)    const;
    bool rideExists(int id)       const;

    void updateEarnings(int driverId, double fare);
    void addRating(int driverId, int rating);
    void rebuildAvailableDriverPool();
};
