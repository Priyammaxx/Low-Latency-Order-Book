#include "order.hpp"

#include <atomic>
#include <chrono>

class IDGenerator {
   public:
    static int generateNextID() {
        static std::atomic<uint64_t> currentID{1};
        return currentID++;
    }
};

Order* createOrder(Price price, Quantity qty, bool is_buy) {
    Time timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch())
                         .count();
    Order* order = new Order{};
    order->id = IDGenerator::generateNextID();
    order->is_buy = is_buy;
    order->price = price;
    order->qty = qty;
    order->timestamp = timestamp;
    order->prev = nullptr;
    order->next = nullptr;

    return order;
}
