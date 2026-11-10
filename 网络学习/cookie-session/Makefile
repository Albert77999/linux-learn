CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -g
LDLIBS ?= -pthread

TARGET := http_server
SOURCE := HttpServer.cc
HEADERS := $(wildcard *.hpp)
PORT ?= 18081

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(SOURCE) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCE) -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET) $(PORT)

# 本机临时端口联调，不占用你手动启动的 18081；测试后自动停止自己的服务。
test: $(TARGET)
	python3 tests/session_smoke.py
