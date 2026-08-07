#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

int maxPrice = 10000;
int minPrice = 0;
float tickSize = 1.0f;
int maxTicks =
    static_cast<int>(std::ceil((float)(maxPrice - minPrice) / tickSize)) + 1;

// not thread safe yet
int global_id = 0;

struct Order {
    uint64_t id;         // 8 bytes
    bool is_buy;         // 1 byte, padding of 7 bytes
    uint32_t price;      // 4 bytes
    uint32_t qty;        // 4 bytes
    uint64_t timestamp;  // 8 bytes

    Order* prev;  // 8 bytes
    Order* next;  // 8 bytes, 64-bit system
};

struct PriceLevel {
    Order* head;
    Order* tail;
    uint32_t cnt;  // padding of 4 bytes
};

std::vector<PriceLevel> bids(maxTicks);  // indexed by price
std::vector<PriceLevel> asks(maxTicks);

Order* createOrder(uint32_t price, uint32_t qty, bool is_buy) {
    uint64_t timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    Order* order = new Order;
    order->id = global_id;
    order->is_buy = is_buy;
    order->price = price;
    order->qty = qty;
    order->timestamp = timestamp;
    order->prev = nullptr;
    order->next = nullptr;

    global_id++;
    return order;
}

void addOrder(Order* order) {
    uint32_t price = order->price;
    bool is_buy = order->is_buy;
}

int main() {
    int x;
    std::cout << "Align of Order: " << alignof(Order) << '\n';
    std::cout << "Size of Order: " << sizeof(Order) << '\n';
    std::cout << "Align of PriceLevel: " << alignof(PriceLevel) << '\n';
    std::cout << "Size of PriceLevel: " << sizeof(PriceLevel) << '\n';
    return 0;
}
