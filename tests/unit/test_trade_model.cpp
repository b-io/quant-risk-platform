#include <gtest/gtest.h>
#include <qrp/domain/portfolio.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>

TEST(TradeModelTest, ParsesKnownTradeTypes) {
    EXPECT_EQ(qrp::domain::parse_trade_type("vanilla_swap"), qrp::domain::TradeType::VanillaSwap);
    EXPECT_EQ(qrp::domain::parse_trade_type("fixed_rate_bond"), qrp::domain::TradeType::FixedRateBond);
    EXPECT_EQ(qrp::domain::parse_trade_type("equity_spot"), qrp::domain::TradeType::EquitySpot);
    EXPECT_EQ(qrp::domain::parse_trade_type("fx_forward"), qrp::domain::TradeType::FxForward);
}

TEST(TradeModelTest, RejectsUnknownTradeTypes) {
    EXPECT_THROW(qrp::domain::parse_trade_type("legacy_trade"), std::runtime_error);
    EXPECT_THROW(qrp::domain::make_trade("legacy_trade"), std::runtime_error);
}

TEST(TradeModelTest, PortfolioJsonUsesCanonicalTradeFactory) {
    const auto payload = nlohmann::json::parse(R"json(
        {
          "portfolio_id": "P1",
          "trades": [
            {
              "id": "EQ1",
              "asset_class": "equity",
              "type": "equity_spot",
              "currency": "USD",
              "quantity": 10.0,
              "direction": "long",
              "book": "BOOK",
              "strategy": "STRAT",
              "details": {
                "reference_price": 100.0,
                "underlier": "AAPL"
              }
            }
          ]
        }
    )json");

    auto portfolio = payload.get<qrp::domain::Portfolio>();

    ASSERT_EQ(portfolio.trades.size(), 1);
    EXPECT_EQ(portfolio.trades[0]->trade_type, qrp::domain::TradeType::EquitySpot);
    EXPECT_EQ(portfolio.trades[0]->type, "equity_spot");
    EXPECT_NE(std::dynamic_pointer_cast<qrp::domain::EquitySpotTrade>(portfolio.trades[0]), nullptr);
}
