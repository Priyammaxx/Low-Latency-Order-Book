#include <iostream>
#include <random>

#include "order.hpp"
#include "order_book.hpp"

int main() {
    OrderBook orderBook("Apple", 0, 1000, 1);

    std::random_device rd;
    std::mt19937 gen(rd());
    int minPrice = 1;
    int maxPrice = 40;
    std::uniform_real_distribution<> distr(minPrice, maxPrice);
    std::uniform_real_distribution<> BidAsk(0, 1000);

    std::vector<int> buyOrders(maxPrice + 1, 0);
    std::vector<int> sellOrders(maxPrice + 1, 0);

    int maxOrders = 20;

    for (int i = 0; i < maxOrders; i++) {
        int randomPrice = (int)distr(gen);
        int bidAskValue = (int)BidAsk(gen);
        bool is_buy = (bidAskValue & 1) == 1;
        Order* order = createOrder(randomPrice, (bidAskValue % 50) + 1, is_buy);
        orderBook.addOrder(order);

        if (is_buy)
            buyOrders[randomPrice]++;
        else
            sellOrders[randomPrice]++;
    }
    // what happens if the resting order has been executed but cancel order is
    // called
    // for (int i = 3; i < 7; i++) {
    //     orderBook.cancelOrder(i);
    // }

    // std::cout << "Buy Order frequency:\n";
    // for (int i = minPrice; i <= maxPrice; i++) {
    //     std::cout << "(Price:" << i << ",frequency:" << buyOrders[i] << "),
    //     ";
    // }
    // std::cout << "\n\n";
    //
    // std::cout << "Sell Order frequency at prices:\n";
    // for (int i = minPrice; i <= maxPrice; i++) {
    //     std::cout << "(Price:" << i << ",frequency: " << sellOrders[i] << "),
    //     ";
    // }
    // std::cout << "\n\n";

    const auto& trades = orderBook.getTrades();

    for (const auto& trade : trades) {
        std::cout << trade.to_string() << '\n';
    }

    std::cout << trades.size() << "\n\n";

    for (int i = minPrice; i <= maxPrice; i++) {
        PriceLevel& level = orderBook.levels_[i];
        if (level.head == nullptr) continue;
        std::cout << "Buy Order? " << level.head->is_buy << ", Price: " << i
                  << ", OrderID: " << level.head->id << '\n';
    }
    std::cout << "Best Bid: " << orderBook.bestBid
              << ", Best Ask: " << orderBook.bestAsk << std::endl;
    return 0;
}
