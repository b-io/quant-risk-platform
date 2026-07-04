// Exercises sample-data valuation through the public loading and analytics services.

#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/io/json_loader.hpp>
#include <qrp/market/market_snapshot.hpp>

#include <gtest/gtest.h>

#include <filesystem>

TEST(ValuationIntegrationTest, PriceSamplePortfolio) {
    // Note: In a real test environment, we'd ensure these files are available
    // For this environment, we'll assume they are in the project root relative path
    std::string market_path = "data/market/demo_market.json";
    std::string portfolio_path = "data/portfolios/demo_portfolio.json";

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

        // Failed valuations are reported per trade so one bad instrument does not hide all results.
        if (res.tags.contains("status") && res.tags.at("status") == "failed") {
            continue;
        }

        // NPV shouldn't be zero for supported trades.
        // For FX Forwards and Equities, we might have 0 NPV if Spot = RefPrice/ForwardRate,
        // which happens at inception. Let's only expect non-zero for swaps/bonds.
        if (res.trade_id.find("swap") != std::string::npos || res.trade_id.find("bond") != std::string::npos) {
            EXPECT_NE(res.npv, 0.0);
        }
    }
}
