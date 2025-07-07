#pragma once
#include "order_book.h"
#include <vector>
#include <mutex>
#include <atomic>

class MatchingEngine {
public:
    MatchingEngine();
    void add_order(std::shared_ptr<Order> order);
    bool cancel_order(OrderId order_id);
    std::vector<TradeEvent> get_trade_events() const;
    uint64_t get_processed_orders() const;
    uint64_t get_matched_trades() const;
    double get_average_processing_time_ns() const;
    std::pair<Price, Price> get_best_bid_ask() const;
private:
    OrderBookSide bid_side_;
    OrderBookSide ask_side_;
    std::unordered_map<OrderId, std::pair<Price, OrderSide>> order_lookup_;
    std::vector<TradeEvent> trade_events_;
    mutable std::mutex engine_mutex_;
    std::atomic<uint64_t> processed_orders_{0};
    std::atomic<uint64_t> matched_trades_{0};
    std::atomic<uint64_t> total_processing_time_ns_{0};
    void process_market_order(std::shared_ptr<Order> order);
    void process_limit_order(std::shared_ptr<Order> order);
    void match_order_against_side(std::shared_ptr<Order> incoming_order, OrderBookSide& opposite_side);
}; 