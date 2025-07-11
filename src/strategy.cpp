#include "strategy.h"
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>

StrategyEngine::StrategyEngine() : config() {}

StrategyEngine::StrategyEngine(const StrategyConfig& config) : config(config) {}

void StrategyEngine::set_config(const StrategyConfig& config) {
    this->config = config;
}

StrategyConfig StrategyEngine::get_config() const {
    return config;
}

std::vector<std::shared_ptr<Order>> StrategyEngine::generate_signals(const std::vector<std::shared_ptr<Order>>& market_orders) {
    std::vector<std::shared_ptr<Order>> strategy_orders;
    
    // Update price history from market orders
    for (const auto& order : market_orders) {
        if (order->type == OrderType::MARKET) {
            price_history.push_back(order->price);
            volume_history.push_back(order->quantity);
            
            // Keep only recent history (last 1000 data points)
            if (price_history.size() > 1000) {
                price_history.pop_front();
                volume_history.pop_front();
            }
        }
    }
    
    // Need sufficient price history to generate signals
    if (price_history.size() < config.long_period) {
        return strategy_orders;
    }
    
    double current_price = price_history.back();
    double current_volume = volume_history.back();
    
    // Generate momentum signal
    Signal signal = generate_momentum_signal(current_price, current_volume);
    
    // Convert signal to order if it's not HOLD
    if (signal.type != SignalType::HOLD) {
        auto order = std::make_shared<Order>();
        order->order_id = static_cast<OrderId>(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count());
        order->side = (signal.type == SignalType::BUY) ? OrderSide::BUY : OrderSide::SELL;
        order->price = static_cast<Price>(signal.price * 100); // Convert to integer price
        order->quantity = static_cast<Quantity>(signal.quantity);
        order->type = OrderType::MARKET; // Strategy orders are market orders
        order->timestamp = std::chrono::high_resolution_clock::now();
        
        strategy_orders.push_back(order);
        
        // Update position tracking and print signal info
        if (signal.type == SignalType::BUY && !in_position) {
            in_position = true;
            entry_price = signal.price;
            std::cout << "🟢 BUY Signal: " << signal.reason << " (Confidence: " 
                      << std::fixed << std::setprecision(2) << signal.confidence * 100 << "%)" << std::endl;
        } else if (signal.type == SignalType::SELL && in_position) {
            in_position = false;
            double pnl = ((signal.price - entry_price) / entry_price) * 100;
            std::cout << "🔴 SELL Signal: " << signal.reason << " (Confidence: " 
                      << std::fixed << std::setprecision(2) << signal.confidence * 100 
                      << "%, P&L: " << pnl << "%)" << std::endl;
            entry_price = 0.0;
        }
    }
    
    return strategy_orders;
}

Signal StrategyEngine::generate_momentum_signal(double current_price, double current_volume) {
    Signal signal;
    signal.price = current_price;
    signal.quantity = config.position_size;
    signal.confidence = calculate_signal_confidence(current_price);
    signal.reason = generate_signal_reason(current_price);
    
    // Calculate key indicators
    double momentum_score = Indicators::momentum_score(price_history, config.short_period, config.long_period);
    double rsi = Indicators::relative_strength_index(price_history, config.rsi_period);
    auto [macd_line, signal_line] = Indicators::macd(price_history, 12, 26, 9);
    double price_change = Indicators::price_change_percent(price_history, config.short_period);
    
    // Check for stop loss or take profit if in position
    if (in_position) {
        double pnl_pct = ((current_price - entry_price) / entry_price) * 100;
        
        // Stop loss
        if (pnl_pct <= -config.stop_loss_pct) {
            signal.type = SignalType::SELL;
            signal.reason = "Stop Loss triggered (" + std::to_string(pnl_pct) + "%)";
            return signal;
        }
        
        // Take profit
        if (pnl_pct >= config.take_profit_pct) {
            signal.type = SignalType::SELL;
            signal.reason = "Take Profit triggered (" + std::to_string(pnl_pct) + "%)";
            return signal;
        }
    }
    
    // Momentum strategy logic
    if (!in_position) {
        // BUY conditions
        bool strong_momentum = momentum_score > config.momentum_threshold;
        bool rsi_not_overbought = rsi < config.rsi_overbought;
        bool macd_bullish = macd_line > signal_line;
        bool price_above_ma = current_price > Indicators::simple_moving_average(price_history, config.short_period);
        
        if (strong_momentum && rsi_not_overbought && macd_bullish && price_above_ma) {
            signal.type = SignalType::BUY;
        } else {
            signal.type = SignalType::HOLD;
        }
    } else {
        // SELL conditions
        bool momentum_weakening = momentum_score < 0.0;
        bool rsi_overbought = rsi > config.rsi_overbought;
        bool macd_bearish = macd_line < signal_line;
        bool price_below_ma = current_price < Indicators::simple_moving_average(price_history, config.short_period);
        
        if (momentum_weakening || rsi_overbought || macd_bearish || price_below_ma) {
            signal.type = SignalType::SELL;
        } else {
            signal.type = SignalType::HOLD;
        }
    }
    
    return signal;
}

double StrategyEngine::calculate_signal_confidence(double current_price) {
    if (price_history.size() < config.long_period) return 0.0;
    
    double momentum_score = Indicators::momentum_score(price_history, config.short_period, config.long_period);
    double rsi = Indicators::relative_strength_index(price_history, config.rsi_period);
    auto [macd_line, signal_line] = Indicators::macd(price_history, 12, 26, 9);
    
    // Normalize indicators to [0, 1] range
    double momentum_norm = std::abs(momentum_score);
    double rsi_norm = (rsi > 50) ? (rsi - 50) / 50 : (50 - rsi) / 50;
    double macd_norm = std::abs(macd_line - signal_line) / std::max(std::abs(macd_line), 1.0);
    
    // Weighted average of confidence factors
    double confidence = (momentum_norm * 0.4 + rsi_norm * 0.3 + macd_norm * 0.3);
    return std::min(confidence, 1.0);
}

std::string StrategyEngine::generate_signal_reason(double current_price) {
    if (price_history.size() < config.long_period) return "Insufficient data";
    
    std::ostringstream reason;
    double momentum_score = Indicators::momentum_score(price_history, config.short_period, config.long_period);
    double rsi = Indicators::relative_strength_index(price_history, config.rsi_period);
    auto [macd_line, signal_line] = Indicators::macd(price_history, 12, 26, 9);
    double short_ma = Indicators::simple_moving_average(price_history, config.short_period);
    double long_ma = Indicators::simple_moving_average(price_history, config.long_period);
    
    reason << "Momentum: " << std::fixed << std::setprecision(2) << momentum_score
           << ", RSI: " << rsi
           << ", MACD: " << (macd_line > signal_line ? "Bullish" : "Bearish")
           << ", Price vs MA: " << (current_price > short_ma ? "Above" : "Below")
           << " (" << short_ma << " vs " << long_ma << ")";
    
    return reason.str();
} 