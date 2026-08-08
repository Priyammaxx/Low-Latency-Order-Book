#include <random>

#include "order.hpp"
#include "order_book.hpp"

int main() {
    OrderBook orderBook("Apple", 0, 1000, 1.0f);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> distr(1, 20);
    std::uniform_real_distribution<> BidAsk(0, 1);

    for (int i = 0; i < 100; i++) {
        int randomPrice = distr(gen);
        bool is_buy = BidAsk(gen) == 1;
        Order* order = createOrder(randomPrice, 20, is_buy);
        orderBook.addOrder(order);
    }
    for (int i = 3; i < 23; i++) {
        orderBook.cancelOrder(i);
    }
    return 0;
}
