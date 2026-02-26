#include "risk.h"

RiskManager::RiskManager() : config_() {}

RiskManager::RiskManager(const RiskConfig& config) : config_(config) {}

void RiskManager::set_config(const RiskConfig& config) {
    config_ = config;
}

const RiskConfig& RiskManager::get_config() const {
    return config_;
}

std::vector<std::shared_ptr<Order>> RiskManager::filter_orders(
    const std::vector<std::shared_ptr<Order>>& orders) {
    std::vector<std::shared_ptr<Order>> out;
    out.reserve(orders.size());

    for (const auto& order : orders) {
        if (config_.max_order_quantity != 0 && order->quantity > config_.max_order_quantity) {
            ++orders_rejected_;
            continue;
        }
        if (config_.max_notional_per_order != 0) {
            uint64_t notional = order->price * order->quantity;
            if (notional > config_.max_notional_per_order) {
                ++orders_rejected_;
                continue;
            }
        }
        if (config_.max_orders_per_batch != 0 && out.size() >= config_.max_orders_per_batch) {
            ++orders_rejected_;
            continue;
        }
        if (config_.max_daily_volume != 0) {
            if (daily_volume_ + order->quantity > config_.max_daily_volume) {
                ++orders_rejected_;
                continue;
            }
            daily_volume_ += order->quantity;
        }
        out.push_back(order);
    }
    return out;
}
