#pragma once

#include <vector>
#include <memory>
#include <deque>
#include <string>
#include "order_book.h"
#include "indicators.h"

struct MeanReversionConfig {
    size_t lookback_period = 20;
    double entry_threshold_pct = 0.005;   // buy when price < SMA * (1 - this)
    double exit_threshold_pct = 0.005;     // sell when price > SMA * (1 + this)
    double position_size = 100.0;
    double stop_loss_pct = 2.0;
    double take_profit_pct = 4.0;
};

class MeanReversionEngine {
public:
    MeanReversionEngine();
    explicit MeanReversionEngine(const MeanReversionConfig& config);

    void set_config(const MeanReversionConfig& config);
    const MeanReversionConfig& get_config() const;

    std::vector<std::shared_ptr<Order>> generate_signals(
        const std::vector<std::shared_ptr<Order>>& market_orders);

    bool is_in_position() const { return in_position_; }
    double get_entry_price() const { return entry_price_; }
    size_t get_price_history_size() const { return price_history_.size(); }
    void reset_position() { in_position_ = false; entry_price_ = 0.0; }

    enum class SignalType { BUY, SELL, HOLD };
    SignalType get_last_signal_type() const { return last_signal_type_; }
    const std::string& get_last_signal_reason() const { return last_signal_reason_; }
    double get_last_signal_confidence() const { return last_signal_confidence_; }
    double get_last_signal_pnl_pct() const { return last_signal_pnl_pct_; }

private:
    MeanReversionConfig config_;
    std::deque<double> price_history_;
    bool in_position_ = false;
    double entry_price_ = 0.0;
    SignalType last_signal_type_ = SignalType::HOLD;
    std::string last_signal_reason_;
    double last_signal_confidence_ = 0.0;
    double last_signal_pnl_pct_ = 0.0;
};
