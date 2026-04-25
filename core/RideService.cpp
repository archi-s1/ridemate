#include "RideService.h"

// ==========================================================================
//  CONSTRUCTOR — load persisted state, build city map, pre-warm cache
// ==========================================================================

RideService::RideService(int nodeCount, const std::string& dataDir)
    : nextUserId_(1),
      nextRideId_(1),
      store_(dataDir),
      cityMap(nodeCount),
      zoneSystem(nodeCount),
      availableDrivers(500)
{
    // --- Seed location trie ---
    for (const auto& loc : {
        "mgroad","brigaderoad","ecity","banashankari","whitefield",
        "indiranagar","ghaziabad","jpnagar","hebbal","yelahanka",
        "koramangala","malleshwaram","rajajinagar","basavanagudi","btm"
    }) locationTrie.insert(loc);

    // --- Load persisted counters ---
    auto counters = store_.loadCounters();
    nextUserId_ = counters.nextUserId;
    nextRideId_ = counters.nextRideId;

    // --- Load persisted drivers ---
    drivers = store_.loadDrivers();
    if (drivers.empty()) {
        std::cerr << "[RideService] No saved drivers found — starting fresh.\n";
    } else {
        std::cerr << "[RideService] Loaded " << drivers.size() << " drivers.\n";
    }

    // --- Load persisted passengers ---
    passengers = store_.loadPassengers();
    if (!passengers.empty())
        std::cerr << "[RideService] Loaded " << passengers.size() << " passengers.\n";

    // --- Load completed ride history ---
    auto savedRides = store_.loadRides();
    for (auto& [id, ride] : savedRides) {
        rides[id] = ride;
        passengerHistory[ride.getPassengerId()].push(ride);
        driverHistory[ride.getDriverId()].push(ride);
        recentRides.pushFront(ride);
    }
    if (!savedRides.empty())
        std::cerr << "[RideService] Loaded " << savedRides.size() << " completed rides.\n";

    // --- Rebuild available driver pool from loaded state ---
    rebuildAvailableDriverPool();

    // --- Pre-warm route cache (requires city map to be set up first) ---
    // City map edges are added by main.cpp right after construction via
    // addCityEdge(), so the actual pre-warm happens at first use or
    // when main.cpp calls routeCache_.preWarm() explicitly.
}

// ==========================================================================
//  CITY MAP MUTATIONS
// ==========================================================================

void RideService::addCityEdge(int u, int v, int weight) {
    cityMap.addEdge(u, v, weight);
    zoneSystem.unite(u, v);
    routeCache_.invalidate();
}

bool RideService::removeCityEdge(int u, int v) {
    bool removed = cityMap.removeEdge(u, v);
    if (removed) routeCache_.invalidate();
    return removed;
}

// ==========================================================================
//  FARE  (RouteCache sits in front of PathFinder)
// ==========================================================================

double RideService::calculateFare(int pickup, int drop) {
    int dist = routeCache_.get(cityMap, pickup, drop);
    if (dist == std::numeric_limits<int>::max()) return -1.0;
    return dist * 2.0;
}

double RideService::previewFareForType(int pickup, int drop, RideType type) {
    double base = calculateFare(pickup, drop);
    if (base < 0) return -1.0;

    switch (type) {
        case RIDE_MOTO:  return base * 0.7;
        case RIDE_AUTO:  return base * 1.0;
        case RIDE_MINI:  return base * 1.3;
        case RIDE_SEDAN: return base * 2.0;
    }
    return base;
}

// ==========================================================================
//  USER MANAGEMENT
// ==========================================================================

int RideService::addDriver(const std::string& name, int location, VehicleType type) {
    int id = nextUserId_++;
    drivers[id] = Driver(id, name, location, type);
    availableDrivers.enqueue(id);
    store_.saveDrivers(drivers);
    store_.saveCounters(nextUserId_, nextRideId_);
    return id;
}

int RideService::addPassenger(const std::string& name, int location) {
    int id = nextUserId_++;
    passengers[id] = Passenger(id, name, location);
    store_.savePassengers(passengers);
    store_.saveCounters(nextUserId_, nextRideId_);
    return id;
}

bool RideService::removeDriver(int driverId) {
    if (!isValidDriver(driverId)) return false;

    // Cannot remove a driver who is currently on a ride
    for (const auto& ride : ongoingRides) {
        if (ride.getDriverId() == driverId) {
            std::cerr << "[RideService] Cannot remove driver " << driverId
                      << " — has an ongoing ride.\n";
            return false;
        }
    }

    drivers.erase(driverId);
    // Note: CircularQueue has no targeted remove, so we rebuild the pool
    rebuildAvailableDriverPool();
    store_.saveDrivers(drivers);
    return true;
}

bool RideService::toggleAvailability(int driverId) {
    if (!isValidDriver(driverId)) return false;

    bool newStatus = !drivers[driverId].isAvailable();
    drivers[driverId].setAvailability(newStatus);

    if (newStatus) {
        availableDrivers.enqueue(driverId);
    }
    // If going offline the driver will just be skipped during matching
    store_.saveDrivers(drivers);
    return true;
}

// ==========================================================================
//  RIDE REQUESTS
// ==========================================================================

int RideService::requestRideWithType(int passengerId, int pickup,
                                     int drop, RideType type) {
    if (!isValidPassenger(passengerId)) return -1;

    // Verify route is reachable
    int dist = routeCache_.get(cityMap, pickup, drop);
    if (dist == std::numeric_limits<int>::max()) {
        std::cerr << "[RideService] No path from " << pickup
                  << " to " << drop << ".\n";
        return -1;
    }

    int rideId = nextRideId_++;
    rides[rideId] = Ride(rideId, passengerId, pickup, drop, type);
    requestQueue.enqueue(rideId);
    store_.saveCounters(nextUserId_, nextRideId_);
    return rideId;
}

bool RideService::cancelRequest(int rideId) {
    if (!rideExists(rideId)) return false;

    Ride& r = rides[rideId];
    if (r.getStatus() != "requested") return false;

    r.cancelRide();
    return true;
}

// ==========================================================================
//  DRIVER ASSIGNMENT
// ==========================================================================

int RideService::findNearestDriver(int pickupLocation, RideType type) {
    driverHeap.clear();

    int n = availableDrivers.size();
    for (int i = 0; i < n; ++i) {
        int driverId = availableDrivers.dequeue();
        availableDrivers.enqueue(driverId); // keep cycling

        if (!isValidDriver(driverId)) continue;
        if (!drivers[driverId].isAvailable()) continue;

        // Match vehicle type
        VehicleType vt = drivers[driverId].getVehicleType();
        if ((type == RIDE_AUTO  && vt != AUTO)  ||
            (type == RIDE_MOTO  && vt != MOTO)  ||
            (type == RIDE_MINI  && vt != MINI)  ||
            (type == RIDE_SEDAN && vt != SEDAN))
            continue;

        int dist = routeCache_.get(cityMap, drivers[driverId].getLocation(),
                                   pickupLocation);
        if (dist != std::numeric_limits<int>::max())
            driverHeap.push({dist, driverId});
    }

    if (driverHeap.empty()) return -1;
    return driverHeap.pop().second;
}

bool RideService::assignRide() {
    if (requestQueue.empty()) return false;

    int rideId = requestQueue.dequeue();
    Ride& ride = rides[rideId];

    // Skip cancelled rides
    if (ride.getStatus() == "cancelled") {
        std::cerr << "[RideService] Skipped cancelled ride " << rideId << ".\n";
        return false;
    }

    int driverId = findNearestDriver(ride.getPickup(), ride.getRideType());
    if (driverId == -1) {
        std::cerr << "[RideService] No available driver for ride " << rideId << ".\n";
        // Re-queue so it can be tried again
        requestQueue.enqueue(rideId);
        return false;
    }

    double fare = previewFareForType(ride.getPickup(), ride.getDrop(),
                                     ride.getRideType());
    ride.assignDriver(driverId);
    ride.setFare(fare);

    drivers[driverId].setAvailability(false);
    ongoingRides.pushBack(ride);

    std::cerr << "[RideService] Ride " << rideId
              << " assigned to driver " << driverId
              << "  fare=₹" << fare << "\n";
    return true;
}

// ==========================================================================
//  RIDE COMPLETION
// ==========================================================================

bool RideService::completeRide(int rideId, int rating) {
    if (!rideExists(rideId)) return false;

    Ride& ride = rides[rideId];
    if (ride.getStatus() != "ongoing") return false;

    int driverId    = ride.getDriverId();
    int passengerId = ride.getPassengerId();

    double fare = previewFareForType(ride.getPickup(), ride.getDrop(),
                                     ride.getRideType());
    ride.setFare(fare);
    ride.markCompleted();

    // Update driver state
    updateEarnings(driverId, fare);
    addRating(driverId, rating);
    drivers[driverId].setLocation(ride.getDrop());
    drivers[driverId].setAvailability(true);
    availableDrivers.enqueue(driverId);

    // Move from ongoing → history
    ongoingRides.remove(ride);
    passengerHistory[passengerId].push(ride);
    driverHistory[driverId].push(ride);
    recentRides.pushFront(ride);

    // Persist
    store_.saveDrivers(drivers);
    store_.saveRides(rides);

    std::cerr << "[RideService] Ride " << rideId
              << " completed. Fare=₹" << fare
              << "  Rating=" << rating << "\n";
    return true;
}

// ==========================================================================
//  DISPLAY
// ==========================================================================

void RideService::showPassengerHistory(int passengerId) const {
    if (!isValidPassenger(passengerId)) {
        std::cout << "Passenger not found.\n"; return;
    }
    auto it = passengerHistory.find(passengerId);
    if (it == passengerHistory.end() || it->second.empty()) {
        std::cout << "No ride history.\n"; return;
    }
    Stack<Ride> copy = it->second;
    while (!copy.empty()) { copy.top().printInfo(); copy.pop(); }
}

void RideService::showDriverHistory(int driverId) const {
    if (!isValidDriver(driverId)) {
        std::cout << "Driver not found.\n"; return;
    }
    auto it = driverHistory.find(driverId);
    if (it == driverHistory.end() || it->second.empty()) {
        std::cout << "No ride history.\n"; return;
    }
    Stack<Ride> copy = it->second;
    while (!copy.empty()) { copy.top().printInfo(); copy.pop(); }
}

void RideService::showOngoingRides() const {
    if (ongoingRides.empty()) { std::cout << "No ongoing rides.\n"; return; }
    for (const auto& ride : const_cast<LinkedList<Ride>&>(ongoingRides))
        ride.printInfo();
}

void RideService::showRecentRides() const {
    if (recentRides.empty()) { std::cout << "No recent rides.\n"; return; }
    for (const auto& ride : const_cast<DoublyLinkedList<Ride>&>(recentRides))
        ride.printInfo();
}

void RideService::showCityMap() const {
    cityMap.printGraph();
}

void RideService::showAvailableDrivers() const {
    int count = 0;
    for (const auto& [id, d] : drivers) {
        if (d.isAvailable()) {
            std::cout << "Driver " << id << " | " << d.getName()
                      << " | " << FileStore::vehicleTypeToStr(d.getVehicleType())
                      << " | loc=" << d.getLocation()
                      << " | rating=" << d.getAverageRating() << "\n";
            ++count;
        }
    }
    if (count == 0) std::cout << "No drivers currently available.\n";
}

void RideService::searchLocation(const std::string& prefix) const {
    auto results = locationTrie.searchPrefix(prefix);
    if (results.empty()) { std::cout << "No locations found.\n"; return; }
    for (const auto& s : results) std::cout << "  " << s << "\n";
}

void RideService::showAllDrivers() const {
    if (drivers.empty()) { std::cout << "No drivers registered.\n"; return; }
    std::cout << "\n--- ALL DRIVERS ---\n";
    for (const auto& [id, d] : drivers) {
        std::cout << "ID:" << d.getId()
                  << " | " << d.getName()
                  << " | " << FileStore::vehicleTypeToStr(d.getVehicleType())
                  << " | loc=" << d.getLocation()
                  << " | avail=" << (d.isAvailable() ? "yes" : "no")
                  << " | earn=₹" << d.getEarnings()
                  << " | rating=" << d.getAverageRating()
                  << "\n";
    }
}

void RideService::showAllPassengers() const {
    if (passengers.empty()) { std::cout << "No passengers registered.\n"; return; }
    std::cout << "\n--- ALL PASSENGERS ---\n";
    for (const auto& [id, p] : passengers) {
        std::cout << "ID:" << p.getId()
                  << " | " << p.getName()
                  << " | loc=" << p.getLocation() << "\n";
    }
}

std::string RideService::currentAlgorithm() const {
    return PathFinder::algorithmName(PathFinder::pickAlgorithm(cityMap));
}

// ==========================================================================
//  PERSISTENCE
// ==========================================================================

void RideService::saveAll() const {
    store_.saveDrivers(drivers);
    store_.savePassengers(passengers);
    store_.saveRides(rides);
    store_.saveCounters(nextUserId_, nextRideId_);
}

// ==========================================================================
//  INTERNAL HELPERS
// ==========================================================================

bool RideService::isValidPassenger(int id) const {
    return passengers.find(id) != passengers.end();
}

bool RideService::isValidDriver(int id) const {
    return drivers.find(id) != drivers.end();
}

bool RideService::rideExists(int id) const {
    return rides.find(id) != rides.end();
}

void RideService::updateEarnings(int driverId, double fare) {
    if (isValidDriver(driverId)) drivers[driverId].addEarnings(fare);
}

void RideService::addRating(int driverId, int rating) {
    if (isValidDriver(driverId)) drivers[driverId].addRating(rating);
}

// Rebuild CircularQueue from scratch after driver additions / removals.
void RideService::rebuildAvailableDriverPool() {
    availableDrivers.clear();
    for (const auto& [id, d] : drivers) {
        if (d.isAvailable()) availableDrivers.enqueue(id);
    }
}
