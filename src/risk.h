#pragma once
#include <vector>
#include <memory>
#include "order_book.h"

class RiskManager {
public:
    RiskManager();
    std::vector<std::shared_ptr<Order>> filter_orders(const std::vector<std::shared_ptr<Order>>& orders);
}; 