// Exercises sample-data risk calculation through the public loading and analytics services.

#include <qrp/analytics/risk_service.hpp>
#include <qrp/io/json_loader.hpp>
#include <qrp/market/market_snapshot.hpp>

#include <gtest/gtest.h>

#include <filesystem>

TEST(RiskIntegrationTest, ComputeRiskSamplePortfolio) {
    std::string market_path = "data/market/demo_market.json";
    std::string portfolio_path = "data/portfolios/demo_portfolio.json";

    if (!std::filesystem::exists(market_path)) {
        GTEST_SKIP() << "Sample data not found";
    }

    auto market_dto = qrp::io::load_market(market_path);
    auto portfolio_dto = qrp::io::load_portfolio(portfolio_path);

    // Create minimal factors and bindings for risk calculation
    std::vector<qrp::domain::FactorDefinition> factors;
    std::vector<qrp::domain::FactorBinding> bindings;

    for (const auto& quote : market_dto.quotes) {
        qrp::domain::FactorDefinition f;
        const std::string curve = quote.index_family.empty() ? quote.id : quote.index_family;
        const std::string tenor = quote.tenor.empty() ? "SPOT" : quote.tenor;
        f.factor_id = qrp::domain::make_rates_factor_id(quote.currency, curve, tenor);
        f.factor_type = qrp::domain::FactorType::RateZero;
        f.currency = quote.currency;
        f.tenor = quote.tenor;
        factors.push_back(f);

        qrp::domain::FactorBinding b;
        b.factor_id = f.factor_id;
        b.quote_id = quote.id;
        bindings.push_back(b);
    }

    auto risk_results = qrp::analytics::RiskService::compute_risk(portfolio_dto, market_dto, factors, bindings);

    EXPECT_FALSE(risk_results.empty());
    for (const auto& res : risk_results) {
        EXPECT_FALSE(res.trade_id.empty());

        // Only rates-linked products are expected to carry PV01 in this sample factor setup.
        if (res.trade_id.find("swap") != std::string::npos || res.trade_id.find("bond") != std::string::npos) {
            EXPECT_NE(res.pv01, 0.0);
        }

        // Bucketed risk should reconcile to parallel risk within curve-rebuild tolerance.
        double bucket_sum = 0.0;
        for (const auto& [tenor, risk] : res.bucketed_risk) {
            bucket_sum += risk;
        }
        EXPECT_NEAR(bucket_sum, res.pv01, 10.0);
    }
}
