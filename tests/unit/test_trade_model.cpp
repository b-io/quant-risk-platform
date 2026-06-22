#include <gtest/gtest.h>
#include <qrp/domain/factors.hpp>
#include <qrp/domain/portfolio.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>

TEST(TradeModelTest, ParsesKnownTradeTypes) {
    EXPECT_EQ(qrp::domain::parse_trade_type("vanilla_swap"), qrp::domain::TradeType::VanillaSwap);
    EXPECT_EQ(qrp::domain::parse_trade_type("fixed_rate_bond"), qrp::domain::TradeType::FixedRateBond);
    EXPECT_EQ(qrp::domain::parse_trade_type("equity_spot"), qrp::domain::TradeType::EquitySpot);
    EXPECT_EQ(qrp::domain::parse_trade_type("fx_forward"), qrp::domain::TradeType::FxForward);
}

TEST(TradeModelTest, ParsesCanonicalAssetClassesAndProductTypes) {
    EXPECT_EQ(qrp::domain::parse_asset_class("rates"), qrp::domain::AssetClass::Rates);
    EXPECT_EQ(qrp::domain::parse_asset_class("FX"), qrp::domain::AssetClass::FX);
    EXPECT_EQ(qrp::domain::parse_asset_class("commodities"), qrp::domain::AssetClass::Commodity);
    EXPECT_EQ(qrp::domain::parse_asset_class("legacy"), qrp::domain::AssetClass::Unknown);

    EXPECT_EQ(qrp::domain::parse_product_type("swaption"), qrp::domain::ProductType::Swaption);
    EXPECT_EQ(qrp::domain::parse_product_type("fx_option"), qrp::domain::ProductType::FxOption);
    EXPECT_EQ(qrp::domain::parse_product_type("commodity_future_option"), qrp::domain::ProductType::CommodityFutureOption);
    EXPECT_EQ(qrp::domain::parse_product_type("legacy"), qrp::domain::ProductType::Unknown);
}

TEST(TradeModelTest, BuildsCanonicalFactorIds) {
    EXPECT_EQ(
        qrp::domain::make_commodity_forward_factor_id("WTI", "3M"),
        "RF:COM:WTI:FWD:3M");
    EXPECT_EQ(
        qrp::domain::make_commodity_vol_factor_id("TTF", "1Y", "ATM"),
        "RF:COMVOL:TTF:1Y:ATM");
    EXPECT_EQ(
        qrp::domain::make_credit_spread_factor_id("CDX_IG", "5Y"),
        "RF:CREDIT:CDX_IG:SPREAD:5Y");
    EXPECT_EQ(
        qrp::domain::make_credit_vol_factor_id("ITRAXX_MAIN", "6M", "100"),
        "RF:CREDITVOL:ITRAXX_MAIN:6M:100");
    EXPECT_EQ(
        qrp::domain::make_equity_spot_factor_id("AAPL"),
        "RF:EQ:AAPL:SPOT");
    EXPECT_EQ(
        qrp::domain::make_equity_vol_factor_id("SPX", "3M", "ATM"),
        "RF:EQVOL:SPX:3M:ATM");
    EXPECT_EQ(
        qrp::domain::make_fx_spot_factor_id("EURUSD"),
        "RF:FX:EURUSD:SPOT");
    EXPECT_EQ(
        qrp::domain::make_fx_vol_factor_id("EURUSD", "1M", "25D_RR"),
        "RF:FXVOL:EURUSD:1M:25D_RR");
    EXPECT_EQ(
        qrp::domain::make_rates_factor_id(qrp::domain::Currency::USD, "OIS", "5Y"),
        "RF:RATES:USD:OIS:5Y");
    EXPECT_EQ(
        qrp::domain::make_rates_vol_factor_id(qrp::domain::Currency::USD, "SWAPTION", "1Y", "5Y"),
        "RF:RATESVOL:USD:SWAPTION:1Y:5Y");
}

TEST(TradeModelTest, ParsesAndSerializesFactorEnums) {
    EXPECT_EQ(
        qrp::domain::parse_factor_type("CommodityForward"),
        qrp::domain::FactorType::CommodityForward);
    EXPECT_EQ(
        qrp::domain::to_string(qrp::domain::FactorType::RateZero),
        "RateZero");
    EXPECT_EQ(
        qrp::domain::parse_shock_measure("BasisPoints"),
        qrp::domain::ShockMeasure::BasisPoints);
    EXPECT_EQ(
        qrp::domain::to_string(qrp::domain::ShockMeasure::VolPoints),
        "VolPoints");

    EXPECT_THROW(qrp::domain::parse_factor_type("LegacyFactor"), std::invalid_argument);
    EXPECT_THROW(qrp::domain::parse_shock_measure("LegacyShock"), std::invalid_argument);
}

TEST(TradeModelTest, RejectsMalformedFactorTokens) {
    EXPECT_THROW(
        qrp::domain::make_rates_factor_id(qrp::domain::Currency::USD, "USD:OIS", "5Y"),
        std::invalid_argument);
    EXPECT_THROW(
        qrp::domain::make_fx_spot_factor_id(""),
        std::invalid_argument);
    EXPECT_THROW(
        qrp::domain::make_rates_factor_id(qrp::domain::Currency::UNKNOWN, "OIS", "5Y"),
        std::invalid_argument);
}

TEST(TradeModelTest, ValidatesCanonicalFactorIds) {
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:COM:WTI:FWD:3M"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:COMVOL:TTF:1Y:ATM"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:CREDIT:CDX_IG:SPREAD:5Y"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:CREDITVOL:ITRAXX_MAIN:6M:100"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:EQ:AAPL:SPOT"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:EQVOL:SPX:3M:ATM"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:FX:EURUSD:SPOT"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:FXVOL:EURUSD:1M:25D_RR"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:RATES:USD:OIS:5Y"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:RATESVOL:USD:SWAPTION:1Y:5Y"));

    EXPECT_FALSE(qrp::domain::is_canonical_factor_id("RF:RATE:USD:OIS:5Y"));
    EXPECT_FALSE(qrp::domain::is_canonical_factor_id("RF:VOL:USD:CAP:1Y"));
    EXPECT_FALSE(qrp::domain::is_canonical_factor_id("RF:RATES:USD::5Y"));
    EXPECT_FALSE(qrp::domain::is_canonical_factor_id("RATES:USD:OIS:5Y"));
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
    EXPECT_EQ(portfolio.trades[0]->asset_class_type, qrp::domain::AssetClass::Equity);
    EXPECT_EQ(portfolio.trades[0]->product_type, qrp::domain::ProductType::EquitySpot);
    EXPECT_EQ(portfolio.trades[0]->type, "equity_spot");
    EXPECT_NE(std::dynamic_pointer_cast<qrp::domain::EquitySpotTrade>(portfolio.trades[0]), nullptr);
}
