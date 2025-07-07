#pragma once
#include <vector>
#include <memory>
#include "order_book.h"

class StrategyEngine {
public:
    StrategyEngine();
    std::vector<std::shared_ptr<Order>> generate_signals(const std::vector<std::shared_ptr<Order>>& market_orders);
}; 