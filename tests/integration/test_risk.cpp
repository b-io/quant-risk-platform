// Exercises sample-data risk calculation through the public loading and analytics services.

#include "test_paths.hpp"

#include <qrp/analytics/risk_service.hpp>
#include <qrp/io/json_loader.hpp>
#include <qrp/market/market_snapshot.hpp>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <set>
#include <string>

TEST(RiskIntegrationTest, ComputeRiskSamplePortfolio) {
    const auto market_path = qrp::test::data_file({"market", "demo_market.json"});
    const auto portfolio_path = qrp::test::data_file({"portfolios", "demo_portfolio.json"});
    const auto golden_path = qrp::test::data_file({"regression", "demo_golden.json"});

    auto market_dto = qrp::io::load_market(market_path.string());
    auto portfolio_dto = qrp::io::load_portfolio(portfolio_path.string());

    std::ifstream golden_stream(golden_path);
    ASSERT_TRUE(golden_stream.is_open()) << "Unable to open " << golden_path.string();

    nlohmann::json golden;
    golden_stream >> golden;

    // Create rate-style factors for integration-level quote shock plumbing.
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

    const auto& expected_risk = golden.at("risk");
    std::set<std::string> expected_trade_ids;
    for (const auto& [trade_id, expected] : expected_risk.items()) {
        if (trade_id != "tolerance") {
            expected_trade_ids.insert(trade_id);
        }
    }

    ASSERT_EQ(risk_results.size(), expected_trade_ids.size());
    for (const auto& res : risk_results) {
        EXPECT_FALSE(res.trade_id.empty());
        EXPECT_TRUE(expected_trade_ids.contains(res.trade_id)) << "Unexpected risk result for " << res.trade_id;

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
