#pragma once
#include <cstdint>

using OrderId = uint64_t;
using Price = uint32_t;
using Quantity = uint32_t;
using Time = uint64_t;

struct Order {
    OrderId id;      // 8 bytes
    bool is_buy;     // 1 byte, padding of 7 bytes
    Price price;     // 4 bytes
    Quantity qty;    // 4 bytes
    Time timestamp;  // 8 bytes

    Order* prev;  // 8 bytes
    Order* next;  // 8 bytes, 64-bit system
};

Order* createOrder(Price price, Quantity qty, bool is_buy);
