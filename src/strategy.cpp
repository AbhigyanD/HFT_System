#include "strategy.h"

StrategyEngine::StrategyEngine() {}

std::vector<std::shared_ptr<Order>> StrategyEngine::generate_signals(const std::vector<std::shared_ptr<Order>>& market_orders) {
    // For demonstration, just return the incoming market orders as signals
    return market_orders;
} 