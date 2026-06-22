#include <qrp/analytics/valuation_service.hpp>
#include <qrp/domain/factors.hpp>
#include <qrp/analytics/risk_service.hpp>
#include <qrp/io/json_loader.hpp>
#include <iostream>
#include <chrono>
#include <utility>
#include <vector>

namespace {

std::pair<std::vector<qrp::domain::FactorDefinition>, std::vector<qrp::domain::FactorBinding>>
build_rate_quote_factors(const qrp::domain::MarketSnapshot& market_dto) {
    std::vector<qrp::domain::FactorDefinition> factors;
    std::vector<qrp::domain::FactorBinding> bindings;

    for (const auto& quote : market_dto.quotes) {
        if (quote.instrument_type == qrp::domain::QuoteInstrumentType::Future ||
            quote.instrument_type == qrp::domain::QuoteInstrumentType::UNKNOWN) {
            continue;
        }

        qrp::domain::FactorDefinition factor;
        factor.factor_id = quote.id;
        factor.factor_type = qrp::domain::FactorType::RateZero;
        factor.currency = quote.currency;
        factor.tenor = quote.tenor;
        factor.quote_ids = {quote.id};
        factors.push_back(factor);

        qrp::domain::FactorBinding binding;
        binding.factor_id = factor.factor_id;
        binding.quote_id = quote.id;
        bindings.push_back(binding);
    }

    return {factors, bindings};
}

} // namespace

int main() {
    try {
        std::cout << "Starting Portfolio Benchmark..." << std::endl;

        auto market_dto = qrp::io::load_market("data/market/demo_market.json");
        auto portfolio = qrp::io::load_portfolio("data/portfolios/demo_portfolio.json");
        qrp::market::MarketSnapshot market(market_dto);

        const int iterations = 100;
        
        // 1. Benchmark Valuation
        auto start = std::chrono::high_resolution_clock::now();
        std::size_t valuation_count = 0;
        for (int i = 0; i < iterations; ++i) {
            auto results = qrp::analytics::ValuationService::price_portfolio(portfolio, market);
            valuation_count += results.size();
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        std::cout << "Valuation Benchmark (100 iterations): " << duration << " ms" << std::endl;
        std::cout << "Valuation results: " << valuation_count << std::endl;
        std::cout << "Trades per second: " << (iterations * portfolio.trades.size() * 1000.0 / duration) << std::endl;

        // 2. Benchmark Risk
        auto [factors, bindings] = build_rate_quote_factors(market_dto);
        start = std::chrono::high_resolution_clock::now();
        auto risk_results = qrp::analytics::RiskService::compute_risk(portfolio, market_dto, factors, bindings);
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

        std::cout << "Risk Benchmark (PV01 + Bucketed): " << duration << " ms" << std::endl;
        std::cout << "Risk results: " << risk_results.size() << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
