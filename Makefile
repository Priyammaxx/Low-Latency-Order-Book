CXX := g++
CXXFLAGS_BASE := -std=c++17 -pthread
SRC := main.cpp

TARGET_DEBUG := debug_build
CXXFLAGS_DEBUG := $(CXXFLAGS_BASE) -g -O0 -Wall -Wextra -fsanitize=address,undefined

TARGET_RELEASE := release_build
CXXFLAGS_RELEASE := $(CXXFLAGS_BASE) -O3 -DNDEBUG

.PHONY: all debug release clean

all: debug release

debug: $(TARGET_DEBUG)
release: $(TARGET_RELEASE)

$(TARGET_DEBUG): $(SRC)
	$(CXX) $(CXXFLAGS_DEBUG) $< -o $@

$(TARGET_RELEASE): $(SRC)
	$(CXX) $(CXXFLAGS_RELEASE) $< -o $@

clean:
	rm -f $(TARGET_DEBUG) $(TARGET_RELEASE)
