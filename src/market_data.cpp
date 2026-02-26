#include "market_data.h"
#include <chrono>

MarketData::MarketData() {}
MarketData::~MarketData() { stop(); }

void MarketData::start(MarketDataCallback callback) {
    running_ = true;
    feed_thread_ = std::thread(&MarketData::feed_loop, this, callback);
}

void MarketData::stop() {
    running_ = false;
    if (feed_thread_.joinable()) feed_thread_.join();
}

void MarketData::feed_loop(MarketDataCallback callback) {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> side_dist(0, 1);
    std::uniform_real_distribution<double> price_dist(99.0, 101.0);
    std::uniform_int_distribution<int> qty_dist(1, 10);
    std::uniform_int_distribution<int> type_dist(0, 1);
    OrderId next_id = 1;
    while (running_) {
        std::vector<std::shared_ptr<Order>> orders;
        for (int i = 0; i < 10; ++i) {
            auto order = std::make_shared<Order>();
            order->order_id = next_id++;
            order->side = side_dist(rng) == 0 ? OrderSide::BUY : OrderSide::SELL;
            order->price = static_cast<Price>(price_dist(rng) * 100.0);
            order->quantity = static_cast<Quantity>(qty_dist(rng));
            order->type = type_dist(rng) == 0 ? OrderType::LIMIT : OrderType::MARKET;
            orders.push_back(order);
        }
        callback(orders);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
} 