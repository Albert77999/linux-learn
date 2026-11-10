CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Wpedantic

.PHONY: all clean
all: tcp_client

tcp_client: tcp_client.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f tcp_client
