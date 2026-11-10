CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -g -pthread
LDFLAGS ?=
# Protocol.hpp 使用 JsonCpp，链接时需要带上该库。
LDLIBS ?= -pthread -ljsoncpp

SERVER := reactor_server
CLIENT := tcp_client
TARGETS := $(SERVER) $(CLIENT)
HEADERS := $(wildcard *.hpp)

.PHONY: all clean

all: $(TARGETS)

$(SERVER): Main.cc $(HEADERS) Makefile
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) Main.cc $(LDFLAGS) $(LDLIBS) -o $@

$(CLIENT): TcpClient.cc $(HEADERS) Makefile
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) TcpClient.cc $(LDFLAGS) $(LDLIBS) -o $@

clean:
	rm -f $(TARGETS)
