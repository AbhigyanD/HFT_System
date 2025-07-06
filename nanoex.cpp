#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <random>
#include <algorithm>

// ============================================================================
// Core Types and Enums
// ============================================================================

enum class OrderSide : uint8_t {
    BUY = 0,
    SELL = 1
};

enum class OrderType : uint8_t {
    LIMIT = 0,
    MARKET = 1
};

using OrderId = uint64_t;
using Price = uint64_t;  // Fixed-point price (scaled by 10000, e.g., $12.34 = 123400)
using Quantity = uint64_t;
using Timestamp = std::chrono::steady_clock::time_point;

// ============================================================================
// Order Structure
// ============================================================================

struct Order {
    OrderId order_id;
    Timestamp timestamp;
    OrderSide side;
    Price price;
    Quantity quantity;
    OrderType type;
    
    Order(OrderId id, OrderSide s, Price p, Quantity q, OrderType t)
        : order_id(id), timestamp(std::chrono::steady_clock::now()), 
          side(s), price(p), quantity(q), type(t) {}
};

// ============================================================================
// Trade Event Structure
// ============================================================================

struct TradeEvent {
    OrderId buy_order_id;
    OrderId sell_order_id;
    Price price;
    Quantity quantity;
    Timestamp timestamp;
    
    TradeEvent(OrderId buy_id, OrderId sell_id, Price p, Quantity q)
        : buy_order_id(buy_id), sell_order_id(sell_id), price(p), quantity(q),
          timestamp(std::chrono::steady_clock::now()) {}
};

// ============================================================================
// Order Book Level (Price Level Management)
// ============================================================================

class OrderBookLevel {
private:
    Price price_;
    std::queue<std::shared_ptr<Order>> orders_;
    Quantity total_quantity_;

public:
    explicit OrderBookLevel(Price price) : price_(price), total_quantity_(0) {}
    
    void add_order(std::shared_ptr<Order> order) {
        orders_.push(order);
        total_quantity_ += order->quantity;
    }
    
    std::shared_ptr<Order> get_front_order() {
        return orders_.empty() ? nullptr : orders_.front();
    }
    
    void remove_front_order() {
        if (!orders_.empty()) {
            total_quantity_ -= orders_.front()->quantity;
            orders_.pop();
        }
    }
    
    bool remove_order(OrderId order_id) {
        // Note: For production systems, we'd use a more efficient data structure
        // like a doubly-linked list for O(1) removal. This is O(n) for simplicity.
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
    
    Price get_price() const { return price_; }
    Quantity get_total_quantity() const { return total_quantity_; }
    bool is_empty() const { return orders_.empty(); }
};

// ============================================================================
// Order Book Side (Bid or Ask)
// ============================================================================

class OrderBookSide {
private:
    std::map<Price, std::unique_ptr<OrderBookLevel>> levels_;
    bool is_bid_side_;
    
public:
    explicit OrderBookSide(bool is_bid) : is_bid_side_(is_bid) {}
    
    void add_order(std::shared_ptr<Order> order) {
        Price price = order->price;
        
        if (levels_.find(price) == levels_.end()) {
            levels_[price] = std::make_unique<OrderBookLevel>(price);
        }
        
        levels_[price]->add_order(order);
    }
    
    std::shared_ptr<Order> get_best_order() {
        if (levels_.empty()) return nullptr;
        
        if (is_bid_side_) {
            auto best_it = levels_.rbegin();
            return best_it->second->get_front_order();
        } else {
            auto best_it = levels_.begin();
            return best_it->second->get_front_order();
        }
    }
    
    void remove_best_order() {
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
    
    bool remove_order(OrderId order_id, Price price) {
        auto it = levels_.find(price);
        if (it == levels_.end()) return false;
        
        bool removed = it->second->remove_order(order_id);
        if (removed && it->second->is_empty()) {
            levels_.erase(it);
        }
        
        return removed;
    }
    
    Price get_best_price() const {
        if (levels_.empty()) return 0;
        
        if (is_bid_side_) {
            auto best_it = levels_.rbegin();
            return best_it->second->get_price();
        } else {
            auto best_it = levels_.begin();
            return best_it->second->get_price();
        }
    }
    
    bool is_empty() const { return levels_.empty(); }
};

// ============================================================================
// Main Matching Engine
// ============================================================================

class MatchingEngine {
private:
    OrderBookSide bid_side_;
    OrderBookSide ask_side_;
    std::unordered_map<OrderId, std::pair<Price, OrderSide>> order_lookup_;
    std::vector<TradeEvent> trade_events_;
    mutable std::mutex engine_mutex_;
    std::atomic<uint64_t> processed_orders_{0};
    std::atomic<uint64_t> matched_trades_{0};
    
    // Performance metrics
    std::atomic<uint64_t> total_processing_time_ns_{0};
    
public:
    MatchingEngine() : bid_side_(true), ask_side_(false) {}
    
    // Add a new order to the book
    void add_order(std::shared_ptr<Order> order) {
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
    
    // Cancel an existing order
    bool cancel_order(OrderId order_id) {
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
    
    // Get recent trade events
    std::vector<TradeEvent> get_trade_events() const {
        std::lock_guard<std::mutex> lock(engine_mutex_);
        return trade_events_;
    }
    
    // Performance metrics
    uint64_t get_processed_orders() const { return processed_orders_.load(); }
    uint64_t get_matched_trades() const { return matched_trades_.load(); }
    double get_average_processing_time_ns() const {
        uint64_t orders = processed_orders_.load();
        return orders > 0 ? static_cast<double>(total_processing_time_ns_.load()) / orders : 0.0;
    }
    
    // Get market data snapshot
    std::pair<Price, Price> get_best_bid_ask() const {
        std::lock_guard<std::mutex> lock(engine_mutex_);
        return {bid_side_.get_best_price(), ask_side_.get_best_price()};
    }

private:
    void process_market_order(std::shared_ptr<Order> order) {
        if (order->side == OrderSide::BUY) {
            // Market buy order - match against ask side
            match_order_against_side(order, ask_side_);
        } else {
            // Market sell order - match against bid side
            match_order_against_side(order, bid_side_);
        }
    }
    
    void process_limit_order(std::shared_ptr<Order> order) {
        if (order->side == OrderSide::BUY) {
            // Try to match against ask side first
            match_order_against_side(order, ask_side_);
            
            // If any quantity remains, add to bid side
            if (order->quantity > 0) {
                bid_side_.add_order(order);
                order_lookup_[order->order_id] = {order->price, order->side};
            }
        } else {
            // Try to match against bid side first
            match_order_against_side(order, bid_side_);
            
            // If any quantity remains, add to ask side
            if (order->quantity > 0) {
                ask_side_.add_order(order);
                order_lookup_[order->order_id] = {order->price, order->side};
            }
        }
    }
    
    void match_order_against_side(std::shared_ptr<Order> incoming_order, OrderBookSide& opposite_side) {
        while (incoming_order->quantity > 0 && !opposite_side.is_empty()) {
            auto resting_order = opposite_side.get_best_order();
            if (!resting_order) break;
            
            // Check if orders can match
            bool can_match = false;
            if (incoming_order->type == OrderType::MARKET) {
                can_match = true;  // Market orders match at any price
            } else {
                // Limit order price check
                if (incoming_order->side == OrderSide::BUY) {
                    can_match = (incoming_order->price >= resting_order->price);
                } else {
                    can_match = (incoming_order->price <= resting_order->price);
                }
            }
            
            if (!can_match) break;
            
            // Execute the trade
            Price trade_price = resting_order->price;  // Resting order price priority
            Quantity trade_quantity = std::min(incoming_order->quantity, resting_order->quantity);
            
            // Create trade event
            OrderId buy_id = (incoming_order->side == OrderSide::BUY) ? 
                           incoming_order->order_id : resting_order->order_id;
            OrderId sell_id = (incoming_order->side == OrderSide::SELL) ? 
                            incoming_order->order_id : resting_order->order_id;
            
            trade_events_.emplace_back(buy_id, sell_id, trade_price, trade_quantity);
            matched_trades_++;
            
            // Update order quantities
            incoming_order->quantity -= trade_quantity;
            resting_order->quantity -= trade_quantity;
            
            // Remove fully filled resting order
            if (resting_order->quantity == 0) {
                opposite_side.remove_best_order();
                order_lookup_.erase(resting_order->order_id);
            }
        }
    }
};

// ============================================================================
// Order Generator (for testing)
// ============================================================================

class OrderGenerator {
private:
    std::mt19937 rng_;
    std::uniform_int_distribution<Price> price_dist_;
    std::uniform_int_distribution<Quantity> quantity_dist_;
    std::uniform_int_distribution<int> side_dist_;
    std::uniform_int_distribution<int> type_dist_;
    std::atomic<OrderId> next_order_id_{1};
    
public:
    OrderGenerator() : rng_(std::random_device{}()), 
                      price_dist_(100000, 110000),  // $10.00 - $11.00
                      quantity_dist_(1, 1000),
                      side_dist_(0, 1),
                      type_dist_(0, 9) {}  // 90% limit, 10% market
    
    std::shared_ptr<Order> generate_order() {
        OrderId id = next_order_id_++;
        OrderSide side = static_cast<OrderSide>(side_dist_(rng_));
        Price price = price_dist_(rng_);
        Quantity quantity = quantity_dist_(rng_);
        OrderType type = (type_dist_(rng_) < 9) ? OrderType::LIMIT : OrderType::MARKET;
        
        return std::make_shared<Order>(id, side, price, quantity, type);
    }
};

// ============================================================================
// Performance Monitor
// ============================================================================

class PerformanceMonitor {
private:
    std::chrono::steady_clock::time_point start_time_;
    std::atomic<bool> running_{false};
    
public:
    void start() {
        start_time_ = std::chrono::steady_clock::now();
        running_ = true;
    }
    
    void stop() {
        running_ = false;
    }
    
    void print_stats(const MatchingEngine& engine) {
        if (!running_) return;
        
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time_);
        
        uint64_t orders = engine.get_processed_orders();
        uint64_t trades = engine.get_matched_trades();
        double avg_latency_ns = engine.get_average_processing_time_ns();
        
        auto [best_bid, best_ask] = engine.get_best_bid_ask();
        
        std::cout << "\n=== NanoEX Performance Stats ===\n";
        std::cout << "Runtime: " << elapsed.count() << " ms\n";
        std::cout << "Orders processed: " << orders << "\n";
        std::cout << "Trades matched: " << trades << "\n";
        std::cout << "Orders/sec: " << (orders * 1000.0 / elapsed.count()) << "\n";
        std::cout << "Avg latency: " << std::fixed << std::setprecision(2) << avg_latency_ns << " ns\n";
        std::cout << "Best bid: $" << (best_bid / 10000.0) << "\n";
        std::cout << "Best ask: $" << (best_ask / 10000.0) << "\n";
        std::cout << "Spread: $" << ((best_ask - best_bid) / 10000.0) << "\n";
        std::cout << "================================\n";
    }
};

// ============================================================================
// Multi-threaded Simulation
// ============================================================================

void order_feeder_thread(MatchingEngine& engine, OrderGenerator& generator, 
                        std::atomic<bool>& should_stop, int orders_per_second) {
    auto sleep_duration = std::chrono::microseconds(1000000 / orders_per_second);
    
    while (!should_stop.load()) {
        auto order = generator.generate_order();
        engine.add_order(order);
        
        std::this_thread::sleep_for(sleep_duration);
    }
}

// ============================================================================
// Main Demo Function
// ============================================================================

int main() {
    std::cout << "=== NanoEX High-Frequency Trading Engine ===\n";
    std::cout << "Step 1: Core Matching Engine Demo\n\n";
    
    // Initialize components
    MatchingEngine engine;
    OrderGenerator generator;
    PerformanceMonitor monitor;
    
    // Threading controls
    std::atomic<bool> should_stop{false};
    const int ORDERS_PER_SECOND = 100000;  // 100k orders/sec
    const int SIMULATION_SECONDS = 5;
    
    // Start performance monitoring
    monitor.start();
    
    // Launch order feeder thread
    std::thread feeder_thread(order_feeder_thread, std::ref(engine), 
                             std::ref(generator), std::ref(should_stop), 
                             ORDERS_PER_SECOND);
    
    // Run simulation
    std::cout << "Running simulation for " << SIMULATION_SECONDS << " seconds...\n";
    std::cout << "Target: " << ORDERS_PER_SECOND << " orders/second\n\n";
    
    for (int i = 0; i < SIMULATION_SECONDS; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        monitor.print_stats(engine);
    }
    
    // Stop simulation
    should_stop = true;
    feeder_thread.join();
    monitor.stop();
    
    // Final stats
    std::cout << "\n=== Final Results ===\n";
    monitor.print_stats(engine);
    
    // Show some recent trades
    auto trades = engine.get_trade_events();
    std::cout << "\nRecent Trades (last 10):\n";
    int count = 0;
    for (auto it = trades.rbegin(); it != trades.rend() && count < 10; ++it, ++count) {
        std::cout << "Trade: Buy#" << it->buy_order_id << " Sell#" << it->sell_order_id
                  << " Price: $" << std::fixed << std::setprecision(4) << (it->price / 10000.0)
                  << " Qty: " << it->quantity << "\n";
    }
    
    std::cout << "\nStep 1 Complete! Ready for Step 2: Market Data Feed\n";
    
    return 0;
}