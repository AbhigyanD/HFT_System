#pragma once
#include <vector>
#include <memory>
#include <deque>
#include <string>
#include "order_book.h"
#include "indicators.h"

enum class SignalType {
    BUY,
    SELL,
    HOLD
};

struct StrategyConfig {
    double momentum_threshold = 0.3;      // Minimum momentum score to trigger signal
    double rsi_oversold = 30.0;           // RSI level for oversold condition
    double rsi_overbought = 70.0;         // RSI level for overbought condition
    size_t short_period = 10;             // Short-term moving average period
    size_t long_period = 30;              // Long-term moving average period
    size_t rsi_period = 14;               // RSI calculation period
    double position_size = 100.0;         // Default position size
    double stop_loss_pct = 2.0;           // Stop loss percentage
    double take_profit_pct = 5.0;         // Take profit percentage
};

struct Signal {
    SignalType type;
    double price;
    double quantity;
    std::string reason;
    double confidence;
};

class StrategyEngine {
private:
    StrategyConfig config;
    std::deque<double> price_history;
    std::deque<double> volume_history;
    double last_signal_price = 0.0;
    bool in_position = false;
    double entry_price = 0.0;
    SignalType last_signal_type_ = SignalType::HOLD;
    std::string last_signal_reason_;
    double last_signal_confidence_ = 0.0;
    double last_signal_pnl_pct_ = 0.0;

    Signal generate_momentum_signal(double current_price, double current_volume);
    double calculate_signal_confidence(double current_price);
    std::string generate_signal_reason(double current_price);

public:
    StrategyEngine();
    StrategyEngine(const StrategyConfig& config);
    
    std::vector<std::shared_ptr<Order>> generate_signals(const std::vector<std::shared_ptr<Order>>& market_orders);
    
    // Strategy configuration
    void set_config(const StrategyConfig& config);
    StrategyConfig get_config() const;
    
    // Strategy state
    bool is_in_position() const { return in_position; }
    double get_entry_price() const { return entry_price; }
    size_t get_price_history_size() const { return price_history.size(); }
    
    // Performance tracking
    void reset_position() { in_position = false; entry_price = 0.0; }

    // Last signal metadata for logging (set by generate_signals when order is produced)
    SignalType get_last_signal_type() const { return last_signal_type_; }
    const std::string& get_last_signal_reason() const { return last_signal_reason_; }
    double get_last_signal_confidence() const { return last_signal_confidence_; }
    double get_last_signal_pnl_pct() const { return last_signal_pnl_pct_; }
}; 