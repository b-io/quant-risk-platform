#include <gtest/gtest.h>
#include <qrp/io/json_loader.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <qrp/analytics/risk_service.hpp>
#include <filesystem>

TEST(RiskIntegrationTest, ComputeRiskSamplePortfolio) {
    std::string market_path = "data/market/base_market_v2.json";
    std::string portfolio_path = "data/portfolios/demo_macro_book.json";

    if (!std::filesystem::exists(market_path)) {
        GTEST_SKIP() << "Sample data not found";
    }

    auto market_dto = qrp::io::load_market(market_path);
    auto portfolio_dto = qrp::io::load_portfolio(portfolio_path);

    auto risk_results = qrp::analytics::RiskService::compute_risk(portfolio_dto, market_dto);

    EXPECT_FALSE(risk_results.empty());
    for (const auto& res : risk_results) {
        EXPECT_FALSE(res.trade_id.empty());
        // Parallel risk should be non-zero for swaps/bonds
        EXPECT_NE(res.pv01, 0.0);
        // Bucketed risk sum should be approximately equal to parallel risk
        double bucket_sum = 0.0;
        for (const auto& [tenor, risk] : res.bucketed_risk) {
            bucket_sum += risk;
        }
        EXPECT_NEAR(bucket_sum, res.pv01, 10.0); // Allow for bootstrapping and non-linearity with 1bp bump
    }
}
