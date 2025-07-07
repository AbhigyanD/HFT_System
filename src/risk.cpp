#include "risk.h"

RiskManager::RiskManager() {}

std::vector<std::shared_ptr<Order>> RiskManager::filter_orders(const std::vector<std::shared_ptr<Order>>& orders) {
    // For demonstration, pass all orders through
    return orders;
} 