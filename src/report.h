#pragma once
#include <iosfwd>
#include <string>
#include "backtester.h"

// CSV writers for backtest output (for spreadsheets / plotting).
// Trades: entry_bar,exit_bar,side,entry_price,exit_price,quantity,pnl_pct
void write_trades_csv(std::ostream& out, const BacktestResult& r);
// Equity: bar,equity
void write_equity_csv(std::ostream& out, const BacktestResult& r);
// Convenience: writes <prefix>_trades.csv and <prefix>_equity.csv; returns false on I/O failure.
bool save_report(const std::string& prefix, const BacktestResult& r);
