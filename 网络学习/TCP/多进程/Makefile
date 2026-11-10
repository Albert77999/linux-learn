CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2 -pthread

SERVER := tcp_server
CLIENT := tcp_client
IP ?= 127.0.0.1
PORT ?= 8080

.PHONY: all run-server run-client clean

all: $(SERVER) $(CLIENT)

$(SERVER): TcpServer.cc TcpServer.hpp InetAddr.hpp Common.hpp Log.hpp Mutex.hpp
	$(CXX) $(CXXFLAGS) TcpServer.cc -o $@

$(CLIENT): TcpClient.cc TcpClient.hpp InetAddr.hpp Common.hpp Log.hpp Mutex.hpp
	$(CXX) $(CXXFLAGS) TcpClient.cc -o $@

run-server: $(SERVER)
	./$(SERVER) $(PORT)

run-client: $(CLIENT)
	./$(CLIENT) $(IP) $(PORT)

clean:
	rm -f $(SERVER) $(CLIENT)
