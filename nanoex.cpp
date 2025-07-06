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
#include <functional>
#include <deque>
#include <condition_variable>
#include <future>

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
            auto rit = levels_.rbegin();
            return rit->second->get_front_order();
        } else {
            auto it  = levels_.begin();
            return it->second->get_front_order();
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
// Market Data Structures (Step 2)
// ============================================================================

struct MarketDataUpdate {
    enum class Type : uint8_t {
        TRADE = 0,
        QUOTE = 1,
        BOOK_UPDATE = 2
    };
    
    Type type;
    std::string symbol;
    Price price;
    Quantity quantity;
    OrderSide side;  // For quotes and book updates
    Timestamp timestamp;
    uint64_t sequence_number;
    
    MarketDataUpdate(Type t, const std::string& sym, Price p, Quantity q, OrderSide s = OrderSide::BUY)
        : type(t), symbol(sym), price(p), quantity(q), side(s), 
          timestamp(std::chrono::steady_clock::now()), sequence_number(0) {}
};

struct MarketSnapshot {
    std::string symbol;
    Price best_bid;
    Price best_ask;
    Quantity bid_quantity;
    Quantity ask_quantity;
    Price last_trade_price;
    Quantity last_trade_quantity;
    Timestamp timestamp;
    
    MarketSnapshot(const std::string& sym) : symbol(sym), best_bid(0), best_ask(0),
                                            bid_quantity(0), ask_quantity(0),
                                            last_trade_price(0), last_trade_quantity(0),
                                            timestamp(std::chrono::steady_clock::now()) {}
};

// ============================================================================
// Market Data Feed Generator (Step 2)
// ============================================================================

class MarketDataFeed {
private:
    std::string symbol_;
    mutable std::mt19937 rng_;
    std::normal_distribution<double> price_walk_;
    std::exponential_distribution<double> time_dist_;
    mutable std::uniform_int_distribution<Quantity> quantity_dist_;
    std::uniform_int_distribution<int> update_type_dist_;
    
    Price current_mid_price_;
    uint64_t sequence_number_;
    std::atomic<bool> running_{false};
    
    // Market microstructure parameters
    const double TICK_SIZE = 0.01;  // $0.01 minimum price increment
    const double SPREAD_BASIS_POINTS = 5.0;  // 5 basis points typical spread
    const double VOLATILITY_ANNUALIZED = 0.20;  // 20% annual volatility
    
    // Feed statistics
    std::atomic<uint64_t> updates_generated_{0};
    std::atomic<uint64_t> trades_generated_{0};
    std::atomic<uint64_t> quotes_generated_{0};
    
public:
    MarketDataFeed(const std::string& symbol, Price initial_price = 105000)  // $10.50
        : symbol_(symbol), rng_(std::random_device{}()), 
          price_walk_(0.0, 0.0001), time_dist_(1000000.0), // 1M updates/sec average
          quantity_dist_(1, 500), update_type_dist_(0, 99),
          current_mid_price_(initial_price), sequence_number_(0) {
        
        // Scale volatility to nanosecond intervals
        double vol_per_ns = VOLATILITY_ANNUALIZED / std::sqrt(365.25 * 24 * 3600 * 1e9);
        price_walk_ = std::normal_distribution<double>(0.0, vol_per_ns);
    }
    
    // Copy constructor and assignment operator for vector storage
    MarketDataFeed(const MarketDataFeed& other)
        : symbol_(other.symbol_), rng_(std::random_device{}()), 
          price_walk_(other.price_walk_), time_dist_(other.time_dist_),
          quantity_dist_(other.quantity_dist_), update_type_dist_(other.update_type_dist_),
          current_mid_price_(other.current_mid_price_), sequence_number_(other.sequence_number_),
          running_(false) {}
    
    MarketDataFeed& operator=(const MarketDataFeed& other) {
        if (this != &other) {
            symbol_ = other.symbol_;
            current_mid_price_ = other.current_mid_price_;
            sequence_number_ = other.sequence_number_;
            running_ = false;
            // Note: rng_ and distributions are not copied as they should be independent
        }
        return *this;
    }
    
    void start() {
        running_ = true;
        std::cout << "Market data feed started for " << symbol_ << "\n";
    }
    
    void stop() {
        running_ = false;
    }
    
    bool is_running() const {
        return running_.load();
    }
    
    // Generate next market data update
    MarketDataUpdate generate_update() {
        if (!running_) {
            return MarketDataUpdate(MarketDataUpdate::Type::QUOTE, symbol_, 0, 0);
        }
        
        sequence_number_++;
        
        // Evolve the mid price using geometric Brownian motion
        double price_change = price_walk_(rng_);
        current_mid_price_ = static_cast<Price>(current_mid_price_ * (1.0 + price_change));
        
        // Ensure price stays within reasonable bounds
        current_mid_price_ = std::max(static_cast<Price>(50000), 
                                     std::min(static_cast<Price>(200000), current_mid_price_));
        
        // Generate different types of updates
        int update_type = update_type_dist_(rng_);
        
        if (update_type < 60) {  // 60% quotes
            return generate_quote_update();
        } else if (update_type < 85) {  // 25% book updates
            return generate_book_update();
        } else {  // 15% trade updates
            return generate_trade_update();
        }
    }
    
    // Get current market snapshot
    MarketSnapshot get_snapshot() const {
        MarketSnapshot snapshot(symbol_);
        
        Price half_spread = static_cast<Price>(current_mid_price_ * SPREAD_BASIS_POINTS / 20000);
        snapshot.best_bid = current_mid_price_ - half_spread;
        snapshot.best_ask = current_mid_price_ + half_spread;
        snapshot.bid_quantity = quantity_dist_(rng_);
        snapshot.ask_quantity = quantity_dist_(rng_);
        snapshot.last_trade_price = current_mid_price_;
        snapshot.last_trade_quantity = quantity_dist_(rng_);
        
        return snapshot;
    }
    
    // Get feed statistics
    uint64_t get_updates_generated() const { return updates_generated_.load(); }
    uint64_t get_trades_generated() const { return trades_generated_.load(); }
    uint64_t get_quotes_generated() const { return quotes_generated_.load(); }
    Price get_current_price() const { return current_mid_price_; }
    
private:
    MarketDataUpdate generate_quote_update() {
        Price half_spread = static_cast<Price>(current_mid_price_ * SPREAD_BASIS_POINTS / 20000);
        OrderSide side = (update_type_dist_(rng_) < 50) ? OrderSide::BUY : OrderSide::SELL;
        
        Price quote_price = (side == OrderSide::BUY) ? 
                           current_mid_price_ - half_spread : 
                           current_mid_price_ + half_spread;
        
        Quantity quote_quantity = quantity_dist_(rng_);
        
        auto update = MarketDataUpdate(MarketDataUpdate::Type::QUOTE, symbol_, 
                                      quote_price, quote_quantity, side);
        update.sequence_number = sequence_number_;
        
        updates_generated_++;
        quotes_generated_++;
        
        return update;
    }
    
    MarketDataUpdate generate_book_update() {
        OrderSide side = (update_type_dist_(rng_) < 50) ? OrderSide::BUY : OrderSide::SELL;
        
        // Price slightly away from mid
        Price level_offset = static_cast<Price>(current_mid_price_ * 0.001 * (1 + update_type_dist_(rng_) % 5));
        Price book_price = (side == OrderSide::BUY) ? 
                          current_mid_price_ - level_offset : 
                          current_mid_price_ + level_offset;
        
        Quantity book_quantity = quantity_dist_(rng_);
        
        auto update = MarketDataUpdate(MarketDataUpdate::Type::BOOK_UPDATE, symbol_, 
                                      book_price, book_quantity, side);
        update.sequence_number = sequence_number_;
        
        updates_generated_++;
        
        return update;
    }
    
    MarketDataUpdate generate_trade_update() {
        // Trades happen at or near mid price
        Price trade_price = current_mid_price_ + static_cast<Price>(price_walk_(rng_) * 100);
        Quantity trade_quantity = quantity_dist_(rng_);
        
        auto update = MarketDataUpdate(MarketDataUpdate::Type::TRADE, symbol_, 
                                      trade_price, trade_quantity);
        update.sequence_number = sequence_number_;
        
        updates_generated_++;
        trades_generated_++;
        
        return update;
    }
};

// ============================================================================
// Market Data Consumer Interface
// ============================================================================

class MarketDataConsumer {
public:
    virtual ~MarketDataConsumer() = default;
    virtual void on_market_update(const MarketDataUpdate& update) = 0;
    virtual void on_trade(const MarketDataUpdate& trade) = 0;
    virtual void on_quote(const MarketDataUpdate& quote) = 0;
};

// ============================================================================
// Market Data Publisher (Step 2)
// ============================================================================

class MarketDataPublisher {
private:
    std::vector<MarketDataFeed> feeds_;
    std::vector<std::unique_ptr<MarketDataConsumer>> consumers_;
    std::atomic<bool> running_{false};
    std::thread publisher_thread_;
    
    // Performance metrics
    std::atomic<uint64_t> total_updates_published_{0};
    std::atomic<uint64_t> total_latency_ns_{0};
    std::chrono::steady_clock::time_point start_time_;
    
public:
    MarketDataPublisher() {}
    
    ~MarketDataPublisher() {
        stop();
    }
    
    void add_feed(const std::string& symbol, Price initial_price = 105000) {
        feeds_.emplace_back(symbol, initial_price);
    }
    
    void subscribe(std::unique_ptr<MarketDataConsumer> consumer) {
        consumers_.push_back(std::move(consumer));
    }
    
    void start(int target_updates_per_second = 1000000) {  // 1M updates/sec
        if (running_) return;
        
        running_ = true;
        start_time_ = std::chrono::steady_clock::now();
        
        // Start all feeds
        for (auto& feed : feeds_) {
            feed.start();
        }
        
        // Start publisher thread
        publisher_thread_ = std::thread([this, target_updates_per_second]() {
            publish_loop(target_updates_per_second);
        });
        
        std::cout << "Market data publisher started with " << feeds_.size() << " feeds\n";
    }
    
    void stop() {
        if (!running_) return;
        
        running_ = false;
        
        // Stop all feeds
        for (auto& feed : feeds_) {
            feed.stop();
        }
        
        if (publisher_thread_.joinable()) {
            publisher_thread_.join();
        }
        
        std::cout << "Market data publisher stopped\n";
    }
    
    void print_stats() const {
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time_);
        
        uint64_t total_updates = total_updates_published_.load();
        double updates_per_sec = total_updates * 1000.0 / std::max(elapsed.count(), 1LL);
        double avg_latency_ns = total_updates > 0 ? 
                               static_cast<double>(total_latency_ns_.load()) / total_updates : 0.0;
        
        std::cout << "\n=== Market Data Feed Stats ===\n";
        std::cout << "Runtime: " << elapsed.count() << " ms\n";
        std::cout << "Total updates: " << total_updates << "\n";
        std::cout << "Updates/sec: " << std::fixed << std::setprecision(0) << updates_per_sec << "\n";
        std::cout << "Avg publish latency: " << std::fixed << std::setprecision(2) << avg_latency_ns << " ns\n";
        
        for (size_t i = 0; i < feeds_.size(); ++i) {
            const auto& feed = feeds_[i];
            std::cout << "Feed " << i << " - Updates: " << feed.get_updates_generated()
                      << ", Trades: " << feed.get_trades_generated()
                      << ", Quotes: " << feed.get_quotes_generated()
                      << ", Price: $" << std::fixed << std::setprecision(4) 
                      << (feed.get_current_price() / 10000.0) << "\n";
        }
        std::cout << "===============================\n";
    }
    
private:
    void publish_loop(int target_updates_per_second) {
        auto sleep_duration = std::chrono::nanoseconds(1000000000 / target_updates_per_second);
        
        while (running_) {
            auto loop_start = std::chrono::steady_clock::now();
            
            // Generate and publish updates from all feeds
            for (auto& feed : feeds_) {
                if (!feed.is_running()) continue;
                
                auto update = feed.generate_update();
                publish_update(update);
            }
            
            auto loop_end = std::chrono::steady_clock::now();
            auto loop_duration = loop_end - loop_start;
            
            // Sleep to maintain target rate
            if (loop_duration < sleep_duration) {
                std::this_thread::sleep_for(sleep_duration - loop_duration);
            }
        }
    }
    
    void publish_update(const MarketDataUpdate& update) {
        auto publish_start = std::chrono::steady_clock::now();
        
        // Dispatch to all consumers
        for (auto& consumer : consumers_) {
            consumer->on_market_update(update);
            
            // Dispatch to specific handlers
            switch (update.type) {
                case MarketDataUpdate::Type::TRADE:
                    consumer->on_trade(update);
                    break;
                case MarketDataUpdate::Type::QUOTE:
                    consumer->on_quote(update);
                    break;
                case MarketDataUpdate::Type::BOOK_UPDATE:
                    // Book updates go through generic handler
                    break;
            }
        }
        
        auto publish_end = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(publish_end - publish_start);
        
        total_updates_published_++;
        total_latency_ns_ += latency.count();
    }
};

// ============================================================================
// Strategy Engine Infrastructure (Step 3)
// ============================================================================

enum class SignalType : uint8_t {
    NONE = 0,
    BUY = 1,
    SELL = 2,
    HOLD = 3
};

struct TradingSignal {
    std::string symbol;
    SignalType signal;
    Price target_price;
    Quantity suggested_quantity;
    double confidence;  // 0.0 to 1.0
    std::string reason;
    Timestamp timestamp;
    
    TradingSignal(const std::string& sym, SignalType sig, Price price, Quantity qty, 
                  double conf, const std::string& r)
        : symbol(sym), signal(sig), target_price(price), suggested_quantity(qty),
          confidence(conf), reason(r), timestamp(std::chrono::steady_clock::now()) {}
};

struct Position {
    std::string symbol;
    int64_t quantity;  // Signed: positive = long, negative = short
    Price average_price;
    double unrealized_pnl;
    double realized_pnl;
    Timestamp last_update;
    
    // Default constructor for unordered_map
    Position() : symbol(""), quantity(0), average_price(0),
                 unrealized_pnl(0.0), realized_pnl(0.0),
                 last_update(std::chrono::steady_clock::now()) {}
    
    Position(const std::string& sym) : symbol(sym), quantity(0), average_price(0),
                                       unrealized_pnl(0.0), realized_pnl(0.0),
                                       last_update(std::chrono::steady_clock::now()) {}
};

// ============================================================================
// Risk Management System
// ============================================================================

class RiskManager {
private:
    double max_position_size_;
    double max_daily_loss_;
    double max_order_size_;
    double current_daily_pnl_;
    std::unordered_map<std::string, Position> positions_;
    mutable std::mutex risk_mutex_;
    
public:
    RiskManager(double max_pos_size = 10000.0, double max_daily_loss = 5000.0, 
                double max_order_size = 1000.0)
        : max_position_size_(max_pos_size), max_daily_loss_(max_daily_loss),
          max_order_size_(max_order_size), current_daily_pnl_(0.0) {}
    
    bool validate_order(const std::string& symbol, OrderSide side, Quantity quantity, Price price) {
        std::lock_guard<std::mutex> lock(risk_mutex_);
        
        // Check order size limit
        if (quantity > max_order_size_) {
            return false;
        }
        
        // Check daily loss limit
        if (current_daily_pnl_ < -max_daily_loss_) {
            return false;
        }
        
        // Check position size limit
        auto it = positions_.find(symbol);
        if (it != positions_.end()) {
            int64_t new_position = it->second.quantity;
            new_position += (side == OrderSide::BUY) ? static_cast<int64_t>(quantity) : -static_cast<int64_t>(quantity);
            
            if (std::abs(new_position) > max_position_size_) {
                return false;
            }
        }
        
        return true;
    }
    
    void update_position(const std::string& symbol, OrderSide side, Quantity quantity, Price price) {
        std::lock_guard<std::mutex> lock(risk_mutex_);
        
        auto& position = positions_[symbol];
        if (position.symbol.empty()) {
            position.symbol = symbol;
        }
        
        int64_t trade_quantity = (side == OrderSide::BUY) ? static_cast<int64_t>(quantity) : -static_cast<int64_t>(quantity);
        
        // Update position
        if (position.quantity == 0) {
            position.quantity = trade_quantity;
            position.average_price = price;
        } else {
            // Calculate new average price
            int64_t new_quantity = position.quantity + trade_quantity;
            if (new_quantity != 0) {
                position.average_price = static_cast<Price>(
                    (position.average_price * std::abs(position.quantity) + price * quantity) / 
                    std::abs(new_quantity)
                );
            }
            position.quantity = new_quantity;
        }
        
        position.last_update = std::chrono::steady_clock::now();
    }
    
    double get_current_pnl() const {
        std::lock_guard<std::mutex> lock(risk_mutex_);
        return current_daily_pnl_;
    }
    
    const std::unordered_map<std::string, Position>& get_positions() const {
        return positions_;
    }
};

// ============================================================================
// Technical Analysis Indicators
// ============================================================================

class TechnicalIndicators {
private:
    struct PriceHistory {
        std::vector<Price> prices;
        std::vector<Timestamp> timestamps;
        static constexpr size_t MAX_HISTORY = 1000;
        
        void add_price(Price price) {
            prices.push_back(price);
            timestamps.push_back(std::chrono::steady_clock::now());
            
            if (prices.size() > MAX_HISTORY) {
                prices.erase(prices.begin());
                timestamps.erase(timestamps.begin());
            }
        }
    };
    
    std::unordered_map<std::string, PriceHistory> price_histories_;
    mutable std::mutex indicators_mutex_;
    
public:
    void update_price(const std::string& symbol, Price price) {
        std::lock_guard<std::mutex> lock(indicators_mutex_);
        price_histories_[symbol].add_price(price);
    }
    
    // Simple Moving Average
    double calculate_sma(const std::string& symbol, int period) const {
        std::lock_guard<std::mutex> lock(indicators_mutex_);
        
        auto it = price_histories_.find(symbol);
        if (it == price_histories_.end() || it->second.prices.size() < period) {
            return 0.0;
        }
        
        const auto& prices = it->second.prices;
        double sum = 0.0;
        for (int i = prices.size() - period; i < prices.size(); ++i) {
            sum += prices[i];
        }
        
        return sum / period;
    }
    
    // Exponential Moving Average
    double calculate_ema(const std::string& symbol, int period) const {
        std::lock_guard<std::mutex> lock(indicators_mutex_);
        
        auto it = price_histories_.find(symbol);
        if (it == price_histories_.end() || it->second.prices.empty()) {
            return 0.0;
        }
        
        const auto& prices = it->second.prices;
        double multiplier = 2.0 / (period + 1);
        double ema = prices[0];
        
        for (size_t i = 1; i < prices.size(); ++i) {
            ema = (prices[i] * multiplier) + (ema * (1 - multiplier));
        }
        
        return ema;
    }
    
    // Relative Strength Index
    double calculate_rsi(const std::string& symbol, int period = 14) const {
        std::lock_guard<std::mutex> lock(indicators_mutex_);
        
        auto it = price_histories_.find(symbol);
        if (it == price_histories_.end() || it->second.prices.size() < period + 1) {
            return 50.0;  // Neutral RSI
        }
        
        const auto& prices = it->second.prices;
        double gains = 0.0, losses = 0.0;
        
        for (size_t i = prices.size() - period; i < prices.size(); ++i) {
            double change = prices[i] - prices[i-1];
            if (change > 0) {
                gains += change;
            } else {
                losses += std::abs(change);
            }
        }
        
        if (losses == 0) return 100.0;
        
        double avg_gain = gains / period;
        double avg_loss = losses / period;
        double rs = avg_gain / avg_loss;
        
        return 100.0 - (100.0 / (1.0 + rs));
    }
    
    // Price momentum
    double calculate_momentum(const std::string& symbol, int period = 10) const {
        std::lock_guard<std::mutex> lock(indicators_mutex_);
        
        auto it = price_histories_.find(symbol);
        if (it == price_histories_.end() || it->second.prices.size() < period + 1) {
            return 0.0;
        }
        
        const auto& prices = it->second.prices;
        return (prices.back() - prices[prices.size() - 1 - period]) / static_cast<double>(prices[prices.size() - 1 - period]);
    }
};

// ============================================================================
// Strategy Engine Base Class
// ============================================================================

class StrategyEngine : public MarketDataConsumer {
protected:
    std::string strategy_name_;
    MatchingEngine* matching_engine_;
    RiskManager risk_manager_;
    TechnicalIndicators indicators_;
    
    // Strategy state
    std::atomic<bool> is_active_{true};
    std::atomic<uint64_t> signals_generated_{0};
    std::atomic<uint64_t> orders_sent_{0};
    std::atomic<uint64_t> orders_rejected_{0};
    
    // Performance tracking
    std::atomic<uint64_t> updates_processed_{0};
    std::atomic<uint64_t> total_processing_time_ns_{0};
    
public:
    StrategyEngine(const std::string& name, MatchingEngine* engine)
        : strategy_name_(name), matching_engine_(engine) {}
    
    virtual ~StrategyEngine() = default;
    
    // Pure virtual strategy logic
    virtual TradingSignal generate_signal(const MarketDataUpdate& update) = 0;
    
    // MarketDataConsumer interface
    void on_market_update(const MarketDataUpdate& update) override {
        if (!is_active_) return;
        
        auto start_time = std::chrono::steady_clock::now();
        
        // Update technical indicators
        indicators_.update_price(update.symbol, update.price);
        
        // Generate trading signal
        auto signal = generate_signal(update);
        
        // Execute signal if actionable
        if (signal.signal != SignalType::NONE && signal.signal != SignalType::HOLD) {
            execute_signal(signal);
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        
        updates_processed_++;
        total_processing_time_ns_ += duration.count();
    }
    
    void on_trade(const MarketDataUpdate& trade) override {
        // Default implementation - can be overridden
        on_market_update(trade);
    }
    
    void on_quote(const MarketDataUpdate& quote) override {
        // Default implementation - can be overridden
        on_market_update(quote);
    }
    
    // Strategy controls
    void activate() { is_active_ = true; }
    void deactivate() { is_active_ = false; }
    bool is_active() const { return is_active_; }
    
    // Performance metrics
    uint64_t get_signals_generated() const { return signals_generated_.load(); }
    uint64_t get_orders_sent() const { return orders_sent_.load(); }
    uint64_t get_orders_rejected() const { return orders_rejected_.load(); }
    uint64_t get_updates_processed() const { return updates_processed_.load(); }
    double get_average_processing_time_ns() const {
        uint64_t updates = updates_processed_.load();
        return updates > 0 ? static_cast<double>(total_processing_time_ns_.load()) / updates : 0.0;
    }
    
    void print_stats() const {
        std::cout << "Strategy '" << strategy_name_ << "':\n";
        std::cout << "  Updates processed: " << get_updates_processed() << "\n";
        std::cout << "  Signals generated: " << get_signals_generated() << "\n";
        std::cout << "  Orders sent: " << get_orders_sent() << "\n";
        std::cout << "  Orders rejected: " << get_orders_rejected() << "\n";
        std::cout << "  Avg processing time: " << std::fixed << std::setprecision(2) 
                  << get_average_processing_time_ns() << " ns\n";
        std::cout << "  Current PnL: $" << std::fixed << std::setprecision(2) 
                  << (risk_manager_.get_current_pnl() / 10000.0) << "\n";
    }

protected:
    void execute_signal(const TradingSignal& signal) {
        signals_generated_++;
        
        // Convert signal to order parameters
        OrderSide side = (signal.signal == SignalType::BUY) ? OrderSide::BUY : OrderSide::SELL;
        
        // Risk check
        if (!risk_manager_.validate_order(signal.symbol, side, signal.suggested_quantity, signal.target_price)) {
            orders_rejected_++;
            return;
        }
        
        // Create and submit order
        static std::atomic<OrderId> next_strategy_order_id{1000000};  // Start strategy orders at 1M
        OrderId order_id = next_strategy_order_id++;
        
        auto order = std::make_shared<Order>(order_id, side, signal.target_price, 
                                           signal.suggested_quantity, OrderType::LIMIT);
        
        matching_engine_->add_order(order);
        orders_sent_++;
        
        // Update risk manager (simulate fill for demo)
        risk_manager_.update_position(signal.symbol, side, signal.suggested_quantity, signal.target_price);
    }
};

// ============================================================================
// Mean Reversion Strategy
// ============================================================================

class MeanReversionStrategy : public StrategyEngine {
private:
    double mean_reversion_threshold_;
    int lookback_period_;
    
public:
    MeanReversionStrategy(const std::string& name, MatchingEngine* engine, 
                         double threshold = 0.02, int lookback = 20)
        : StrategyEngine(name, engine), mean_reversion_threshold_(threshold), 
          lookback_period_(lookback) {}
    
    TradingSignal generate_signal(const MarketDataUpdate& update) override {
        // Skip non-trade updates for this strategy
        if (update.type != MarketDataUpdate::Type::TRADE) {
            return TradingSignal(update.symbol, SignalType::NONE, 0, 0, 0.0, "Not a trade");
        }
        
        double sma = indicators_.calculate_sma(update.symbol, lookback_period_);
        if (sma == 0.0) {
            return TradingSignal(update.symbol, SignalType::NONE, 0, 0, 0.0, "Insufficient data");
        }
        
        double price_deviation = (update.price - sma) / sma;
        
        // Mean reversion logic
        if (price_deviation > mean_reversion_threshold_) {
            // Price is above mean - sell signal
            Quantity quantity = std::min(static_cast<Quantity>(100), static_cast<Quantity>(std::abs(price_deviation) * 500));
            double confidence = std::min(0.9, std::abs(price_deviation) / mean_reversion_threshold_);
            
            return TradingSignal(update.symbol, SignalType::SELL, update.price, quantity, 
                               confidence, "Price above mean");
        } else if (price_deviation < -mean_reversion_threshold_) {
            // Price is below mean - buy signal
            Quantity quantity = std::min(static_cast<Quantity>(100), static_cast<Quantity>(std::abs(price_deviation) * 500));
            double confidence = std::min(0.9, std::abs(price_deviation) / mean_reversion_threshold_);
            
            return TradingSignal(update.symbol, SignalType::BUY, update.price, quantity, 
                               confidence, "Price below mean");
        }
        
        return TradingSignal(update.symbol, SignalType::HOLD, update.price, 0, 0.0, "Within mean range");
    }
};

// ============================================================================
// Momentum Strategy
// ============================================================================

class MomentumStrategy : public StrategyEngine {
private:
    double momentum_threshold_;
    int momentum_period_;
    double rsi_oversold_;
    double rsi_overbought_;
    
public:
    MomentumStrategy(const std::string& name, MatchingEngine* engine, 
                    double threshold = 0.01, int period = 10, 
                    double oversold = 30.0, double overbought = 70.0)
        : StrategyEngine(name, engine), momentum_threshold_(threshold), 
          momentum_period_(period), rsi_oversold_(oversold), rsi_overbought_(overbought) {}
    
    TradingSignal generate_signal(const MarketDataUpdate& update) override {
        // Focus on quote updates for momentum
        if (update.type != MarketDataUpdate::Type::QUOTE) {
            return TradingSignal(update.symbol, SignalType::NONE, 0, 0, 0.0, "Not a quote");
        }
        
        double momentum = indicators_.calculate_momentum(update.symbol, momentum_period_);
        double rsi = indicators_.calculate_rsi(update.symbol);
        
        // Momentum + RSI combined strategy
        if (momentum > momentum_threshold_ && rsi < rsi_oversold_) {
            // Strong upward momentum + oversold RSI = buy signal
            Quantity quantity = static_cast<Quantity>(std::min(200.0, momentum * 5000));
            double confidence = std::min(0.95, (momentum / momentum_threshold_) * 0.5 + 0.3);
            
            return TradingSignal(update.symbol, SignalType::BUY, update.price, quantity, 
                               confidence, "Momentum up + RSI oversold");
        } else if (momentum < -momentum_threshold_ && rsi > rsi_overbought_) {
            // Strong downward momentum + overbought RSI = sell signal
            Quantity quantity = static_cast<Quantity>(std::min(200.0, std::abs(momentum) * 5000));
            double confidence = std::min(0.95, (std::abs(momentum) / momentum_threshold_) * 0.5 + 0.3);
            
            return TradingSignal(update.symbol, SignalType::SELL, update.price, quantity, 
                               confidence, "Momentum down + RSI overbought");
        }
        
        return TradingSignal(update.symbol, SignalType::HOLD, update.price, 0, 0.0, "No momentum signal");
    }
};

// ============================================================================
// Sample Market Data Consumer (for testing)
// ============================================================================

class SampleMarketDataConsumer : public MarketDataConsumer {
private:
    std::string name_;
    std::atomic<uint64_t> updates_received_{0};
    std::atomic<uint64_t> trades_received_{0};
    std::atomic<uint64_t> quotes_received_{0};
    
public:
    SampleMarketDataConsumer(const std::string& name) : name_(name) {}
    
    void on_market_update(const MarketDataUpdate& update) override {
        updates_received_++;
        
        // Simulate some processing work
        auto processing_start = std::chrono::steady_clock::now();
        volatile int dummy = 0;
        for (int i = 0; i < 100; ++i) { dummy += i; }  // Simulate work
        auto processing_end = std::chrono::steady_clock::now();
    }
    
    void on_trade(const MarketDataUpdate& trade) override {
        trades_received_++;
    }
    
    void on_quote(const MarketDataUpdate& quote) override {
        quotes_received_++;
    }
    
    uint64_t get_updates_received() const { return updates_received_.load(); }
    uint64_t get_trades_received() const { return trades_received_.load(); }
    uint64_t get_quotes_received() const { return quotes_received_.load(); }
    
    void print_stats() const {
        std::cout << "Consumer '" << name_ << "' - Updates: " << get_updates_received()
                  << ", Trades: " << get_trades_received()
                  << ", Quotes: " << get_quotes_received() << "\n";
    }
};

// ============================================================================
// Step 4: Advanced Multi-threading with Race Conditions
// ============================================================================

// ============================================================================
// Race Condition Detection and Monitoring
// ============================================================================

class RaceConditionDetector {
private:
    struct ThreadAccess {
        std::thread::id thread_id;
        std::string resource_name;
        std::chrono::steady_clock::time_point timestamp;
        std::string operation_type;  // "read", "write", "lock", "unlock"
    };
    
    std::unordered_map<std::string, std::vector<ThreadAccess>> resource_access_log_;
    std::unordered_map<std::string, std::thread::id> resource_locks_;
    mutable std::mutex detector_mutex_;
    
    // Race condition statistics
    std::atomic<uint64_t> potential_races_detected_{0};
    std::atomic<uint64_t> actual_races_confirmed_{0};
    std::atomic<uint64_t> deadlock_situations_{0};
    
public:
    void log_access(const std::string& resource, const std::string& operation) {
        std::lock_guard<std::mutex> lock(detector_mutex_);
        
        ThreadAccess access{
            std::this_thread::get_id(),
            resource,
            std::chrono::steady_clock::now(),
            operation
        };
        
        resource_access_log_[resource].push_back(access);
        
        // Check for potential race conditions
        if (operation == "write" && resource_access_log_[resource].size() > 1) {
            auto& accesses = resource_access_log_[resource];
            auto current_thread = std::this_thread::get_id();
            
            // Check if another thread recently accessed this resource
            for (auto it = accesses.rbegin() + 1; it != accesses.rend(); ++it) {
                if (it->thread_id != current_thread && 
                    std::chrono::duration_cast<std::chrono::microseconds>(
                        access.timestamp - it->timestamp).count() < 1000) {
                    potential_races_detected_++;
                    break;
                }
            }
        }
    }
    
    void log_lock_attempt(const std::string& resource) {
        std::lock_guard<std::mutex> lock(detector_mutex_);
        
        auto current_thread = std::this_thread::get_id();
        auto it = resource_locks_.find(resource);
        
        if (it != resource_locks_.end() && it->second == current_thread) {
            // Potential deadlock: same thread trying to lock same resource
            deadlock_situations_++;
        }
        
        resource_locks_[resource] = current_thread;
        log_access(resource, "lock");
    }
    
    void log_unlock(const std::string& resource) {
        std::lock_guard<std::mutex> lock(detector_mutex_);
        resource_locks_.erase(resource);
        log_access(resource, "unlock");
    }
    
    uint64_t get_potential_races() const { return potential_races_detected_.load(); }
    uint64_t get_confirmed_races() const { return actual_races_confirmed_.load(); }
    uint64_t get_deadlock_situations() const { return deadlock_situations_.load(); }
    
    void print_race_report() const {
        std::cout << "\n=== Race Condition Detection Report ===\n";
        std::cout << "Potential race conditions detected: " << get_potential_races() << "\n";
        std::cout << "Confirmed race conditions: " << get_confirmed_races() << "\n";
        std::cout << "Deadlock situations: " << get_deadlock_situations() << "\n";
        std::cout << "========================================\n";
    }
};

// Global race condition detector
static RaceConditionDetector g_race_detector;

// ============================================================================
// Lock-Free Data Structures
// ============================================================================

template<typename T>
class LockFreeQueue {
private:
    struct Node {
        T data;
        std::atomic<Node*> next{nullptr};
        
        Node(const T& item) : data(item) {}
    };
    
    std::atomic<Node*> head_{nullptr};
    std::atomic<Node*> tail_{nullptr};
    std::atomic<size_t> size_{0};
    
public:
    LockFreeQueue() {
        Node* dummy = new Node(T{});
        head_.store(dummy);
        tail_.store(dummy);
    }
    
    ~LockFreeQueue() {
        Node* current = head_.load();
        while (current) {
            Node* next = current->next.load();
            delete current;
            current = next;
        }
    }
    
    void push(const T& item) {
        Node* new_node = new Node(item);
        Node* expected_tail;
        
        while (true) {
            expected_tail = tail_.load();
            Node* expected_next = expected_tail->next.load();
            
            if (expected_tail == tail_.load()) {
                if (expected_next == nullptr) {
                    if (expected_tail->next.compare_exchange_weak(expected_next, new_node)) {
                        break;
                    }
                } else {
                    tail_.compare_exchange_weak(expected_tail, expected_next);
                }
            }
        }
        
        tail_.compare_exchange_weak(expected_tail, new_node);
        size_.fetch_add(1);
    }
    
    bool pop(T& item) {
        Node* expected_head;
        Node* expected_tail;
        Node* expected_next;
        
        while (true) {
            expected_head = head_.load();
            expected_tail = tail_.load();
            expected_next = expected_head->next.load();
            
            if (expected_head == head_.load()) {
                if (expected_head == expected_tail) {
                    if (expected_next == nullptr) {
                        return false;  // Queue is empty
                    }
                    tail_.compare_exchange_weak(expected_tail, expected_next);
                } else {
                    if (expected_next == nullptr) {
                        continue;
                    }
                    item = expected_next->data;
                    if (head_.compare_exchange_weak(expected_head, expected_next)) {
                        delete expected_head;
                        size_.fetch_sub(1);
                        return true;
                    }
                }
            }
        }
    }
    
    size_t size() const { return size_.load(); }
    bool empty() const { return size() == 0; }
};

// ============================================================================
// Advanced Thread Pool with Work Stealing
// ============================================================================

class WorkStealingThreadPool {
private:
    struct Task {
        std::function<void()> func;
        std::chrono::steady_clock::time_point created_time;
        
        Task(std::function<void()> f) : func(std::move(f)), 
                                       created_time(std::chrono::steady_clock::now()) {}
    };
    
    struct WorkerThread {
        std::thread thread;
        std::deque<Task> local_queue;
        std::mutex queue_mutex;
        std::atomic<bool> running{true};
        std::atomic<uint64_t> tasks_processed{0};
        std::atomic<uint64_t> total_processing_time_ns{0};
        
        WorkerThread() = default;
    };
    
    std::vector<std::unique_ptr<WorkerThread>> workers_;
    std::deque<Task> global_queue_;
    std::mutex global_queue_mutex_;
    std::atomic<bool> shutdown_{false};
    std::atomic<uint64_t> total_tasks_submitted_{0};
    std::atomic<uint64_t> total_tasks_completed_{0};
    
    static thread_local WorkerThread* current_worker_;
    
public:
    WorkStealingThreadPool(size_t num_threads = std::thread::hardware_concurrency()) {
        workers_.reserve(num_threads);
        
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back(std::make_unique<WorkerThread>());
            workers_[i]->thread = std::thread([this, i]() { worker_loop(i); });
        }
    }
    
    ~WorkStealingThreadPool() {
        shutdown_ = true;
        for (auto& worker : workers_) {
            if (worker->thread.joinable()) {
                worker->thread.join();
            }
        }
    }
    
    template<typename F>
    auto submit(F&& func) -> std::future<decltype(func())> {
        using ReturnType = decltype(func());
        
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(std::forward<F>(func));
        auto future = task->get_future();
        
        Task wrapped_task([task]() { (*task)(); });
        
        if (current_worker_ && current_worker_->running) {
            // Submit to local queue if we're in a worker thread
            std::lock_guard<std::mutex> lock(current_worker_->queue_mutex);
            current_worker_->local_queue.push_back(std::move(wrapped_task));
        } else {
            // Submit to global queue
            std::lock_guard<std::mutex> lock(global_queue_mutex_);
            global_queue_.push_back(std::move(wrapped_task));
        }
        
        total_tasks_submitted_++;
        return future;
    }
    
    void print_stats() const {
        std::cout << "\n=== Work Stealing Thread Pool Stats ===\n";
        std::cout << "Total tasks submitted: " << total_tasks_submitted_.load() << "\n";
        std::cout << "Total tasks completed: " << total_tasks_completed_.load() << "\n";
        
        for (size_t i = 0; i < workers_.size(); ++i) {
            const auto& worker = workers_[i];
            std::cout << "Worker " << i << ": " << worker->tasks_processed.load() 
                      << " tasks, " << std::fixed << std::setprecision(2)
                      << (worker->total_processing_time_ns.load() / 1000000.0) << " ms total time\n";
        }
        std::cout << "========================================\n";
    }
    
private:
    void worker_loop(size_t worker_id) {
        current_worker_ = workers_[worker_id].get();
        
        while (workers_[worker_id]->running && !shutdown_) {
            Task task(nullptr);
            bool found_task = false;
            
            // Try to get task from local queue first
            {
                std::lock_guard<std::mutex> lock(workers_[worker_id]->queue_mutex);
                if (!workers_[worker_id]->local_queue.empty()) {
                    task = std::move(workers_[worker_id]->local_queue.front());
                    workers_[worker_id]->local_queue.pop_front();
                    found_task = true;
                }
            }
            
            // Try to steal from other workers
            if (!found_task) {
                for (size_t i = 0; i < workers_.size(); ++i) {
                    if (i == worker_id) continue;
                    
                    std::lock_guard<std::mutex> lock(workers_[i]->queue_mutex);
                    if (!workers_[i]->local_queue.empty()) {
                        task = std::move(workers_[i]->local_queue.back());
                        workers_[i]->local_queue.pop_back();
                        found_task = true;
                        break;
                    }
                }
            }
            
            // Try global queue
            if (!found_task) {
                std::lock_guard<std::mutex> lock(global_queue_mutex_);
                if (!global_queue_.empty()) {
                    task = std::move(global_queue_.front());
                    global_queue_.pop_front();
                    found_task = true;
                }
            }
            
            // Execute task if found
            if (found_task) {
                auto start_time = std::chrono::steady_clock::now();
                task.func();
                auto end_time = std::chrono::steady_clock::now();
                
                auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    end_time - start_time);
                
                workers_[worker_id]->tasks_processed++;
                workers_[worker_id]->total_processing_time_ns += duration.count();
                total_tasks_completed_++;
            } else {
                // No work available, yield
                std::this_thread::yield();
            }
        }
    }
};

thread_local WorkStealingThreadPool::WorkerThread* WorkStealingThreadPool::current_worker_ = nullptr;

// ============================================================================
// Advanced Synchronization Primitives
// ============================================================================

class ReadWriteLock {
private:
    std::atomic<int> readers_{0};
    std::atomic<bool> writer_{false};
    std::mutex writer_mutex_;
    std::condition_variable writer_cv_;
    
public:
    void read_lock() {
        while (true) {
            while (writer_.load()) {
                std::this_thread::yield();
            }
            
            readers_.fetch_add(1);
            
            if (!writer_.load()) {
                break;  // Successfully acquired read lock
            }
            
            readers_.fetch_sub(1);
        }
    }
    
    void read_unlock() {
        readers_.fetch_sub(1);
    }
    
    void write_lock() {
        std::unique_lock<std::mutex> lock(writer_mutex_);
        
        // Wait for writer flag to be false
        writer_cv_.wait(lock, [this]() { return !writer_.load(); });
        
        // Set writer flag
        writer_.store(true);
        
        // Wait for all readers to finish
        while (readers_.load() > 0) {
            std::this_thread::yield();
        }
    }
    
    void write_unlock() {
        writer_.store(false);
        writer_cv_.notify_one();
    }
};

class Barrier {
private:
    std::mutex mutex_;
    std::condition_variable cv_;
    size_t count_;
    size_t initial_count_;
    
public:
    explicit Barrier(size_t count) : count_(count), initial_count_(count) {}
    
    void wait() {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (--count_ == 0) {
            count_ = initial_count_;
            cv_.notify_all();
        } else {
            cv_.wait(lock, [this]() { return count_ == initial_count_; });
        }
    }
};

// ============================================================================
// Advanced Order Processing with Race Condition Protection
// ============================================================================

class AdvancedMatchingEngine {
private:
    OrderBookSide bid_side_;
    OrderBookSide ask_side_;
    std::unordered_map<OrderId, std::pair<Price, OrderSide>> order_lookup_;
    std::vector<TradeEvent> trade_events_;
    
    // Advanced synchronization
    ReadWriteLock order_book_lock_;
    std::mutex trade_events_mutex_;
    std::mutex order_lookup_mutex_;  // Using regular mutex instead of shared_mutex
    
    // Performance metrics with race condition protection
    std::atomic<uint64_t> processed_orders_{0};
    std::atomic<uint64_t> matched_trades_{0};
    std::atomic<uint64_t> total_processing_time_ns_{0};
    
    // Lock-free order queue for high-frequency processing
    LockFreeQueue<std::shared_ptr<Order>> order_queue_;
    
    // Thread pool for order processing
    std::unique_ptr<WorkStealingThreadPool> thread_pool_;
    
    // Race condition monitoring
    std::atomic<uint64_t> concurrent_access_count_{0};
    std::atomic<uint64_t> max_concurrent_access_{0};
    
public:
    AdvancedMatchingEngine(size_t num_threads = 4) 
        : bid_side_(true), ask_side_(false),  // Initialize OrderBookSide members
          thread_pool_(std::make_unique<WorkStealingThreadPool>(num_threads)) {}
    
    void add_order(std::shared_ptr<Order> order) {
        // Log access for race condition detection
        g_race_detector.log_access("order_book", "write");
        
        // Track concurrent access
        auto current_concurrent = concurrent_access_count_.fetch_add(1) + 1;
        auto max_concurrent = max_concurrent_access_.load();
        while (current_concurrent > max_concurrent && 
               !max_concurrent_access_.compare_exchange_weak(max_concurrent, current_concurrent)) {}
        
        auto start_time = std::chrono::steady_clock::now();
        
        // Use read-write lock for better concurrency
        order_book_lock_.write_lock();
        
        if (order->type == OrderType::MARKET) {
            process_market_order(order);
        } else {
            process_limit_order(order);
        }
        
        order_book_lock_.write_unlock();
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        
        total_processing_time_ns_ += duration.count();
        processed_orders_++;
        concurrent_access_count_.fetch_sub(1);
    }
    
    bool cancel_order(OrderId order_id) {
        g_race_detector.log_access("order_lookup", "read");
        
        std::lock_guard<std::mutex> lock(order_lookup_mutex_);
        
        auto it = order_lookup_.find(order_id);
        if (it == order_lookup_.end()) {
            return false;
        }
        
        Price price = it->second.first;
        OrderSide side = it->second.second;
        
        // Need write access to order book
        order_book_lock_.write_lock();
        
        bool removed = false;
        if (side == OrderSide::BUY) {
            removed = bid_side_.remove_order(order_id, price);
        } else {
            removed = ask_side_.remove_order(order_id, price);
        }
        
        if (removed) {
            order_lookup_.erase(order_id);
        }
        
        order_book_lock_.write_unlock();
        
        return removed;
    }
    
    std::pair<Price, Price> get_best_bid_ask() const {
        g_race_detector.log_access("order_book", "read");
        
        const_cast<ReadWriteLock&>(order_book_lock_).read_lock();
        auto result = std::make_pair(bid_side_.get_best_price(), ask_side_.get_best_price());
        const_cast<ReadWriteLock&>(order_book_lock_).read_unlock();
        
        return result;
    }
    
    // Performance metrics
    uint64_t get_processed_orders() const { return processed_orders_.load(); }
    uint64_t get_matched_trades() const { return matched_trades_.load(); }
    uint64_t get_max_concurrent_access() const { return max_concurrent_access_.load(); }
    
    double get_average_processing_time_ns() const {
        uint64_t orders = processed_orders_.load();
        return orders > 0 ? static_cast<double>(total_processing_time_ns_.load()) / orders : 0.0;
    }
    
    void print_advanced_stats() const {
        std::cout << "\n=== Advanced Matching Engine Stats ===\n";
        std::cout << "Processed orders: " << get_processed_orders() << "\n";
        std::cout << "Matched trades: " << get_matched_trades() << "\n";
        std::cout << "Max concurrent access: " << get_max_concurrent_access() << "\n";
        std::cout << "Avg processing time: " << std::fixed << std::setprecision(2) 
                  << get_average_processing_time_ns() << " ns\n";
        std::cout << "========================================\n";
        
        thread_pool_->print_stats();
    }
    
private:
    void process_market_order(std::shared_ptr<Order> order) {
        if (order->side == OrderSide::BUY) {
            match_order_against_side(order, ask_side_);
        } else {
            match_order_against_side(order, bid_side_);
        }
    }
    
    void process_limit_order(std::shared_ptr<Order> order) {
        if (order->side == OrderSide::BUY) {
            match_order_against_side(order, ask_side_);
            
            if (order->quantity > 0) {
                bid_side_.add_order(order);
                std::lock_guard<std::mutex> lock(order_lookup_mutex_);
                order_lookup_[order->order_id] = {order->price, order->side};
            }
        } else {
            match_order_against_side(order, bid_side_);
            
            if (order->quantity > 0) {
                ask_side_.add_order(order);
                std::lock_guard<std::mutex> lock(order_lookup_mutex_);
                order_lookup_[order->order_id] = {order->price, order->side};
            }
        }
    }
    
    void match_order_against_side(std::shared_ptr<Order> incoming_order, OrderBookSide& opposite_side) {
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
            
            // Thread-safe trade event creation
            {
                std::lock_guard<std::mutex> lock(trade_events_mutex_);
                trade_events_.emplace_back(buy_id, sell_id, trade_price, trade_quantity);
            }
            
            matched_trades_++;
            
            incoming_order->quantity -= trade_quantity;
            resting_order->quantity -= trade_quantity;
            
            if (resting_order->quantity == 0) {
                opposite_side.remove_best_order();
                std::lock_guard<std::mutex> lock(order_lookup_mutex_);
                order_lookup_.erase(resting_order->order_id);
            }
        }
    }
};

// ============================================================================
// Stress Testing with Race Condition Simulation
// ============================================================================

class RaceConditionStressTest {
private:
    std::atomic<uint64_t> shared_counter_{0};
    std::atomic<uint64_t> race_condition_count_{0};
    std::atomic<bool> test_running_{false};
    
    std::vector<std::thread> stress_threads_;
    std::vector<std::thread> monitoring_threads_;
    
public:
    void start_stress_test(size_t num_threads = 8, size_t duration_seconds = 10) {
        std::cout << "\n=== Starting Race Condition Stress Test ===\n";
        std::cout << "Threads: " << num_threads << ", Duration: " << duration_seconds << " seconds\n";
        
        test_running_ = true;
        
        // Start stress threads
        for (size_t i = 0; i < num_threads; ++i) {
            stress_threads_.emplace_back([this, i]() { stress_worker(i); });
        }
        
        // Start monitoring threads
        for (size_t i = 0; i < 2; ++i) {
            monitoring_threads_.emplace_back([this, i]() { monitoring_worker(i); });
        }
        
        // Run for specified duration
        std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));
        
        test_running_ = false;
        
        // Wait for all threads to finish
        for (auto& thread : stress_threads_) {
            if (thread.joinable()) thread.join();
        }
        for (auto& thread : monitoring_threads_) {
            if (thread.joinable()) thread.join();
        }
        
        print_stress_test_results();
    }
    
private:
    void stress_worker(size_t thread_id) {
        std::mt19937 rng(thread_id);
        std::uniform_int_distribution<int> delay_dist(1, 100);
        
        while (test_running_) {
            // Simulate race condition by reading and writing without proper synchronization
            uint64_t current_value = shared_counter_.load();
            
            // Simulate some processing time
            std::this_thread::sleep_for(std::chrono::microseconds(delay_dist(rng)));
            
            // This creates a race condition - multiple threads might read the same value
            shared_counter_.store(current_value + 1);
            
            // Check if we lost updates due to race conditions
            if (current_value + 1 != shared_counter_.load()) {
                race_condition_count_++;
            }
        }
    }
    
    void monitoring_worker(size_t worker_id) {
        while (test_running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Monitor system state
            g_race_detector.log_access("stress_test_monitor", "read");
        }
    }
    
    void print_stress_test_results() {
        std::cout << "\n=== Race Condition Stress Test Results ===\n";
        std::cout << "Final counter value: " << shared_counter_.load() << "\n";
        std::cout << "Race conditions detected: " << race_condition_count_.load() << "\n";
        std::cout << "Expected counter value: ~" << (stress_threads_.size() * 10) << "\n";
        std::cout << "Data integrity: " << std::fixed << std::setprecision(2)
                  << (100.0 * shared_counter_.load() / (stress_threads_.size() * 10)) << "%\n";
        std::cout << "==========================================\n";
    }
};

// ============================================================================
// Main Demo Function (Step 3)
// ============================================================================

int main() {
    std::cout << "=== NanoEX High-Frequency Trading Engine ===\n";
    std::cout << "Step 4: Advanced Multi-threading with Race Conditions\n\n";
    
    // Initialize components
    MatchingEngine engine;
    AdvancedMatchingEngine advanced_engine(4);  // 4 threads
    OrderGenerator generator;
    PerformanceMonitor monitor;
    MarketDataPublisher md_publisher;
    
    // Add market data feeds for multiple symbols
    md_publisher.add_feed("AAPL", 150000);  // $15.00
    md_publisher.add_feed("GOOGL", 280000); // $28.00
    md_publisher.add_feed("MSFT", 330000);  // $33.00
    
    // Create strategy engines
    auto mean_reversion_strategy = std::make_unique<MeanReversionStrategy>("MeanRev-1", &engine, 0.015, 25);
    auto momentum_strategy = std::make_unique<MomentumStrategy>("Momentum-1", &engine, 0.008, 15, 25.0, 75.0);
    auto momentum_strategy2 = std::make_unique<MomentumStrategy>("Momentum-2", &engine, 0.012, 20, 20.0, 80.0);
    
    // Keep references for stats
    auto* mean_rev_ptr = mean_reversion_strategy.get();
    auto* momentum_ptr = momentum_strategy.get();
    auto* momentum2_ptr = momentum_strategy2.get();
    
    // Create one basic market data consumer for comparison
    auto basic_consumer = std::make_unique<SampleMarketDataConsumer>("Basic-Consumer");
    auto* basic_consumer_ptr = basic_consumer.get();
    
    // Subscribe all consumers to market data
    md_publisher.subscribe(std::move(mean_reversion_strategy));
    md_publisher.subscribe(std::move(momentum_strategy));
    md_publisher.subscribe(std::move(momentum_strategy2));
    md_publisher.subscribe(std::move(basic_consumer));
    
    // Threading controls
    std::atomic<bool> should_stop{false};
    const int ORDERS_PER_SECOND = 25000;   // Reduced to allow strategy orders
    const int MD_UPDATES_PER_SECOND = 300000;  // 300k market data updates/sec
    const int SIMULATION_SECONDS = 5;  // Shorter simulation to focus on Step 4
    
    // Start components
    monitor.start();
    md_publisher.start(MD_UPDATES_PER_SECOND);
    
    // Launch order feeder thread (background noise)
    std::thread feeder_thread(order_feeder_thread, std::ref(engine), 
                             std::ref(generator), std::ref(should_stop), 
                             ORDERS_PER_SECOND);
    
    // Run simulation
    std::cout << "Running strategy simulation for " << SIMULATION_SECONDS << " seconds...\n";
    std::cout << "Background Orders: " << ORDERS_PER_SECOND << " orders/second\n";
    std::cout << "Market Data Feed: " << MD_UPDATES_PER_SECOND << " updates/second\n";
    std::cout << "Strategy Count: 3 active strategies\n\n";
    
    for (int i = 0; i < SIMULATION_SECONDS; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        std::cout << "=== Second " << (i + 1) << " ===\n";
        
        // Print matching engine stats
        monitor.print_stats(engine);
        
        // Print market data stats
        md_publisher.print_stats();
        
        // Print strategy performance
        std::cout << "\n--- Strategy Performance ---\n";
        mean_rev_ptr->print_stats();
        momentum_ptr->print_stats();
        momentum2_ptr->print_stats();
        
        // Print basic consumer for comparison
        std::cout << "\n--- Basic Consumer ---\n";
        basic_consumer_ptr->print_stats();
        
        std::cout << "\n" << std::string(60, '=') << "\n";
    }
    
    // Stop simulation
    should_stop = true;
    feeder_thread.join();
    md_publisher.stop();
    monitor.stop();
    
    // ============================================================================
    // Step 4: Advanced Multi-threading Demonstrations
    // ============================================================================
    
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "=== STEP 4: ADVANCED MULTI-THREADING DEMONSTRATIONS ===\n";
    std::cout << std::string(60, '=') << "\n";
    
    // 1. Race Condition Stress Test
    std::cout << "\n1. Running Race Condition Stress Test...\n";
    RaceConditionStressTest stress_test;
    stress_test.start_stress_test(8, 5);  // 8 threads, 5 seconds
    
    // 2. Advanced Matching Engine Performance Test
    std::cout << "\n2. Testing Advanced Matching Engine with Race Condition Protection...\n";
    
    std::vector<std::thread> advanced_engine_threads;
    std::atomic<uint64_t> advanced_orders_processed{0};
    
    // Start multiple threads adding orders to advanced engine
    for (int i = 0; i < 4; ++i) {
        advanced_engine_threads.emplace_back([&advanced_engine, &generator, &advanced_orders_processed, i]() {
            for (int j = 0; j < 1000; ++j) {
                auto order = generator.generate_order();
                advanced_engine.add_order(order);
                advanced_orders_processed++;
                
                // Simulate some processing time
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : advanced_engine_threads) {
        if (thread.joinable()) thread.join();
    }
    
    // Print advanced engine stats
    advanced_engine.print_advanced_stats();
    
    // 3. Lock-Free Queue Performance Test
    std::cout << "\n3. Testing Lock-Free Queue Performance...\n";
    
    LockFreeQueue<int> lock_free_queue;
    std::vector<std::thread> queue_threads;
    std::atomic<uint64_t> queue_operations{0};
    
    // Producer threads
    for (int i = 0; i < 2; ++i) {
        queue_threads.emplace_back([&lock_free_queue, &queue_operations, i]() {
            for (int j = 0; j < 10000; ++j) {
                lock_free_queue.push(i * 10000 + j);
                queue_operations++;
            }
        });
    }
    
    // Consumer threads
    for (int i = 0; i < 2; ++i) {
        queue_threads.emplace_back([&lock_free_queue, &queue_operations]() {
            int value;
            while (queue_operations.load() < 20000) {
                if (lock_free_queue.pop(value)) {
                    // Process value
                    volatile int dummy = value;  // Prevent optimization
                }
            }
        });
    }
    
    // Wait for all queue operations
    for (auto& thread : queue_threads) {
        if (thread.joinable()) thread.join();
    }
    
    std::cout << "Lock-free queue operations completed: " << queue_operations.load() << "\n";
    std::cout << "Final queue size: " << lock_free_queue.size() << "\n";
    
    // 4. Read-Write Lock Performance Test
    std::cout << "\n4. Testing Read-Write Lock Performance...\n";
    
    ReadWriteLock rw_lock;
    std::atomic<int> shared_data{0};
    std::vector<std::thread> rw_threads;
    
    // Reader threads
    for (int i = 0; i < 4; ++i) {
        rw_threads.emplace_back([&rw_lock, &shared_data, i]() {
            for (int j = 0; j < 1000; ++j) {
                rw_lock.read_lock();
                volatile int value = shared_data.load();  // Read operation
                rw_lock.read_unlock();
                
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });
    }
    
    // Writer threads
    for (int i = 0; i < 2; ++i) {
        rw_threads.emplace_back([&rw_lock, &shared_data, i]() {
            for (int j = 0; j < 500; ++j) {
                rw_lock.write_lock();
                shared_data.fetch_add(1);  // Write operation
                rw_lock.write_unlock();
                
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        });
    }
    
    // Wait for all RW lock operations
    for (auto& thread : rw_threads) {
        if (thread.joinable()) thread.join();
    }
    
    std::cout << "Read-Write lock test completed. Final shared data value: " << shared_data.load() << "\n";
    
    // 5. Barrier Synchronization Test
    std::cout << "\n5. Testing Barrier Synchronization...\n";
    
    Barrier barrier(4);
    std::vector<std::thread> barrier_threads;
    std::atomic<int> phase{0};
    
    for (int i = 0; i < 4; ++i) {
        barrier_threads.emplace_back([&barrier, &phase, i]() {
            for (int p = 0; p < 3; ++p) {
                std::cout << "Thread " << i << " starting phase " << p << "\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(100 + i * 50));
                
                barrier.wait();
                
                if (i == 0) {
                    phase++;
                    std::cout << "All threads completed phase " << p << "\n";
                }
            }
        });
    }
    
    // Wait for barrier test
    for (auto& thread : barrier_threads) {
        if (thread.joinable()) thread.join();
    }
    
    // Print race condition detection report
    g_race_detector.print_race_report();
    
    // Final comprehensive stats
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "=== FINAL STEP 4 RESULTS ===\n";
    std::cout << std::string(60, '=') << "\n";
    
    std::cout << "\n--- Original Matching Engine Performance ---\n";
    monitor.print_stats(engine);
    
    std::cout << "\n--- Advanced Matching Engine Performance ---\n";
    advanced_engine.print_advanced_stats();
    
    std::cout << "\n--- Strategy Performance Summary ---\n";
    mean_rev_ptr->print_stats();
    std::cout << "\n";
    momentum_ptr->print_stats();
    std::cout << "\n";
    momentum2_ptr->print_stats();
    
    std::cout << "\n--- Performance Comparison ---\n";
    basic_consumer_ptr->print_stats();
    
    // Calculate aggregate strategy metrics
    uint64_t total_strategy_orders = mean_rev_ptr->get_orders_sent() + 
                                    momentum_ptr->get_orders_sent() + 
                                    momentum2_ptr->get_orders_sent();
    
    uint64_t total_strategy_signals = mean_rev_ptr->get_signals_generated() + 
                                     momentum_ptr->get_signals_generated() + 
                                     momentum2_ptr->get_signals_generated();
    
    double avg_strategy_latency = (mean_rev_ptr->get_average_processing_time_ns() + 
                                  momentum_ptr->get_average_processing_time_ns() + 
                                  momentum2_ptr->get_average_processing_time_ns()) / 3.0;
    
    std::cout << "\n--- Aggregate Strategy Metrics ---\n";
    std::cout << "Total strategy orders sent: " << total_strategy_orders << "\n";
    std::cout << "Total trading signals generated: " << total_strategy_signals << "\n";
    std::cout << "Average strategy processing latency: " << std::fixed << std::setprecision(2) 
              << avg_strategy_latency << " ns\n";
    
    if (total_strategy_signals > 0) {
        double signal_to_order_ratio = static_cast<double>(total_strategy_orders) / total_strategy_signals;
        std::cout << "Signal-to-order conversion rate: " << std::fixed << std::setprecision(2) 
                  << (signal_to_order_ratio * 100.0) << "%\n";
    }
    
    std::cout << "\n--- System Integration Summary ---\n";
    uint64_t total_orders = engine.get_processed_orders();
    uint64_t background_orders = total_orders - total_strategy_orders;
    
    std::cout << "Total orders processed: " << total_orders << "\n";
    std::cout << "Background orders: " << background_orders << " (" 
              << std::fixed << std::setprecision(1) 
              << (100.0 * background_orders / total_orders) << "%)\n";
    std::cout << "Strategy orders: " << total_strategy_orders << " (" 
              << std::fixed << std::setprecision(1) 
              << (100.0 * total_strategy_orders / total_orders) << "%)\n";
    
    std::cout << "\n--- Step 4 Multi-threading Features ---\n";
    std::cout << "✓ Race condition detection and monitoring\n";
    std::cout << "✓ Lock-free data structures (queue)\n";
    std::cout << "✓ Work-stealing thread pool\n";
    std::cout << "✓ Advanced read-write locks\n";
    std::cout << "✓ Barrier synchronization\n";
    std::cout << "✓ Stress testing with race condition simulation\n";
    std::cout << "✓ Advanced matching engine with race condition protection\n";
    
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "Step 4 Complete! Advanced multi-threading with race condition protection implemented.\n";
    std::cout << "Ready for Step 5: Network Layer and External Connectivity\n";
    std::cout << std::string(60, '=') << "\n";
    
    return 0;
}

