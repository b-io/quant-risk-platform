#include <qrp/analytics/valuation_service.hpp>
#include <qrp/analytics/risk_service.hpp>
#include <qrp/io/json_loader.hpp>
#include <iostream>
#include <chrono>

int main() {
    try {
        std::cout << "Starting Portfolio Benchmark..." << std::endl;

        auto market_dto = qrp::io::load_market("data/market/base_market.json");
        auto portfolio = qrp::io::load_portfolio("data/portfolios/demo_macro_book.json");
        qrp::market::MarketSnapshot market(market_dto);

        const int iterations = 100;
        
        // 1. Benchmark Valuation
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) {
            auto results = qrp::analytics::ValuationService::price_portfolio(portfolio, market);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        std::cout << "Valuation Benchmark (100 iterations): " << duration << " ms" << std::endl;
        std::cout << "Trades per second: " << (iterations * portfolio.trades.size() * 1000.0 / duration) << std::endl;

        // 2. Benchmark Risk
        start = std::chrono::high_resolution_clock::now();
        auto risk_results = qrp::analytics::RiskService::compute_risk(portfolio, market_dto);
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        std::cout << "Risk Benchmark (PV01 + Bucketed): " << duration << " ms" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
