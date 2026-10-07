#include "sim_worker.h"
#include <random>

void SimWorker::start(StrategyParams params, double volatilityPct, int tickMs, double startPrice) {
    stop();
    sim_.start(params, volatilityPct, startPrice, std::random_device{}());
    publish(sim_.step());

    timer_ = std::make_unique<QTimer>();
    connect(timer_.get(), &QTimer::timeout, this, &SimWorker::onTick);
    timer_->start(tickMs);
}

void SimWorker::stop() {
    if (timer_) {
        timer_->stop();
        timer_.reset();
    }
}

void SimWorker::onTick() { publish(sim_.step()); }

void SimWorker::publish(const PaperSim::Step& s) {
    emit tick(s.index, s.price);
    if (s.traded) {
        emit trade(s.buy, s.index, s.price, s.qty, s.pnl_pct, s.pnl_abs, QString::fromStdString(s.reason));
    }
    emit account(s.equity, s.cash, s.position, s.realized, s.trades, s.wins);
}
