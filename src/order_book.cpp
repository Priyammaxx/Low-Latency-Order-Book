#include "order_book.hpp"

#include <cassert>
#include <cmath>
#include <string>

#include "order.hpp"

// ----------------------------------------
// --- Class functions ---

OrderBook::OrderBook(std::string name, Price minPrice, Price maxPrice,
                     int tickSize) {
    this->name = name;
    this->minPrice = minPrice;
    this->maxPrice = maxPrice;
    this->tickSize = tickSize;
    this->maxTicks = (maxPrice - minPrice) / tickSize + 1;
    this->bestAsk = maxPrice;
    this->bestBid = minPrice;
    levels_.resize(this->maxTicks);
}

OrderBook::~OrderBook() {
    for (auto& [id, orderPtr] : lookup_) {
        delete orderPtr;
    }
    lookup_.clear();
}

int OrderBook::priceToIndex(Price price) const {
    return (price - minPrice) / tickSize;
}

Price OrderBook::indexToPrice(int index) const {
    return minPrice + index * tickSize;
}

void OrderBook::restOrder(Order* order) {
    int index = priceToIndex(order->price);
    PriceLevel& level = levels_[index];

    if (level.depth == 0) {
        level.head = level.tail = order;
    } else {
        level.tail->next = order;
        order->prev = level.tail;
        level.tail = order;
    }

    if (order->is_buy) {
        bestBid = std::max(bestBid, order->price);
    } else {
        bestAsk = std::min(bestAsk, order->price);
    }

    level.depth++;

    lookup_[order->id] = order;
}

void OrderBook::matchBuy(Order* incoming) {
    int idx = priceToIndex(bestAsk);

    while (idx < maxTicks && indexToPrice(idx) <= incoming->price &&
           incoming->qty > 0) {
        PriceLevel& level = levels_[idx];

        while (level.head != nullptr && incoming->qty > 0) {
            Order* resting = level.head;
            Quantity tradeQty = std::min(incoming->qty, resting->qty);
            trades_.push_back(
                {resting->id, incoming->id, indexToPrice(idx), tradeQty});
            incoming->qty -= tradeQty;
            resting->qty -= tradeQty;
            if (resting->qty == 0) {
                removeFromBook(resting);
                delete resting;
            }
        }
        if (level.head == nullptr)
            idx++;
        else
            break;
    }
    while (idx < maxTicks && levels_[idx].head == nullptr) idx++;
    bestAsk = (idx < maxTicks) ? indexToPrice(idx) : maxPrice;
}

void OrderBook::matchSell(Order* incoming) {
    int idx = priceToIndex(bestBid);
    while (idx >= 0 && indexToPrice(idx) >= incoming->price &&
           incoming->qty > 0) {
        PriceLevel& level = levels_[idx];

        while (level.head != nullptr && incoming->qty > 0) {
            Order* resting = level.head;
            Quantity tradeQty = std::min(incoming->qty, resting->qty);
            trades_.push_back(
                {resting->id, incoming->id, indexToPrice(idx), tradeQty});
            incoming->qty -= tradeQty;
            resting->qty -= tradeQty;
            if (resting->qty == 0) {
                removeFromBook(resting);
                delete resting;
            }
        }
        if (level.head == nullptr)
            idx--;
        else
            break;
    }
    while (idx >= 0 && levels_[idx].head == nullptr) idx--;
    bestBid = (idx > 0) ? indexToPrice(idx) : minPrice;
}

void OrderBook::removeFromBook(Order* order) {
    int idx = priceToIndex(order->price);
    PriceLevel& level = levels_[idx];

    if (order == level.head) level.head = order->next;
    if (order == level.tail) level.tail = order->prev;
    level.depth--;

    if (order->prev) order->prev->next = order->next;
    if (order->next) order->next->prev = order->prev;
    order->prev = order->next = nullptr;

    lookup_.erase(order->id);
}

void OrderBook::addOrder(Order* order) {
    // Price-time-priority, FIFO
    if (order->is_buy) {
        matchBuy(order);
    } else {
        matchSell(order);
    }

    if (order->qty > 0) {
        restOrder(order);
    } else {
        delete order;
    }
}

bool OrderBook::cancelOrder(OrderId id) {
    auto it = lookup_.find(id);
    if (it == lookup_.end()) {
        return false;
    }
    Order* curOrder = it->second;
    bool aloneInLevel = curOrder->prev == nullptr && curOrder->next == nullptr;
    removeFromBook(curOrder);
    if (aloneInLevel) {
        if (curOrder->price == bestBid) {
            int idx = priceToIndex(bestBid);
            while (idx >= 0 && levels_[idx].head == nullptr) idx--;
            bestBid = (idx > 0) ? indexToPrice(idx) : minPrice;
        } else if (curOrder->price == bestAsk) {
            int idx = priceToIndex(bestAsk);
            while (idx < maxTicks && levels_[idx].head == nullptr) idx++;
            bestAsk = (idx < maxTicks) ? indexToPrice(idx) : maxPrice;
        }
    }
    delete curOrder;
    return true;
}

const std::vector<Trade>& OrderBook::getTrades() const { return trades_; }

// ----------------------------------------
// --- Utility and other functions ---

std::string Trade::to_string() const {
    return "Resting Order ID: " + std::to_string(restingOrderId) +
           ", Incoming Order ID: " + std::to_string(aggressorOrderId) +
           "\nExecuted at Price: " + std::to_string(price) +
           ", Quantity: " + std::to_string(qty);
}

void OrderBook::recomputeBestBidAskBruteForce(Price& bb, Price& ba) const {
    bb = minPrice;
    ba = maxPrice;
    for (int i = 0; i < maxTicks; i++) {
        if (levels_[i].head == nullptr) continue;
        if (levels_[i].head->is_buy)
            bb = std::max(bb, indexToPrice(i));
        else
            ba = std::min(ba, indexToPrice(i));
    }
}

void OrderBook::assertInvariants() const {
    for (const PriceLevel& level : levels_) {
        if (level.depth == 0) {
            assert(level.head == nullptr);
            assert(level.tail == nullptr);
        }
        Order* order = level.head;
        while (order != nullptr) {
            assert(order->qty > 0);
            order = order->next;
        }
    }
}
