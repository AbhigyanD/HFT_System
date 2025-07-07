#include "order_book.h"
#include <algorithm>

Order::Order(OrderId id, OrderSide s, Price p, Quantity q, OrderType t)
    : order_id(id), timestamp(std::chrono::steady_clock::now()),
      side(s), price(p), quantity(q), type(t) {}

TradeEvent::TradeEvent(OrderId buy_id, OrderId sell_id, Price p, Quantity q)
    : buy_order_id(buy_id), sell_order_id(sell_id), price(p), quantity(q),
      timestamp(std::chrono::steady_clock::now()) {}

OrderBookLevel::OrderBookLevel(Price price) : price_(price), total_quantity_(0) {}

void OrderBookLevel::add_order(std::shared_ptr<Order> order) {
    orders_.push(order);
    total_quantity_ += order->quantity;
}

std::shared_ptr<Order> OrderBookLevel::get_front_order() {
    return orders_.empty() ? nullptr : orders_.front();
}

void OrderBookLevel::remove_front_order() {
    if (!orders_.empty()) {
        total_quantity_ -= orders_.front()->quantity;
        orders_.pop();
    }
}

bool OrderBookLevel::remove_order(OrderId order_id) {
    std::queue<std::shared_ptr<Order>> temp_queue;
    bool found = false;
    while (!orders_.empty()) {
        auto order = orders_.front();
        orders_.pop();
        if (order->order_id == order_id) {
            total_quantity_ -= order->quantity;
            found = true;
        } else {
            temp_queue.push(order);
        }
    }
    orders_ = std::move(temp_queue);
    return found;
}

Price OrderBookLevel::get_price() const { return price_; }
Quantity OrderBookLevel::get_total_quantity() const { return total_quantity_; }
bool OrderBookLevel::is_empty() const { return orders_.empty(); }

OrderBookSide::OrderBookSide(bool is_bid) : is_bid_side_(is_bid) {}

void OrderBookSide::add_order(std::shared_ptr<Order> order) {
    Price price = order->price;
    if (levels_.find(price) == levels_.end()) {
        levels_[price] = std::make_unique<OrderBookLevel>(price);
    }
    levels_[price]->add_order(order);
}

std::shared_ptr<Order> OrderBookSide::get_best_order() {
    if (levels_.empty()) return nullptr;
    if (is_bid_side_) {
        auto rit = levels_.rbegin();
        return rit->second->get_front_order();
    } else {
        auto it = levels_.begin();
        return it->second->get_front_order();
    }
}

void OrderBookSide::remove_best_order() {
    if (levels_.empty()) return;
    if (is_bid_side_) {
        auto best_it = levels_.rbegin();
        best_it->second->remove_front_order();
        if (best_it->second->is_empty()) {
            levels_.erase(std::next(best_it).base());
        }
    } else {
        auto best_it = levels_.begin();
        best_it->second->remove_front_order();
        if (best_it->second->is_empty()) {
            levels_.erase(best_it);
        }
    }
}

bool OrderBookSide::remove_order(OrderId order_id, Price price) {
    auto it = levels_.find(price);
    if (it == levels_.end()) return false;
    bool removed = it->second->remove_order(order_id);
    if (removed && it->second->is_empty()) {
        levels_.erase(it);
    }
    return removed;
}

Price OrderBookSide::get_best_price() const {
    if (levels_.empty()) return 0;
    if (is_bid_side_) {
        auto best_it = levels_.rbegin();
        return best_it->second->get_price();
    } else {
        auto best_it = levels_.begin();
        return best_it->second->get_price();
    }
}
bool OrderBookSide::is_empty() const { return levels_.empty(); } 