FULL TEST FLOW (AFTER REGISTERING)-
1. Login as Alice (passenger)   → lands on dashboard.html
2. Select pickup + drop nodes
3. See fare previews for all 4 vehicle types
4. Click a vehicle → Request Ride
5. Open a new tab → Login as Bob (driver) → driver.html
6. Click "Assign Next Ride" → Bob gets assigned
7. Click "Complete Ride" → give a star rating
8. Check history.html on both accounts
9. Check map.html → see the city graph, run shortest path tool


COMMAND TO COMPILE AND RUN - 
cd ridemate 
make 
mkdir data 
cd shim 
npm install
cd ..
node shim/server.js
run - ridemate/frontend/index.html



