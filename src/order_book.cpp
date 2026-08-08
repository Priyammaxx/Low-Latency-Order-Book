#include "order_book.hpp"

#include <cmath>

OrderBook::OrderBook(std::string name, int minPrice, int maxPrice,
                     float tickSize) {
    this->name = name;
    this->minPrice = minPrice;
    this->maxPrice = maxPrice;
    this->tickSize = tickSize;
    this->maxTicks =
        static_cast<int>(std::ceil((float)(maxPrice - minPrice) / tickSize));
    levels_.resize(this->maxTicks);
}

OrderBook::~OrderBook() {
    for (auto& [id, orderPtr] : lookup_) {
        delete orderPtr;
    }
    lookup_.clear();
}

void OrderBook::addOrder(Order* order) {
    Price price = order->price;
    bool is_buy = order->is_buy;
    int index =
        static_cast<int>(std::ceil((float)(price - minPrice) / tickSize));
    PriceLevel* priceLevel = &levels_[index];

    if (is_buy) {
        if (priceLevel->bidCnt == 0) {
            priceLevel->bidHead = priceLevel->bidTail = order;
        } else {
            priceLevel->bidTail->next = order;
            order->prev = priceLevel->bidTail;
            priceLevel->bidTail = order;
        }
        priceLevel->bidCnt++;
    } else {
        if (priceLevel->askCnt == 0) {
            priceLevel->askHead = priceLevel->askTail = order;
        } else {
            priceLevel->askTail->next = order;
            order->prev = priceLevel->askTail;
            priceLevel->askTail = order;
        }
        priceLevel->askCnt++;
    }
    lookup_[order->id] = order;
}

bool OrderBook::cancelOrder(OrderId id) {
    auto it = lookup_.find(id);
    if (it == lookup_.end()) {
        return false;
    }

    Order* curOrder = it->second;

    int index = static_cast<int>(
        std::ceil((float)(curOrder->price - minPrice) / tickSize));
    PriceLevel* priceLevel = &levels_[index];

    if (curOrder->is_buy) {
        if (curOrder == priceLevel->bidHead)
            priceLevel->bidHead = priceLevel->bidHead->next;
        if (curOrder == priceLevel->bidTail)
            priceLevel->bidTail = priceLevel->bidTail->prev;
        priceLevel->bidCnt--;
    } else {
        if (curOrder == priceLevel->askHead)
            priceLevel->askHead = priceLevel->askHead->next;
        if (curOrder == priceLevel->askTail)
            priceLevel->askTail = priceLevel->askTail->prev;
        priceLevel->askCnt--;
    }

    if (curOrder->prev != nullptr) {
        curOrder->prev->next = curOrder->next;
    }
    if (curOrder->next != nullptr) {
        curOrder->next->prev = curOrder->prev;
    }

    curOrder->prev = nullptr;
    curOrder->next = nullptr;

    lookup_.erase(it);
    delete curOrder;

    return true;
}
