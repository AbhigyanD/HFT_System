#pragma once
#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <random>
#include "order_book.h"

class MarketData {
public:
    using MarketDataCallback = std::function<void(const std::vector<std::shared_ptr<Order>>&)>;
    MarketData();
    ~MarketData();
    void start(MarketDataCallback callback);
    void stop();
private:
    std::thread feed_thread_;
    std::atomic<bool> running_{false};
    void feed_loop(MarketDataCallback callback);
}; 