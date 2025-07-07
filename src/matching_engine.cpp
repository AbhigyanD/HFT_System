#include "matching_engine.h"
#include <algorithm>

MatchingEngine::MatchingEngine() : bid_side_(true), ask_side_(false) {}

void MatchingEngine::add_order(std::shared_ptr<Order> order) {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    auto start_time = std::chrono::steady_clock::now();
    if (order->type == OrderType::MARKET) {
        process_market_order(order);
    } else {
        process_limit_order(order);
    }
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    total_processing_time_ns_ += duration.count();
    processed_orders_++;
}

bool MatchingEngine::cancel_order(OrderId order_id) {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    auto it = order_lookup_.find(order_id);
    if (it == order_lookup_.end()) {
        return false;
    }
    Price price = it->second.first;
    OrderSide side = it->second.second;
    bool removed = false;
    if (side == OrderSide::BUY) {
        removed = bid_side_.remove_order(order_id, price);
    } else {
        removed = ask_side_.remove_order(order_id, price);
    }
    if (removed) {
        order_lookup_.erase(it);
    }
    return removed;
}

std::vector<TradeEvent> MatchingEngine::get_trade_events() const {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    return trade_events_;
}

uint64_t MatchingEngine::get_processed_orders() const { return processed_orders_.load(); }
uint64_t MatchingEngine::get_matched_trades() const { return matched_trades_.load(); }
double MatchingEngine::get_average_processing_time_ns() const {
    uint64_t orders = processed_orders_.load();
    return orders > 0 ? static_cast<double>(total_processing_time_ns_.load()) / orders : 0.0;
}
std::pair<Price, Price> MatchingEngine::get_best_bid_ask() const {
    std::lock_guard<std::mutex> lock(engine_mutex_);
    return {bid_side_.get_best_price(), ask_side_.get_best_price()};
}

void MatchingEngine::process_market_order(std::shared_ptr<Order> order) {
    if (order->side == OrderSide::BUY) {
        match_order_against_side(order, ask_side_);
    } else {
        match_order_against_side(order, bid_side_);
    }
}

void MatchingEngine::process_limit_order(std::shared_ptr<Order> order) {
    if (order->side == OrderSide::BUY) {
        match_order_against_side(order, ask_side_);
        if (order->quantity > 0) {
            bid_side_.add_order(order);
            order_lookup_[order->order_id] = {order->price, order->side};
        }
    } else {
        match_order_against_side(order, bid_side_);
        if (order->quantity > 0) {
            ask_side_.add_order(order);
            order_lookup_[order->order_id] = {order->price, order->side};
        }
    }
}

void MatchingEngine::match_order_against_side(std::shared_ptr<Order> incoming_order, OrderBookSide& opposite_side) {
    while (incoming_order->quantity > 0 && !opposite_side.is_empty()) {
        auto resting_order = opposite_side.get_best_order();
        if (!resting_order) break;
        bool can_match = false;
        if (incoming_order->type == OrderType::MARKET) {
            can_match = true;
        } else {
            if (incoming_order->side == OrderSide::BUY) {
                can_match = (incoming_order->price >= resting_order->price);
            } else {
                can_match = (incoming_order->price <= resting_order->price);
            }
        }
        if (!can_match) break;
        Price trade_price = resting_order->price;
        Quantity trade_quantity = std::min(incoming_order->quantity, resting_order->quantity);
        OrderId buy_id = (incoming_order->side == OrderSide::BUY) ? 
                       incoming_order->order_id : resting_order->order_id;
        OrderId sell_id = (incoming_order->side == OrderSide::SELL) ? 
                        incoming_order->order_id : resting_order->order_id;
        trade_events_.emplace_back(buy_id, sell_id, trade_price, trade_quantity);
        matched_trades_++;
        incoming_order->quantity -= trade_quantity;
        resting_order->quantity -= trade_quantity;
        if (resting_order->quantity == 0) {
            opposite_side.remove_best_order();
            order_lookup_.erase(resting_order->order_id);
        }
    }
} 