// Verifies canonical trade, product, asset-class, and factor-taxonomy conversions.

#include <qrp/domain/factors.hpp>
#include <qrp/domain/portfolio.hpp>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <stdexcept>

TEST(TradeModelTest, ParsesKnownTradeTypes) {
    EXPECT_EQ(qrp::domain::parse_trade_type("deposit"), qrp::domain::TradeType::Deposit);
    EXPECT_EQ(qrp::domain::parse_trade_type("fra"), qrp::domain::TradeType::Fra);
    EXPECT_EQ(qrp::domain::parse_trade_type("interest_rate_future"), qrp::domain::TradeType::InterestRateFuture);
    EXPECT_EQ(qrp::domain::parse_trade_type("vanilla_swap"), qrp::domain::TradeType::VanillaSwap);
    EXPECT_EQ(qrp::domain::parse_trade_type("ois_swap"), qrp::domain::TradeType::OisSwap);
    EXPECT_EQ(qrp::domain::parse_trade_type("fixed_rate_bond"), qrp::domain::TradeType::FixedRateBond);
    EXPECT_EQ(qrp::domain::parse_trade_type("floating_rate_note"), qrp::domain::TradeType::FloatingRateNote);
    EXPECT_EQ(qrp::domain::parse_trade_type("cap_floor"), qrp::domain::TradeType::CapFloor);
    EXPECT_EQ(qrp::domain::parse_trade_type("european_swaption"), qrp::domain::TradeType::EuropeanSwaption);
    EXPECT_EQ(qrp::domain::parse_trade_type("bermudan_swaption"), qrp::domain::TradeType::BermudanSwaption);
    EXPECT_EQ(qrp::domain::parse_trade_type("fx_spot"), qrp::domain::TradeType::FxSpot);
    EXPECT_EQ(qrp::domain::parse_trade_type("fx_forward"), qrp::domain::TradeType::FxForward);
    EXPECT_EQ(qrp::domain::parse_trade_type("fx_swap"), qrp::domain::TradeType::FxSwap);
    EXPECT_EQ(qrp::domain::parse_trade_type("ndf"), qrp::domain::TradeType::Ndf);
    EXPECT_EQ(qrp::domain::parse_trade_type("fx_option"), qrp::domain::TradeType::FxOption);
    EXPECT_EQ(qrp::domain::parse_trade_type("equity_spot"), qrp::domain::TradeType::EquitySpot);
}

TEST(TradeModelTest, ParsesCanonicalAssetClassesAndProductTypes) {
    EXPECT_EQ(qrp::domain::parse_asset_class("rates"), qrp::domain::AssetClass::Rates);
    EXPECT_EQ(qrp::domain::parse_asset_class("FX"), qrp::domain::AssetClass::FX);
    EXPECT_EQ(qrp::domain::parse_asset_class("commodities"), qrp::domain::AssetClass::Commodity);
    EXPECT_EQ(qrp::domain::parse_asset_class("unknown_asset_class"), qrp::domain::AssetClass::Unknown);

    EXPECT_EQ(qrp::domain::parse_product_type("european_swaption"), qrp::domain::ProductType::EuropeanSwaption);
    EXPECT_EQ(qrp::domain::parse_product_type("bermudan_swaption"), qrp::domain::ProductType::BermudanSwaption);
    EXPECT_EQ(qrp::domain::parse_product_type("interest_rate_future"), qrp::domain::ProductType::InterestRateFuture);
    EXPECT_EQ(qrp::domain::parse_product_type("cds_index"), qrp::domain::ProductType::CdsIndex);
    EXPECT_EQ(qrp::domain::parse_product_type("credit_index_option"), qrp::domain::ProductType::CreditIndexOption);
    EXPECT_EQ(qrp::domain::parse_product_type("fx_spot"), qrp::domain::ProductType::FxSpot);
    EXPECT_EQ(qrp::domain::parse_product_type("fx_forward"), qrp::domain::ProductType::FxForward);
    EXPECT_EQ(qrp::domain::parse_product_type("fx_swap"), qrp::domain::ProductType::FxSwap);
    EXPECT_EQ(qrp::domain::parse_product_type("ndf"), qrp::domain::ProductType::Ndf);
    EXPECT_EQ(qrp::domain::parse_product_type("fx_option"), qrp::domain::ProductType::FxOption);
    EXPECT_EQ(qrp::domain::parse_product_type("commodity_future_option"), qrp::domain::ProductType::CommodityFutureOption);
    EXPECT_EQ(qrp::domain::parse_product_type("unknown_product_type"), qrp::domain::ProductType::Unknown);
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

    EXPECT_THROW(qrp::domain::parse_factor_type("UnsupportedFactor"), std::invalid_argument);
    EXPECT_THROW(qrp::domain::parse_shock_measure("UnsupportedShock"), std::invalid_argument);
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

    EXPECT_FALSE(qrp::domain::is_canonical_factor_id("RF:RATES:USD:OIS"));
    EXPECT_FALSE(qrp::domain::is_canonical_factor_id("RF:RATESVOL:USD:SWAPTION:1Y"));
    EXPECT_FALSE(qrp::domain::is_canonical_factor_id("RF:RATES:USD::5Y"));
    EXPECT_FALSE(qrp::domain::is_canonical_factor_id("RATES:USD:OIS:5Y"));
}

TEST(TradeModelTest, RejectsUnknownTradeTypes) {
    EXPECT_EQ(qrp::domain::parse_product_type("future"), qrp::domain::ProductType::Unknown);
    EXPECT_EQ(qrp::domain::parse_product_type("swaption"), qrp::domain::ProductType::Unknown);
    EXPECT_THROW(qrp::domain::parse_trade_type("unsupported_trade"), std::runtime_error);
    EXPECT_THROW(qrp::domain::parse_trade_type("future"), std::runtime_error);
    EXPECT_THROW(qrp::domain::parse_trade_type("swaption"), std::runtime_error);
    EXPECT_THROW(qrp::domain::make_trade("unsupported_trade"), std::runtime_error);
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
