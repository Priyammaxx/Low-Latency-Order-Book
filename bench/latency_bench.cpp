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
// track latency latencies from 0 ns to 100,000 ns
std::vector<uint64_t> serviceLatencyHist(100001, 0);
int serviceLatencyOutlierCnt = 0;
// measured in microseconds
std::vector<uint64_t> queueLatencyHist(100001, 0);
int queueLatencyOutlierCnt = 0;

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

const int64_t iters = 100000000;  // 100 Million
template <template <typename> class Container>
void bench(int cpu1, int cpu2, OrderBook& book) {
    const size_t queueSize = 65536;  // 2^16
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
            uint64_t dequeue_ns = nowNs();
            uint64_t dequeue_ms = nowMs();
            uint64_t enqueue_ms = order->timestamp;

            book.addOrder(order);
            uint64_t end_ns = nowNs();

            uint64_t queueLatency = dequeue_ms - enqueue_ms;
            uint64_t serviceLatency = end_ns - dequeue_ns;
            if (serviceLatency < serviceLatencyHist.size()) {
                serviceLatencyHist[serviceLatency]++;
            } else {
                serviceLatencyOutlierCnt++;
            }
            if (queueLatency < queueLatencyHist.size()) {
                queueLatencyHist[queueLatency]++;
            } else {
                queueLatencyOutlierCnt++;
            }
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

    auto stop = std::chrono::steady_clock::now();
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
            std::cout << "Usage: " << argv[0] << " (-mq | -rb) cpu1 cpu2\n";
            return EXIT_FAILURE;
        }
    } else {
        std::cout << "Usage: " << argv[0] << "(-mq | -rb) cpu1 cpu2\n";
        return EXIT_FAILURE;
    }
    uint64_t p50{};
    uint64_t p99{};
    uint64_t p999{};
    uint64_t p50Num = iters * 0.50;
    uint64_t p99Num = iters * 0.99;
    uint64_t p999Num = iters * 0.999;
    uint64_t orderCount = 0;

    for (uint64_t i = 0; i < serviceLatencyHist.size(); i++) {
        if (p50Num >= orderCount &&
            p50Num <= (orderCount + serviceLatencyHist[i])) {
            p50 = i;
        }
        if (p99Num >= orderCount &&
            p99Num <= (orderCount + serviceLatencyHist[i])) {
            p99 = i;
        }
        if (p999Num >= orderCount &&
            p999Num <= (orderCount + serviceLatencyHist[i])) {
            p999 = i;
        }
        orderCount += serviceLatencyHist[i];
    }

    std::cout << "| Metric | Service Latency (ns) |\n";
    std::cout << "|---|---|\n";
    std::cout << "| p50  | " << p50 << " |\n";
    std::cout << "| p99  | " << p99 << " |\n";
    std::cout << "| p999 | " << p999 << " |\n";

    std::cout << "\nNumber of times Service Latency greater than 100000 ns: "
              << serviceLatencyOutlierCnt << '\n';

    p999 = p99 = p50 = orderCount = 0;
    for (uint64_t i = 0; i < queueLatencyHist.size(); i++) {
        if (p50Num >= orderCount &&
            p50Num <= (orderCount + queueLatencyHist[i])) {
            p50 = i;
        }
        if (p99Num >= orderCount &&
            p99Num <= (orderCount + queueLatencyHist[i])) {
            p99 = i;
        }
        if (p999Num >= orderCount &&
            p999Num <= (orderCount + queueLatencyHist[i])) {
            p999 = i;
        }
        orderCount += queueLatencyHist[i];
    }

    std::cout << "\n| Metric | Queue Latency (micro s) |\n";
    std::cout << "|---|---|\n";
    std::cout << "| p50  | " << p50 << " |\n";
    std::cout << "| p99  | " << p99 << " |\n";
    std::cout << "| p999 | " << p999 << " |\n";

    std::cout << "\nNumber of times Queue Latency greater than 100000 micro s: "
              << queueLatencyOutlierCnt << '\n';
    return 0;
}
