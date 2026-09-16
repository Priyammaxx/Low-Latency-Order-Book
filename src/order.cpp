#include "order.hpp"

#include <atomic>

class IDGenerator {
   public:
    static int generateNextID() {
        static std::atomic<uint64_t> currentID{1};
        return currentID++;
    }
};

Order* createOrder(Price price, Quantity qty, bool is_buy) {
    Order* order = new Order{};
    order->id = IDGenerator::generateNextID();
    order->is_buy = is_buy;
    order->price = price;
    order->qty = qty;
    order->timestamp = nowMs();  // measured in microseconds
    order->prev = nullptr;
    order->next = nullptr;

    return order;
}
