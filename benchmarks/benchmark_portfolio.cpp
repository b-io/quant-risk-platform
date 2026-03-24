#include <iostream>
#include <chrono>
#include <qrp/io/json_loader.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/analytics/risk_service.hpp>
#include <fmt/core.h>

int main() {
    fmt::print("--- QRP Benchmark ---\n");

    std::string market_path = "data/market/base_market.json";
    std::string portfolio_path = "data/portfolios/demo_macro_book.json";

    auto market_dto = qrp::io::load_market(market_path);
    auto portfolio_dto = qrp::io::load_portfolio(portfolio_path);

    // Build market
    auto start = std::chrono::high_resolution_clock::now();
    qrp::market::MarketSnapshot market(market_dto);
    auto end = std::chrono::high_resolution_clock::now();
    auto build_market_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    fmt::print("Market build time: {} ms\n", build_market_ms);

    // Pricing
    start = std::chrono::high_resolution_clock::now();
    auto results = qrp::analytics::ValuationService::price_portfolio(portfolio_dto, market);
    end = std::chrono::high_resolution_clock::now();
    auto price_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    fmt::print("Pricing ({} trades) time: {} ms\n", results.size(), price_ms);

    // Risk
    start = std::chrono::high_resolution_clock::now();
    auto risk_results = qrp::analytics::RiskService::compute_risk(portfolio_dto, market_dto);
    end = std::chrono::high_resolution_clock::now();
    auto risk_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    fmt::print("Risk (PV01 + Bucketed) time: {} ms\n", risk_ms);

    return 0;
}
