CXX = g++
CXXFLAGS_COMMON = -std=c++17 -Wall -Wextra -Iinclude -pthread
DEBUG_FLAGS   = -g -O0 -fsanitize=address,undefined
# DEBUG_FLAGS   = -g -O0 
TSAN_FLAGS    = -g -O1 -fsanitize=thread
RELEASE_FLAGS = -O3 -DNDEBUG

.PHONY: all debug tsan release clean

all: debug tsan release

SRC = $(wildcard src/*.cpp)

debug:
	$(CXX) $(CXXFLAGS_COMMON) $(DEBUG_FLAGS) test/fuzz_test.cpp $(SRC) -o build/fuzz_debug

tsan:
	$(CXX) $(CXXFLAGS_COMMON) $(TSAN_FLAGS) test/spsc_test.cpp -o build/spsc_tsan

release:
	$(CXX) $(CXXFLAGS_COMMON) $(RELEASE_FLAGS) bench/latency_bench.cpp $(SRC) -o build/bench_release

clean:
	rm -rf build/*
