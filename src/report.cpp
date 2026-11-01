#include "report.h"
#include <fstream>
#include <iomanip>
#include <ostream>

void write_trades_csv(std::ostream& out, const BacktestResult& r) {
    out << "entry_bar,exit_bar,side,entry_price,exit_price,quantity,pnl_pct\n" << std::setprecision(10);
    for (const auto& t : r.trades)
        out << t.entry_bar << ',' << t.exit_bar << ',' << (t.is_buy ? "long" : "short") << ',' << t.entry_price << ','
            << t.exit_price << ',' << t.quantity << ',' << t.pnl_pct << '\n';
}

void write_equity_csv(std::ostream& out, const BacktestResult& r) {
    out << "bar,equity\n" << std::setprecision(10);
    for (size_t i = 0; i < r.equity_curve.size(); ++i) out << i << ',' << r.equity_curve[i] << '\n';
}

bool save_report(const std::string& prefix, const BacktestResult& r) {
    std::ofstream t(prefix + "_trades.csv"), e(prefix + "_equity.csv");
    if (!t || !e) return false;
    write_trades_csv(t, r);
    write_equity_csv(e, r);
    return t.good() && e.good();
}
