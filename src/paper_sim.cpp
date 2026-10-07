#include "paper_sim.h"
#include <algorithm>
#include <cmath>

void PaperSim::start(const StrategyParams& params, double volatility_pct, double start_price, uint32_t seed) {
    strategy_ = make_strategy(params);
    strategy_.reset();
    volatility_ = volatility_pct / 100.0;
    price_ = start_price;
    index_ = 0;
    cash_ = kInitialCash;
    position_ = entry_ = realized_ = 0;
    trades_ = wins_ = 0;
    next_id_ = 1;
    rng_.seed(seed);
}

PaperSim::Step PaperSim::snapshot() const {
    Step s;
    s.index = index_;
    s.price = price_;
    s.cash = cash_;
    s.position = position_;
    s.realized = realized_;
    s.equity = cash_ + position_ * price_;
    s.trades = trades_;
    s.wins = wins_;
    return s;
}

PaperSim::Step PaperSim::step() {
    // The first step reports the starting price; later steps move it.
    if (index_ > 0) {
        std::normal_distribution<double> normal(0, 1);
        price_ *= std::exp(volatility_ * normal(rng_));
    }
    const int idx = index_++;

    auto order = std::make_shared<Order>();
    order->order_id = next_id_++;
    order->side = OrderSide::BUY;
    order->price = static_cast<Price>(std::llround(price_ * 100.0));
    order->quantity = 1;
    order->type = OrderType::MARKET;

    Step s;
    for (const auto& o : strategy_.generate({order})) {
        if (!o || s.traded) continue;
        const double qty = static_cast<double>(o->quantity);
        if (o->side == OrderSide::BUY && position_ == 0) {
            double q = std::min(qty, std::floor(cash_ / price_));
            if (q <= 0) continue;
            position_ = q;
            entry_ = price_;
            cash_ -= q * price_;
            s.traded = true;
            s.buy = true;
            s.qty = q;
            s.reason = strategy_.last_reason();
        } else if (o->side == OrderSide::SELL && position_ > 0) {
            s.pnl_abs = (price_ - entry_) * position_;
            s.pnl_pct = (price_ - entry_) / entry_ * 100.0;
            cash_ += position_ * price_;
            realized_ += s.pnl_abs;
            ++trades_;
            if (s.pnl_abs > 0) ++wins_;
            s.traded = true;
            s.buy = false;
            s.qty = position_;
            s.reason = strategy_.last_reason();
            position_ = 0;
            entry_ = 0;
        }
    }

    Step snap = snapshot();
    snap.index = idx;
    snap.traded = s.traded;
    snap.buy = s.buy;
    snap.qty = s.qty;
    snap.pnl_abs = s.pnl_abs;
    snap.pnl_pct = s.pnl_pct;
    snap.reason = s.reason;
    return snap;
}
