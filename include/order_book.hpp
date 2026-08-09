#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "order.hpp"

struct PriceLevel {
    Order* head;
    Order* tail;
    uint32_t depth;

    PriceLevel() : head(nullptr), tail(nullptr), depth(0) {}
};

struct Trade {
    OrderId restingOrderId;
    OrderId aggressorOrderId;
    Price price;
    Quantity qty;

    std::string to_string() const;
};

class OrderBook {
   private:
    std::string name;
    // std::vector<PriceLevel> levels_;
    std::unordered_map<OrderId, Order*> lookup_;
    std::vector<Trade> trades_;
    Price maxPrice;
    Price minPrice;
    int tickSize;
    int maxTicks;
    // Price bestAsk;
    // Price bestBid;
    void restOrder(Order* order);
    void matchBuy(Order* incoming);
    void matchSell(Order* incoming);
    void removeFromBook(Order* order);
    int priceToIndex(Price price) const;
    Price indexToPrice(int index) const;

   public:
    std::vector<PriceLevel> levels_;
    Price bestAsk;
    Price bestBid;
    OrderBook(std::string name, Price minPrice, Price maxPrice, int tickSize);
    ~OrderBook();
    void addOrder(Order* order);
    bool cancelOrder(OrderId id);
    void assertInvariants() const;
    const std::vector<Trade>& getTrades() const;
};
