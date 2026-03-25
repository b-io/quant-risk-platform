#include <gtest/gtest.h>
#include <qrp/io/json_loader.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/analytics/pricing_context.hpp>
#include <filesystem>

TEST(ValuationIntegrationTest, PriceSamplePortfolio) {
    // Note: In a real test environment, we'd ensure these files are available
    // For this environment, we'll assume they are in the project root relative path
    std::string market_path = "data/market/base_market_v2.json";
    std::string portfolio_path = "data/portfolios/demo_macro_book.json";

    if (!std::filesystem::exists(market_path)) {
        GTEST_SKIP() << "Sample data not found";
    }

    auto market_dto = qrp::io::load_market(market_path);
    qrp::market::MarketSnapshot market(market_dto);

    auto portfolio_dto = qrp::io::load_portfolio(portfolio_path);

    qrp::analytics::PricingContext context(market.built_state());
    auto results = qrp::analytics::ValuationService::price_portfolio(portfolio_dto, context);

    EXPECT_FALSE(results.empty());
    for (const auto& res : results) {
        EXPECT_FALSE(res.trade_id.empty());
        // Since we are using zero rates of ~4.5-4.9% and swap/bond rates of ~4.7-5.0%,
        // NPV shouldn't be zero.
        EXPECT_NE(res.npv, 0.0);
    }
}
