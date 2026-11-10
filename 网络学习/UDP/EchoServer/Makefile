CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2 -pthread

SERVER := udp_server
CLIENT := udp_client

.PHONY: all clean

# 默认同时编译服务器和客户端。
all: $(SERVER) $(CLIENT)

$(SERVER): UdpServerMain.cc UdpServer.hpp InetAddr.hpp Common.hpp Log.hpp Mutex.hpp
	$(CXX) $(CXXFLAGS) UdpServerMain.cc -o $@

$(CLIENT): UdpClientMain.cc UdpClient.hpp InetAddr.hpp Common.hpp Log.hpp Mutex.hpp
	$(CXX) $(CXXFLAGS) UdpClientMain.cc -o $@

clean:
	rm -f $(SERVER) $(CLIENT)
