#include <unistd.h>

#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <thread>

#include "mutex_queue.hpp"
#include "order_book.hpp"
#include "spsc_queue.hpp"

const int numCores = sysconf(_SC_NPROCESSORS_ONLN);

void pinThread(int cpu) {
    if (cpu < 0 || cpu > numCores) {
        return;
    }
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) ==
        -1) {
        perror("pthread_setaffinity_no");
        exit(1);
    }
}

template <template <typename> class Container>
void bench(int cpu1, int cpu2) {
    const size_t queueSize = 65536;   // 2^16
    const int64_t iters = 100000000;  // 100 Million

    Container<int> q(queueSize);
    auto t = std::thread([&] {
        pinThread(cpu1);
        for (int i = 0; i < iters; ++i) {
            int val;
            while (!q.pop(val)) {
            }
            if (val != i) {
                throw std::runtime_error("");
            }
        }
        // while (true) {
        //      int val;
        //     if (!q.pop(val)) {
        //         if (q.isDone()) {
        //             if (!q.pop(val)) break;
        //         } else {
        //             continue;
        //         }
        //     }
        //     consumedCount++;
        // }
    });

    pinThread(cpu2);

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iters; ++i) {
        while (!q.push(i)) {
        }
    }
    q.shutdown();
    t.join();
    // while (producedCount.load(std::memory_order_relaxed) !=
    //        consumedCount.load(std::memory_order_relaxed)) {
    // }
    auto stop = std::chrono::steady_clock::now();
    // t.join();
    std::cout << iters * 1000000000 /
                     std::chrono::duration_cast<std::chrono::nanoseconds>(stop -
                                                                          start)
                         .count()
              << " ops/s" << std::endl;
}

int main(int argc, char* argv[]) {
    int cpu1 = -1;
    int cpu2 = -1;

    if (argc == 4) {
        cpu1 = std::stoi(argv[2]);
        cpu2 = std::stoi(argv[3]);
        std::cout << "Cpu1: " << cpu1 << ", Cpu2: " << cpu2 << "\n";
        if (strcmp(argv[1], "-mq") == 0) {
            bench<MutexQueue>(cpu1, cpu2);
        } else if (strcmp(argv[1], "-rb") == 0) {
            bench<RingBuffer>(cpu1, cpu2);
        } else {
            std::cout << "Usage: " << argv[0] << "(-mq | -rb) cpu1 cpu2\n";
            exit(1);
        }
    } else {
        std::cout << "Usage: " << argv[0] << "(-mq | -rb) cpu1 cpu2\n";
        exit(1);
    }

    return 0;
}
