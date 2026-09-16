#include <unistd.h>

#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <random>
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

const int minPrice = 0;
const int maxPrice = 1000;

template <template <typename> class Container>
void bench(int cpu1, int cpu2, OrderBook& book) {
    const size_t queueSize = 65536;   // 2^16
    const int64_t iters = 100000000;  // 100 Million
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> distr(minPrice, maxPrice);
    std::uniform_real_distribution<> BidAsk(minPrice, maxPrice);

    Container<Order*> q(queueSize);
    auto t = std::thread([&] {
        pinThread(cpu1);
        while (true) {
            Order* order;
            if (!q.pop(order)) {
                if (q.isDone()) {
                    if (!q.pop(order)) break;
                } else {
                    continue;
                }
            }
            book.addOrder(order);
        }
    });

    pinThread(cpu2);
    int randomPrice, bidAskValue;
    bool is_buy;

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iters; ++i) {
        randomPrice = (int)distr(gen);
        bidAskValue = (int)BidAsk(gen);
        is_buy = (bidAskValue & 1) == 1;
        Order* o =
            createOrder(randomPrice, (bidAskValue % maxPrice) + 1, is_buy);
        while (!q.push(o)) {
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
        OrderBook book("Apple", minPrice, maxPrice, 1);

        if (strcmp(argv[1], "-mq") == 0) {
            std::cout << "\n--- using Mutex Queue ---\n";
            bench<MutexQueue>(cpu1, cpu2, std::ref(book));
        } else if (strcmp(argv[1], "-rb") == 0) {
            std::cout << "\n--- using Ring Buffer ---\n";
            bench<RingBuffer>(cpu1, cpu2, std::ref(book));
        } else {
            std::cout << "Invalid usage of arguments!\n";
            std::cout << "Usage: " << argv[0] << "(-mq | -rb) cpu1 cpu2\n";
            return EXIT_FAILURE;
        }
    } else {
        std::cout << "Usage: " << argv[0] << "(-mq | -rb) cpu1 cpu2\n";
        return EXIT_FAILURE;
    }

    return 0;
}
