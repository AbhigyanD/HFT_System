#pragma once
#include <cstdint>
#include <chrono>
#include <memory>
#include <queue>
#include <map>
#include <mutex>
#include <unordered_map>

// Core Types and Enums

enum class OrderSide : uint8_t { BUY = 0, SELL = 1 };
enum class OrderType : uint8_t { LIMIT = 0, MARKET = 1 };
using OrderId = uint64_t;
using Price = uint64_t;
using Quantity = uint64_t;
using Timestamp = std::chrono::steady_clock::time_point;

struct Order {
    OrderId order_id;
    Timestamp timestamp;
    OrderSide side;
    Price price;
    Quantity quantity;
    OrderType type;
    Order() : order_id(0), timestamp(std::chrono::steady_clock::now()), side(OrderSide::BUY), price(0), quantity(0), type(OrderType::LIMIT) {}
    Order(OrderId id, OrderSide s, Price p, Quantity q, OrderType t);
};

struct TradeEvent {
    OrderId buy_order_id;
    OrderId sell_order_id;
    Price price;
    Quantity quantity;
    Timestamp timestamp;
    TradeEvent(OrderId buy_id, OrderId sell_id, Price p, Quantity q);
};

class OrderBookLevel {
public:
    explicit OrderBookLevel(Price price);
    void add_order(std::shared_ptr<Order> order);
    std::shared_ptr<Order> get_front_order();
    void remove_front_order();
    bool remove_order(OrderId order_id);
    Price get_price() const;
    Quantity get_total_quantity() const;
    bool is_empty() const;
private:
    Price price_;
    std::queue<std::shared_ptr<Order>> orders_;
    Quantity total_quantity_;
};

class OrderBookSide {
public:
    explicit OrderBookSide(bool is_bid);
    void add_order(std::shared_ptr<Order> order);
    std::shared_ptr<Order> get_best_order();
    void remove_best_order();
    bool remove_order(OrderId order_id, Price price);
    Price get_best_price() const;
    bool is_empty() const;
private:
    std::map<Price, std::unique_ptr<OrderBookLevel>> levels_;
    bool is_bid_side_;
}; 