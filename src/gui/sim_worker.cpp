#include "sim_worker.h"
#include <algorithm>
#include <cmath>

void SimWorker::start(StrategyParams params, double volatilityPct, int tickMs, double startPrice) {
    stop();
    strategy_ = make_strategy(params);
    strategy_.reset();
    volatility_ = volatilityPct / 100.0;
    price_ = startPrice;
    index_ = 0;
    cash_ = kInitialCash;
    position_ = entry_ = realized_ = 0;
    trades_ = wins_ = 0;
    rng_.seed(std::random_device{}());

    timer_ = std::make_unique<QTimer>();
    connect(timer_.get(), &QTimer::timeout, this, &SimWorker::onTick);
    timer_->start(tickMs);
    emit tick(index_++, price_);
    emit account(kInitialCash, kInitialCash, 0, 0, 0, 0);
}

void SimWorker::stop() {
    if (timer_) {
        timer_->stop();
        timer_.reset();
    }
}

void SimWorker::onTick() {
    std::normal_distribution<double> normal(0, 1);
    price_ *= std::exp(volatility_ * normal(rng_));
    const int idx = index_++;

    auto order = std::make_shared<Order>();
    order->order_id = nextId_++;
    order->side = OrderSide::BUY;
    order->price = static_cast<Price>(std::llround(price_ * 100.0));
    order->quantity = 1;
    order->type = OrderType::MARKET;

    emit tick(idx, price_);

    for (const auto& o : strategy_.generate({order})) {
        if (!o) continue;
        const double qty = static_cast<double>(o->quantity);
        const QString reason = QString::fromStdString(strategy_.last_reason());
        if (o->side == OrderSide::BUY && position_ == 0) {
            double q = std::min(qty, std::floor(cash_ / price_));
            if (q <= 0) continue;
            position_ = q;
            entry_ = price_;
            cash_ -= q * price_;
            emit trade(true, idx, price_, q, 0, 0, reason);
        } else if (o->side == OrderSide::SELL && position_ > 0) {
            double pnl = (price_ - entry_) * position_;
            double pct = (price_ - entry_) / entry_ * 100.0;
            cash_ += position_ * price_;
            realized_ += pnl;
            ++trades_;
            if (pnl > 0) ++wins_;
            emit trade(false, idx, price_, position_, pct, pnl, reason);
            position_ = 0;
            entry_ = 0;
        }
    }
    emit account(cash_ + position_ * price_, cash_, position_, realized_, trades_, wins_);
}
