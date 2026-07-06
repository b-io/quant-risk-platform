// Exercises sample-data valuation through the public loading and analytics services.

#include "test_paths.hpp"

#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/io/json_loader.hpp>
#include <qrp/market/market_snapshot.hpp>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <map>
#include <numeric>
#include <string>

TEST(ValuationIntegrationTest, PriceSamplePortfolio) {
    const auto market_path = qrp::test::data_file({"market", "demo_market.json"});
    const auto portfolio_path = qrp::test::data_file({"portfolios", "demo_portfolio.json"});
    const auto golden_path = qrp::test::data_file({"regression", "demo_golden.json"});

    auto market_dto = qrp::io::load_market(market_path.string());
    qrp::market::MarketSnapshot market(market_dto);

    auto portfolio_dto = qrp::io::load_portfolio(portfolio_path.string());

    qrp::analytics::PricingContext context(market.built_state());
    auto results = qrp::analytics::ValuationService::price_portfolio(portfolio_dto, context);

    std::ifstream golden_stream(golden_path);
    ASSERT_TRUE(golden_stream.is_open()) << "Unable to open " << golden_path.string();

    nlohmann::json golden;
    golden_stream >> golden;
    const auto& expected_valuation = golden.at("valuation");
    const auto& expected_trades = expected_valuation.at("trades");
    const double tolerance = expected_valuation.value("tolerance", 1.0);

    ASSERT_EQ(results.size(), expected_trades.size());

    double total_npv = 0.0;
    std::map<std::string, qrp::analytics::ValuationResult> by_trade;
    for (const auto& res : results) {
        EXPECT_FALSE(res.trade_id.empty());
        EXPECT_FALSE(res.tags.contains("status") && res.tags.at("status") == "failed") << res.status_message;

        total_npv += res.npv;
        by_trade[res.trade_id] = res;
    }

    EXPECT_NEAR(total_npv, expected_valuation.at("total_npv").get<double>(), tolerance);

    for (const auto& [trade_id, expected] : expected_trades.items()) {
        ASSERT_TRUE(by_trade.contains(trade_id)) << "Missing valuation result for " << trade_id;
        const auto& actual = by_trade.at(trade_id);

        EXPECT_EQ(qrp::domain::to_string(actual.asset_class), expected.at("asset_class").get<std::string>())
            << trade_id;
        EXPECT_EQ(qrp::domain::to_string(actual.product_type), expected.at("product_type").get<std::string>())
            << trade_id;
        EXPECT_EQ(qrp::domain::to_string(actual.support_status), expected.at("support_status").get<std::string>())
            << trade_id;
        EXPECT_NEAR(actual.npv, expected.at("npv").get<double>(), tolerance) << trade_id;
    }
}
