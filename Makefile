CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -I.

TARGET   = ridemate
SRCS     = main.cpp core/RideService.cpp

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)

run: all
	./$(TARGET)
