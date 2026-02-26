#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include "order_book.h"

struct RiskConfig {
    uint64_t max_order_quantity = 0;      // 0 = no limit
    uint64_t max_notional_per_order = 0;  // 0 = no limit (price * qty in same units as Order)
    uint32_t max_orders_per_batch = 0;    // 0 = no limit
    uint64_t max_daily_volume = 0;        // 0 = no limit
    double max_position_pct = 0.0;        // 0 = no limit (e.g. 0.01 = 1% of book)
};

class RiskManager {
public:
    explicit RiskManager();
    explicit RiskManager(const RiskConfig& config);

    void set_config(const RiskConfig& config);
    const RiskConfig& get_config() const;

    std::vector<std::shared_ptr<Order>> filter_orders(
        const std::vector<std::shared_ptr<Order>>& orders);

    uint64_t get_orders_rejected() const { return orders_rejected_; }
    void reset_counters() { orders_rejected_ = 0; daily_volume_ = 0; }
    void reset_daily_volume() { daily_volume_ = 0; }

private:
    RiskConfig config_;
    uint64_t orders_rejected_ = 0;
    uint64_t daily_volume_ = 0;
};
