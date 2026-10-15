#include "mean_reversion_strategy.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <chrono>

MeanReversionEngine::MeanReversionEngine() : config_() {}

MeanReversionEngine::MeanReversionEngine(const MeanReversionConfig& config) : config_(config) {}

void MeanReversionEngine::set_config(const MeanReversionConfig& config) {
    config_ = config;
}

const MeanReversionConfig& MeanReversionEngine::get_config() const {
    return config_;
}

std::vector<std::shared_ptr<Order>> MeanReversionEngine::generate_signals(
    const std::vector<std::shared_ptr<Order>>& market_orders) {
    last_signal_type_ = SignalType::HOLD;
    std::vector<std::shared_ptr<Order>> out;

    for (const auto& order : market_orders) {
        if (order->type != OrderType::MARKET) continue;
        double price = static_cast<double>(order->price) / 100.0;
        price_history_.push_back(price);
        if (price_history_.size() > 2000) {
            price_history_.pop_front();
        }
    }

    if (price_history_.size() < config_.lookback_period) return out;

    double price = price_history_.back();
    double sma = Indicators::simple_moving_average(price_history_, config_.lookback_period);
    double deviation_pct = (sma > 0) ? (price - sma) / sma : 0.0;

    if (in_position_) {
        double pnl_pct = (price - entry_price_) / entry_price_ * 100.0;
        if (pnl_pct <= -config_.stop_loss_pct) {
            last_signal_type_ = SignalType::SELL;
            last_signal_reason_ = "Stop loss (" + std::to_string(pnl_pct) + "%)";
            last_signal_pnl_pct_ = pnl_pct;
            last_signal_confidence_ = 1.0;
        } else if (pnl_pct >= config_.take_profit_pct) {
            last_signal_type_ = SignalType::SELL;
            last_signal_reason_ = "Take profit (" + std::to_string(pnl_pct) + "%)";
            last_signal_pnl_pct_ = pnl_pct;
            last_signal_confidence_ = 1.0;
        } else if (deviation_pct >= config_.exit_threshold_pct) {
            last_signal_type_ = SignalType::SELL;
            last_signal_reason_ = "Mean reversion exit (price " + std::to_string(deviation_pct * 100) + "% above SMA)";
            last_signal_pnl_pct_ = pnl_pct;
            last_signal_confidence_ = std::min(std::abs(deviation_pct) / 0.02, 1.0);
        } else {
            return out;
        }
    } else {
        if (deviation_pct <= -config_.entry_threshold_pct) {
            last_signal_type_ = SignalType::BUY;
            last_signal_reason_ = "Oversold vs SMA (" + std::to_string(deviation_pct * 100) + "% below)";
            last_signal_pnl_pct_ = 0;
            last_signal_confidence_ = std::min(std::abs(deviation_pct) / 0.02, 1.0);
        } else {
            return out;
        }
    }

    auto order = std::make_shared<Order>();
    order->order_id = static_cast<OrderId>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
    order->side = (last_signal_type_ == SignalType::BUY) ? OrderSide::BUY : OrderSide::SELL;
    order->price = static_cast<Price>(price * 100.0);
    order->quantity = static_cast<Quantity>(config_.position_size);
    order->type = OrderType::MARKET;
    order->timestamp = std::chrono::steady_clock::now();
    out.push_back(order);

    if (last_signal_type_ == SignalType::BUY) {
        in_position_ = true;
        entry_price_ = price;
    } else {
        in_position_ = false;
        entry_price_ = 0.0;
    }
    return out;
}
