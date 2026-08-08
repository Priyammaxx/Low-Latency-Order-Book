#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "order.hpp"

struct PriceLevel {
    Order* bidHead;
    Order* bidTail;
    Order* askHead;
    Order* askTail;
    uint32_t bidCnt;
    uint32_t askCnt;

    PriceLevel()
        : bidHead(nullptr),
          bidTail(nullptr),
          askHead(nullptr),
          askTail(nullptr),
          bidCnt(0),
          askCnt(0) {}
};

class OrderBook {
   private:
    std::string name;
    std::vector<PriceLevel> levels_;
    std::unordered_map<OrderId, Order*> lookup_;
    int maxPrice;
    int minPrice;
    float tickSize;
    int maxTicks;

   public:
    OrderBook(std::string name, int minPrice, int maxPrice, float tickSize);
    ~OrderBook();
    void addOrder(Order* order);
    bool cancelOrder(OrderId id);
    void assertInvariants() const;
};
