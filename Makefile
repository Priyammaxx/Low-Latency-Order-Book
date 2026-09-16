CXX = g++
CXXFLAGS_COMMON = -std=c++17 -Wall -Wextra -Iinclude -pthread
DEBUG_FLAGS   = -g -O0 -fsanitize=address,undefined
TSAN_FLAGS    = -g -O1 -fsanitize=thread
BENCH_FLAGS = -O3 -DNDEBUG

.PHONY: all debug tsan release clean

all: debug tsan release

SRC = $(wildcard src/*.cpp)

debug:
	mkdir -p build
	$(CXX) $(CXXFLAGS_COMMON) $(DEBUG_FLAGS) test/fuzz_test.cpp $(SRC) -o build/fuzz_debug

tsan:
	mkdir -p build
	$(CXX) $(CXXFLAGS_COMMON) $(TSAN_FLAGS) test/spsc_test.cpp $(SRC) -o build/spsc_tsan

latency:
	mkdir -p build
	$(CXX) $(CXXFLAGS_COMMON) $(BENCH_FLAGS) bench/latency_bench.cpp $(SRC) -o build/latency_bench

perf:
	mkdir -p build
	$(CXX) $(CXXFLAGS_COMMON) $(BENCH_FLAGS) bench/perf_bench.cpp $(SRC) -o build/perf_bench

clean:
	rm -rf build/*
