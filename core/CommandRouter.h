#pragma once

#include <string>
#include <iostream>
#include "RideService.h"
#include "AuthManager.h"
#include "FileStore.h"
#include "../third_party/json.hpp"

using json = nlohmann::json;

class CommandRouter {
private:
    RideService& service_;
    AuthManager  auth_;
    FileStore    store_;

    static json ok(const std::string& msg = "ok") {
        return {{"ok", true}, {"msg", msg}};
    }
    static json err(const std::string& msg) {
        return {{"ok", false}, {"msg", msg}};
    }

    static json rideToJson(const Ride& r) {
        return {
            {"rideId",      r.getRideId()},
            {"passengerId", r.getPassengerId()},
            {"driverId",    r.getDriverId()},
            {"pickup",      r.getPickup()},
            {"drop",        r.getDrop()},
            {"fare",        r.getFare()},
            {"status",      r.getStatus()},
            {"rideType",    FileStore::rideTypeToStr(r.getRideType())}
        };
    }

    static json driverToJson(const Driver& d) {
        return {
            {"id",          d.getId()},
            {"name",        d.getName()},
            {"location",    d.getLocation()},
            {"vehicleType", FileStore::vehicleTypeToStr(d.getVehicleType())},
            {"available",   d.isAvailable()},
            {"earnings",    d.getEarnings()},
            {"rating",      d.getAverageRating()}
        };
    }

    static json passengerToJson(const Passenger& p) {
        return {
            {"id",       p.getId()},
            {"name",     p.getName()},
            {"location", p.getLocation()}
        };
    }

    static json stackToJsonArray(Stack<Ride> copy) {
        json arr = json::array();
        while (!copy.empty()) {
            arr.push_back(rideToJson(copy.top()));
            copy.pop();
        }
        return arr;
    }

    const AuthUser* requireAuth(const json& cmd, json& resp) {
        if (!cmd.contains("token") || !cmd["token"].is_string()) {
            resp = err("Authentication required.");
            return nullptr;
        }
        const AuthUser* user = auth_.verifyToken(cmd["token"].get<std::string>());
        if (!user) {
            resp = err("Invalid or expired session token.");
            return nullptr;
        }
        return user;
    }

    json dispatch(const json& cmd) {
        if (!cmd.contains("cmd") || !cmd["cmd"].is_string())
            return err("Missing 'cmd' field.");

        const std::string c = cmd["cmd"];

        // AUTH
        if (c == "register") {
            std::string name     = cmd.value("name", "");
            std::string email    = cmd.value("email", "");
            std::string password = cmd.value("password", "");
            std::string roleStr  = cmd.value("role", "passenger");
            UserRole    role     = AuthManager::roleFromString(roleStr);
            auto res = auth_.registerUser(name, email, password, role);
            if (!res.ok) return err(res.msg);
            if (role == UserRole::DRIVER) {
                service_.addDriver(name, cmd.value("location", 0),
                    FileStore::vehicleTypeFromStr(cmd.value("vehicleType","mini")));
            } else {
                service_.addPassenger(name, cmd.value("location", 0));
            }
            store_.saveAuthUsers(auth_.allUsers());
            auto j = ok(res.msg); j["userId"] = res.userId; j["role"] = roleStr; return j;
        }

        if (c == "login") {
            auto res = auth_.login(cmd.value("email",""), cmd.value("password",""));
            if (!res.ok) return err(res.msg);
            auto j = ok(res.msg);
            j["token"] = res.token; j["userId"] = res.userId;
            j["role"]  = AuthManager::roleToString(res.role);
            return j;
        }

        if (c == "logout") {
            return auth_.logout(cmd.value("token",""))
                ? ok("Logged out.") : err("Token not found.");
        }

        json resp;
        const AuthUser* caller = requireAuth(cmd, resp);
        if (!caller) return resp;

        // FARE
        if (c == "previewFare") {
            RideType rt = FileStore::rideTypeFromStr(cmd.value("rideType","mini"));
            double fare = service_.previewFareForType(
                              cmd.value("pickup",0), cmd.value("drop",0), rt);
            if (fare < 0) return err("No route between those locations.");
            auto j = ok(); j["fare"] = fare; j["algorithm"] = service_.currentAlgorithm();
            return j;
        }

        // RIDE LIFECYCLE
        if (c == "requestRide") {
            RideType rt = FileStore::rideTypeFromStr(cmd.value("rideType","mini"));
            int rideId = service_.requestRideWithType(
                cmd.value("passengerId",-1), cmd.value("pickup",-1),
                cmd.value("drop",-1), rt);
            if (rideId < 0) return err("Could not create ride request.");
            auto j = ok("Ride requested."); j["rideId"] = rideId; return j;
        }

        if (c == "cancelRide")
            return service_.cancelRequest(cmd.value("rideId",-1))
                ? ok("Ride cancelled.") : err("Cannot cancel ride.");

        if (c == "assignRide")
            return service_.assignRide()
                ? ok("Ride assigned.") : err("No pending requests or no available driver.");

        if (c == "completeRide")
            return service_.completeRide(cmd.value("rideId",-1), cmd.value("rating",5))
                ? ok("Ride completed.") : err("Cannot complete ride.");

        // USER MANAGEMENT
        if (c == "addDriver") {
            int id = service_.addDriver(cmd.value("name",""), cmd.value("location",0),
                         FileStore::vehicleTypeFromStr(cmd.value("vehicleType","mini")));
            auto j = ok("Driver added."); j["id"] = id; return j;
        }

        if (c == "addPassenger") {
            int id = service_.addPassenger(cmd.value("name",""), cmd.value("location",0));
            auto j = ok("Passenger added."); j["id"] = id; return j;
        }

        if (c == "removeDriver")
            return service_.removeDriver(cmd.value("driverId",-1))
                ? ok("Driver removed.") : err("Cannot remove driver.");

        if (c == "toggleAvail")
            return service_.toggleAvailability(cmd.value("driverId",-1))
                ? ok("Availability toggled.") : err("Driver not found.");

        // HISTORY
        if (c == "passengerHistory") {
            int pid = cmd.value("passengerId",-1);
            auto it = service_.passengerHistory.find(pid);
            json arr = (it != service_.passengerHistory.end())
                       ? stackToJsonArray(it->second) : json::array();
            auto j = ok(); j["rides"] = arr; return j;
        }

        if (c == "driverHistory") {
            int did = cmd.value("driverId",-1);
            auto it = service_.driverHistory.find(did);
            json arr = (it != service_.driverHistory.end())
                       ? stackToJsonArray(it->second) : json::array();
            auto j = ok(); j["rides"] = arr; return j;
        }

        // QUERIES
        if (c == "ongoingRides") {
            json arr = json::array();
            for (auto& ride : service_.ongoingRides) arr.push_back(rideToJson(ride));
            auto j = ok(); j["rides"] = arr; return j;
        }

        if (c == "recentRides") {
            json arr = json::array();
            for (auto& ride : service_.recentRides) arr.push_back(rideToJson(ride));
            auto j = ok(); j["rides"] = arr; return j;
        }

        if (c == "allDrivers") {
            json arr = json::array();
            for (const auto& [id, d] : service_.drivers) arr.push_back(driverToJson(d));
            auto j = ok(); j["drivers"] = arr; return j;
        }

        if (c == "allPassengers") {
            json arr = json::array();
            for (const auto& [id, p] : service_.passengers) arr.push_back(passengerToJson(p));
            auto j = ok(); j["passengers"] = arr; return j;
        }

        if (c == "availDrivers") {
            json arr = json::array();
            for (const auto& [id, d] : service_.drivers)
                if (d.isAvailable()) arr.push_back(driverToJson(d));
            auto j = ok(); j["drivers"] = arr; return j;
        }

        if (c == "cityMap") {
            json nodes = json::array();
            for (int i = 0; i < service_.cityMap.getNodeCount(); ++i) {
                json edges = json::array();
                for (const auto& [v, w] : service_.cityMap.getNeighbors(i))
                    edges.push_back({{"to", v}, {"weight", w}});
                nodes.push_back({{"node", i}, {"edges", edges}});
            }
            auto j = ok(); j["nodes"] = nodes; return j;
        }

        if (c == "searchLocation") {
            auto results = service_.locationTrie.searchPrefix(cmd.value("prefix",""));
            auto j = ok(); j["locations"] = results; return j;
        }

        // GRAPH MUTATIONS
        if (c == "addEdge") {
            service_.addCityEdge(cmd.value("u",-1), cmd.value("v",-1), cmd.value("weight",1));
            auto j = ok("Edge added."); j["algorithm"] = service_.currentAlgorithm(); return j;
        }

        if (c == "removeEdge") {
            if (!service_.removeCityEdge(cmd.value("u",-1), cmd.value("v",-1)))
                return err("Edge not found.");
            auto j = ok("Edge removed."); j["algorithm"] = service_.currentAlgorithm(); return j;
        }

        // META
        if (c == "algorithm") {
            auto j = ok(); j["algorithm"] = service_.currentAlgorithm(); return j;
        }

        if (c == "preWarmCache") {
            service_.routeCache_.preWarm(service_.cityMap);
            auto j = ok("Cache pre-warmed.");
            j["pairs"] = service_.routeCache_.cachedPairs();
            j["algorithm"] = service_.currentAlgorithm();
            return j;
        }

        return err("Unknown command: " + c);
    }

public:
    explicit CommandRouter(RideService& svc, const std::string& dataDir = "data")
        : service_(svc), store_(dataDir)
    {
        auto users = store_.loadAuthUsers();
        int maxId = 0;
        for (const auto& u : users) maxId = std::max(maxId, u.userId);
        auth_.loadUsers(users, maxId);
    }

    bool run() {
        std::string line;
        if (!std::getline(std::cin, line)) return false;
        if (line.empty()) return true;
        json cmd, resp;
        try {
            cmd = json::parse(line);
        } catch (const json::parse_error& e) {
            resp = err(std::string("JSON parse error: ") + e.what());
            std::cout << resp.dump() << "\n"; std::cout.flush();
            return true;
        }
        resp = dispatch(cmd);
        std::cout << resp.dump() << "\n"; std::cout.flush();
        return true;
    }
};
