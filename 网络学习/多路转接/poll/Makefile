CXX ?= g++
CPPFLAGS ?=
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -g
LDFLAGS ?=
LDLIBS ?= -pthread

# 默认编译 Main.cc；使用其他文件名时可运行 make SOURCE=文件名.cc。
SOURCE ?= Main.cc
TARGET := poll_server
HEADERS := $(wildcard *.hpp)

.PHONY: all clean

ifneq ($(wildcard $(SOURCE)),)
all: $(TARGET)

$(TARGET): $(SOURCE) $(HEADERS) Makefile
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(SOURCE) $(LDFLAGS) $(LDLIBS) -o $@
else
all:
	@echo "未找到 $(SOURCE)，写好入口文件后再运行 make。"
	@echo "若入口文件不是 Main.cc，可用 make SOURCE=文件名.cc。"
endif

clean:
	rm -f $(TARGET)
