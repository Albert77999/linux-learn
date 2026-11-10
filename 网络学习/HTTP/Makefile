CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -g
LDLIBS ?= -pthread

TARGET := http_server
SOURCE := HttpServer.cc
HEADERS := $(wildcard *.hpp)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCE) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCE) -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(TARGET)
