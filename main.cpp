#include <iostream>
#include <string>
#include "core/RideService.h"
#include "core/CommandRouter.h"
#include "core/RouteCache.h"

// ---------------------------------------------------------------------------
//  main.cpp — RideMate backend entry point
//
//  Startup sequence:
//    1. Create RideService (loads persisted drivers/passengers/rides)
//    2. Build city map (always deterministic — not persisted)
//    3. Pre-warm route cache
//    4. Create CommandRouter (loads auth users)
//    5. Enter JSON command loop (one JSON line in → one JSON line out)
//
//  The process stays alive until stdin closes (EOF), which the Node.js shim
//  triggers when it shuts down.
// ---------------------------------------------------------------------------

static void buildCityMap(RideService& svc) {
    // 20-node weighted undirected city graph.
    // Weights represent travel cost / distance units.
    // Edges added via addCityEdge() so DSU and RouteCache are both notified.
    svc.addCityEdge(0,  1,  5);
    svc.addCityEdge(1,  2,  7);
    svc.addCityEdge(2,  3,  3);
    svc.addCityEdge(3,  4,  6);
    svc.addCityEdge(4,  5,  4);
    svc.addCityEdge(1,  5,  10);
    svc.addCityEdge(2,  6,  12);
    svc.addCityEdge(6,  7,  8);
    svc.addCityEdge(7,  8,  4);
    svc.addCityEdge(8,  9,  6);
    svc.addCityEdge(9,  10, 3);
    svc.addCityEdge(10, 11, 8);
    svc.addCityEdge(11, 12, 5);
    svc.addCityEdge(12, 13, 7);
    svc.addCityEdge(13, 14, 4);
    svc.addCityEdge(14, 15, 6);
    svc.addCityEdge(15, 16, 2);
    svc.addCityEdge(16, 17, 9);
    svc.addCityEdge(17, 18, 3);
    svc.addCityEdge(18, 19, 8);
}

int main() {
    // Disable sync so cin/cout are fast for the pipe protocol
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::cerr << "[RideMate] Starting up...\n";

    // 1. Construct service — loads persisted data
    RideService service(20, "data");

    // 2. Build city map
    buildCityMap(service);

    // 3. Pre-warm the route cache
    //    This runs PathFinder (BFS or Dijkstra) for all 20 source nodes
    //    once, filling the 20×20 distance matrix in memory.
    service.routeCache_.preWarm(service.cityMap);

    std::cerr << "[RideMate] Active algorithm: "
              << service.currentAlgorithm() << "\n";

    // 4. Command router
    CommandRouter router(service, "data");

    std::cerr << "[RideMate] Ready. Waiting for commands...\n";

    // 5. JSON command loop
    while (router.run()) { /* process one command per iteration */ }

    // 6. Flush final state to disk
    service.saveAll();
    std::cerr << "[RideMate] Shutdown complete.\n";

    return 0;
}
