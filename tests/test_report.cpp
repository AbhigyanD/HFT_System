#include <sstream>
#include "test_helpers.h"
#include "report.h"

static BacktestResult sample() {
    BacktestResult r;
    r.trades.push_back({1, 5, 100.0, 110.0, true, 10.0, 20.0});
    r.trades.push_back({7, 9, 120.0, 114.0, false, 5.0, 10.0});
    r.equity_curve = {100000, 100000, 100200, 100150};
    return r;
}

static int lines(const std::string& s) {
    int n = 0;
    for (char c : s) n += c == '\n';
    return n;
}

static void test_trades_csv() {
    std::ostringstream o;
    write_trades_csv(o, sample());
    std::string s = o.str();
    CHECK(s.rfind("entry_bar,exit_bar,side,entry_price,exit_price,quantity,pnl_pct\n", 0) == 0);
    CHECK(lines(s) == 3);
    CHECK(s.find("1,5,long,100,110,20,10") != std::string::npos);
    CHECK(s.find("7,9,short,120,114,10,5") != std::string::npos);
}

static void test_equity_csv() {
    std::ostringstream o;
    write_equity_csv(o, sample());
    std::string s = o.str();
    CHECK(s.rfind("bar,equity\n", 0) == 0);
    CHECK(lines(s) == 5);
    CHECK(s.find("2,100200\n") != std::string::npos);
}

static void test_empty_result() {
    BacktestResult r;
    std::ostringstream t, e;
    write_trades_csv(t, r);
    write_equity_csv(e, r);
    CHECK(lines(t.str()) == 1 && lines(e.str()) == 1);  // headers only
}

static void test_save_report_bad_path() {
    CHECK(!save_report("/nonexistent_dir_xyz/out", sample()));
}

int main() {
    test_trades_csv();
    test_equity_csv();
    test_empty_result();
    test_save_report_bad_path();
    std::printf(g_failures ? "%d check(s) failed\n" : "all report tests passed\n", g_failures);
    return g_failures ? 1 : 0;
}
