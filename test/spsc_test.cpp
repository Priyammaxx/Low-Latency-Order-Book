#include <atomic>
#include <cassert>
#include <functional>
#include <iostream>
#include <random>
#include <thread>

#include "order.hpp"
#include "order_book.hpp"
#include "spsc_queue.hpp"

std::atomic<int> producedCount{0}, consumedCount{0};

const int minPrice = 1;
const int maxPrice = 40;

void producerFn(RingBuffer<Order*>& q, int numOrders) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> distr(minPrice, maxPrice);
    std::uniform_real_distribution<> BidAsk(0, 1000);

    for (int i = 0; i < numOrders; i++) {
        int randomPrice = (int)distr(gen);
        int bidAskValue = (int)BidAsk(gen);
        bool is_buy = (bidAskValue & 1) == 1;
        Order* o = createOrder(randomPrice, (bidAskValue % 50) + 1, is_buy);
        o->id = i;  // here id is working as sequence number
        while (!q.push(o)) {
            // spin-lock, necessary for now
        }
        producedCount++;
    }
    q.shutdown();
}

void consumerFn(RingBuffer<Order*>& q, OrderBook& book) {
    Order* o;
    uint64_t expectedId = 0;
    while (true) {
        if (!q.pop(o)) {
            if (q.isDone()) {
                if (!q.pop(o)) break;
            } else {
                continue;
            }
        }
        assert(o->id == expectedId++);
        book.addOrder(o);
        consumedCount++;
        Price bruteBid, bruteAsk;
        book.recomputeBestBidAskBruteForce(bruteBid, bruteAsk);
        assert(bruteAsk == book.bestAsk);
        assert(bruteBid == book.bestBid);
    }
}

int main() {
    const int numOrders = 200000;
    const size_t queueCapacity = 1024;

    RingBuffer<Order*> queue(queueCapacity);
    OrderBook book("Apple", 0, 1000, 1);

    std::thread producerThread(producerFn, std::ref(queue), numOrders);
    std::thread consumerThread(consumerFn, std::ref(queue), std::ref(book));

    producerThread.join();
    consumerThread.join();

    book.assertInvariants();

    for (int i = 0; i < 1000; i++) {
        if (book.cancelOrder(i))
            std::cout << "OrderID " << i << " cancelled.\n";
    }
    book.assertInvariants();

    const auto& trades = book.getTrades();
    for (const auto& trade : trades) {
        std::cout << trade.to_string() << '\n';
    }

    std::cout << trades.size() << "\n\n";

    for (int i = minPrice; i <= maxPrice; i++) {
        PriceLevel& level = book.levels_[i];
        if (level.head == nullptr) continue;
        std::cout << "Buy Order? " << level.head->is_buy << ", Price: " << i
                  << ", OrderID: " << level.head->id << '\n';
    }
    std::cout << "Best Bid: " << book.bestBid << ", Best Ask: " << book.bestAsk
              << std::endl;

    int finalProduced = producedCount.load();
    int finalConsumed = consumedCount.load();

    std::cout << "Produced: " << finalProduced << '\n';
    std::cout << "Consumed: " << finalConsumed << '\n';

    if (finalProduced == finalConsumed && finalProduced == numOrders) {
        std::cout << "SUCCESS: Produced and consumed counts match exactly.\n";
    } else {
        std::cout << "Failure: Data race or dropped orders detected.\n";
    }
    return 0;
}
