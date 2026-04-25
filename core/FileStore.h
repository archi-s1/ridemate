#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>

#include "models/Driver.h"
#include "models/Passenger.h"
#include "models/Ride.h"
#include "AuthManager.h"
#include "../third_party/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
//  FileStore
//
//  Pure C++ fstream-based persistence.  All data is stored as JSON files
//  in the data/ directory (relative to the binary's working directory).
//
//  Files:
//    data/drivers.json      — array of driver records
//    data/passengers.json   — array of passenger records
//    data/rides.json        — array of completed ride records
//    data/counters.json     — {nextUserId, nextRideId}
//    data/users.json        — array of auth user records
//
//  Convention:
//    load*()  — read from disk into the supplied collection
//    save*()  — serialise the supplied collection to disk
//
//  All save operations are atomic: write to a .tmp file then rename,
//  so a crash mid-write never corrupts the existing data file.
// ---------------------------------------------------------------------------

class FileStore {
private:
    std::string dataDir_;

    std::string path(const std::string& filename) const {
        return dataDir_ + "/" + filename;
    }

    // Atomic write: write to .tmp, then rename over the real file.
    bool atomicWrite(const std::string& filepath, const json& j) const {
        std::string tmp = filepath + ".tmp";
        {
            std::ofstream ofs(tmp);
            if (!ofs.is_open()) {
                std::cerr << "[FileStore] Cannot open for writing: " << tmp << "\n";
                return false;
            }
            ofs << j.dump(2);  // 2-space indent for readability
        }
        // Replace destination with the temp file
        std::error_code ec;
        fs::rename(tmp, filepath, ec);
        if (ec) {
            std::cerr << "[FileStore] Rename failed: " << ec.message() << "\n";
            return false;
        }
        return true;
    }

    // Read a JSON file; returns an empty json object on any error.
    json readFile(const std::string& filepath) const {
        std::ifstream ifs(filepath);
        if (!ifs.is_open()) return json{};
        try {
            json j;
            ifs >> j;
            return j;
        } catch (const json::parse_error& e) {
            std::cerr << "[FileStore] Parse error in " << filepath
                      << ": " << e.what() << "\n";
            return json{};
        }
    }

public:
    explicit FileStore(const std::string& dataDir = "data")
        : dataDir_(dataDir)
    {
        fs::create_directories(dataDir_);
    }

    // ======================================================================
    //  COUNTERS
    // ======================================================================

    struct Counters { int nextUserId = 1; int nextRideId = 1; };

    Counters loadCounters() const {
        json j = readFile(path("counters.json"));
        Counters c;
        if (j.contains("nextUserId"))  c.nextUserId  = j["nextUserId"];
        if (j.contains("nextRideId"))  c.nextRideId  = j["nextRideId"];
        return c;
    }

    bool saveCounters(int nextUserId, int nextRideId) const {
        json j = {{"nextUserId", nextUserId}, {"nextRideId", nextRideId}};
        return atomicWrite(path("counters.json"), j);
    }

    // ======================================================================
    //  AUTH USERS
    // ======================================================================

    std::vector<AuthUser> loadAuthUsers() const {
        json j = readFile(path("users.json"));
        std::vector<AuthUser> users;
        if (!j.is_array()) return users;

        for (const auto& item : j) {
            AuthUser u;
            u.userId       = item.value("userId", -1);
            u.name         = item.value("name", "");
            u.email        = item.value("email", "");
            u.role         = AuthManager::roleFromString(item.value("role", "unknown"));
            u.passwordHash = item.value("passwordHash", "");
            if (u.userId > 0 && !u.email.empty()) users.push_back(u);
        }
        return users;
    }

    bool saveAuthUsers(const std::unordered_map<std::string, AuthUser>& users) const {
        json arr = json::array();
        for (const auto& [email, u] : users) {
            arr.push_back({
                {"userId",       u.userId},
                {"name",         u.name},
                {"email",        u.email},
                {"role",         AuthManager::roleToString(u.role)},
                {"passwordHash", u.passwordHash}
            });
        }
        return atomicWrite(path("users.json"), arr);
    }

    // ======================================================================
    //  DRIVERS
    // ======================================================================

    static std::string vehicleTypeToStr(VehicleType vt) {
        switch (vt) {
            case AUTO:  return "auto";
            case MOTO:  return "moto";
            case MINI:  return "mini";
            case SEDAN: return "sedan";
        }
        return "mini";
    }

    static VehicleType vehicleTypeFromStr(const std::string& s) {
        if (s == "auto")  return AUTO;
        if (s == "moto")  return MOTO;
        if (s == "sedan") return SEDAN;
        return MINI;
    }

    std::unordered_map<int, Driver> loadDrivers() const {
        json j = readFile(path("drivers.json"));
        std::unordered_map<int, Driver> drivers;
        if (!j.is_array()) return drivers;

        for (const auto& item : j) {
            int id       = item.value("id", -1);
            std::string name = item.value("name", "");
            int loc      = item.value("location", 0);
            VehicleType vt = vehicleTypeFromStr(item.value("vehicleType", "mini"));
            bool avail   = item.value("available", true);
            double earn  = item.value("earnings", 0.0);

            if (id < 0) continue;

            Driver d(id, name, loc, vt);
            d.setAvailability(avail);
            d.addEarnings(earn);

            // Restore ratings
            if (item.contains("ratings") && item["ratings"].is_array()) {
                for (int r : item["ratings"]) d.addRating(r);
            }

            drivers[id] = d;
        }
        return drivers;
    }

    bool saveDrivers(const std::unordered_map<int, Driver>& drivers) const {
        json arr = json::array();
        for (const auto& [id, d] : drivers) {
            json ratings = json::array();
            for (int r : d.getRatings()) ratings.push_back(r);

            arr.push_back({
                {"id",          d.getId()},
                {"name",        d.getName()},
                {"location",    d.getLocation()},
                {"vehicleType", vehicleTypeToStr(d.getVehicleType())},
                {"available",   d.isAvailable()},
                {"earnings",    d.getEarnings()},
                {"ratings",     ratings}
            });
        }
        return atomicWrite(path("drivers.json"), arr);
    }

    // ======================================================================
    //  PASSENGERS
    // ======================================================================

    std::unordered_map<int, Passenger> loadPassengers() const {
        json j = readFile(path("passengers.json"));
        std::unordered_map<int, Passenger> passengers;
        if (!j.is_array()) return passengers;

        for (const auto& item : j) {
            int id       = item.value("id", -1);
            std::string name = item.value("name", "");
            int loc      = item.value("location", 0);
            if (id < 0) continue;
            passengers[id] = Passenger(id, name, loc);
        }
        return passengers;
    }

    bool savePassengers(const std::unordered_map<int, Passenger>& passengers) const {
        json arr = json::array();
        for (const auto& [id, p] : passengers) {
            arr.push_back({
                {"id",       p.getId()},
                {"name",     p.getName()},
                {"location", p.getLocation()}
            });
        }
        return atomicWrite(path("passengers.json"), arr);
    }

    // ======================================================================
    //  RIDES  (completed rides only — ongoing rides are transient)
    // ======================================================================

    static std::string rideTypeToStr(RideType rt) {
        switch (rt) {
            case RIDE_AUTO:  return "auto";
            case RIDE_MOTO:  return "moto";
            case RIDE_MINI:  return "mini";
            case RIDE_SEDAN: return "sedan";
        }
        return "mini";
    }

    static RideType rideTypeFromStr(const std::string& s) {
        if (s == "auto")  return RIDE_AUTO;
        if (s == "moto")  return RIDE_MOTO;
        if (s == "sedan") return RIDE_SEDAN;
        return RIDE_MINI;
    }

    std::unordered_map<int, Ride> loadRides() const {
        json j = readFile(path("rides.json"));
        std::unordered_map<int, Ride> rides;
        if (!j.is_array()) return rides;

        for (const auto& item : j) {
            int rideId      = item.value("rideId", -1);
            int passId      = item.value("passengerId", -1);
            int driverId    = item.value("driverId", -1);
            int pickup      = item.value("pickup", 0);
            int drop        = item.value("drop", 0);
            double fare     = item.value("fare", 0.0);
            std::string status = item.value("status", "completed");
            RideType rt     = rideTypeFromStr(item.value("rideType", "mini"));

            if (rideId < 0) continue;

            Ride r(rideId, passId, pickup, drop, rt);
            r.assignDriver(driverId);
            r.setFare(fare);
            r.markCompleted();    // all persisted rides are completed
            rides[rideId] = r;
        }
        return rides;
    }

    bool saveRides(const std::unordered_map<int, Ride>& rides) const {
        json arr = json::array();
        for (const auto& [id, r] : rides) {
            if (r.getStatus() != "completed") continue; // only persist completed
            arr.push_back({
                {"rideId",      r.getRideId()},
                {"passengerId", r.getPassengerId()},
                {"driverId",    r.getDriverId()},
                {"pickup",      r.getPickup()},
                {"drop",        r.getDrop()},
                {"fare",        r.getFare()},
                {"status",      r.getStatus()},
                {"rideType",    rideTypeToStr(r.getRideType())}
            });
        }
        return atomicWrite(path("rides.json"), arr);
    }
};
