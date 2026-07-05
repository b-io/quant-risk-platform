// Verifies canonical trade, product, asset-class, and factor-taxonomy conversions.

#include <qrp/domain/factors.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/portfolio.hpp>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace {

namespace domain = qrp::domain;

nlohmann::json trade_payload(
    const std::string& id,
    const std::string& asset_class,
    const std::string& type,
    const std::string& currency,
    nlohmann::json details) {
    return {
        {"id", id},
        {"asset_class", asset_class},
        {"type", type},
        {"currency", currency},
        {"notional", 1000000.0},
        {"quantity", 125.0},
        {"start_date", "2026-03-24"},
        {"maturity_date", "2028-03-24"},
        {"direction", "long"},
        {"book", "BOOK:UNIT"},
        {"strategy", "STRATEGY:UNIT"},
        {"details", std::move(details)}
    };
}

template <typename TradeT>
std::shared_ptr<TradeT> require_trade(const std::map<std::string, std::shared_ptr<domain::Trade>>& trades, const std::string& id) {
    auto it = trades.find(id);
    EXPECT_NE(it, trades.end());
    auto casted = it == trades.end() ? nullptr : std::dynamic_pointer_cast<TradeT>(it->second);
    EXPECT_NE(casted, nullptr);
    return casted;
}

} // namespace

TEST(TradeModelTest, ParsesKnownTradeTypes) {
    EXPECT_EQ(qrp::domain::parse_trade_type("commodity_spot"), qrp::domain::TradeType::CommoditySpot);
    EXPECT_EQ(qrp::domain::parse_trade_type("commodity_forward"), qrp::domain::TradeType::CommodityForward);
    EXPECT_EQ(qrp::domain::parse_trade_type("commodity_future"), qrp::domain::TradeType::CommodityFuture);
    EXPECT_EQ(qrp::domain::parse_trade_type("commodity_future_strip"), qrp::domain::TradeType::CommodityFutureStrip);
    EXPECT_EQ(qrp::domain::parse_trade_type("commodity_future_option"), qrp::domain::TradeType::CommodityFutureOption);
    EXPECT_EQ(qrp::domain::parse_trade_type("commodity_calendar_spread_option"), qrp::domain::TradeType::CommodityCalendarSpreadOption);
    EXPECT_EQ(qrp::domain::parse_trade_type("commodity_swing"), qrp::domain::TradeType::CommoditySwing);
    EXPECT_EQ(qrp::domain::parse_trade_type("credit_bond"), qrp::domain::TradeType::CreditBond);
    EXPECT_EQ(qrp::domain::parse_trade_type("cds"), qrp::domain::TradeType::Cds);
    EXPECT_EQ(qrp::domain::parse_trade_type("cds_index"), qrp::domain::TradeType::CdsIndex);
    EXPECT_EQ(qrp::domain::parse_trade_type("cds_option"), qrp::domain::TradeType::CdsOption);
    EXPECT_EQ(qrp::domain::parse_trade_type("credit_index_option"), qrp::domain::TradeType::CreditIndexOption);
    EXPECT_EQ(qrp::domain::parse_trade_type("equity_spot"), qrp::domain::TradeType::EquitySpot);
    EXPECT_EQ(qrp::domain::parse_trade_type("equity_forward"), qrp::domain::TradeType::EquityForward);
    EXPECT_EQ(qrp::domain::parse_trade_type("equity_future"), qrp::domain::TradeType::EquityFuture);
    EXPECT_EQ(qrp::domain::parse_trade_type("equity_option"), qrp::domain::TradeType::EquityOption);
    EXPECT_EQ(qrp::domain::parse_trade_type("fx_spot"), qrp::domain::TradeType::FxSpot);
    EXPECT_EQ(qrp::domain::parse_trade_type("fx_forward"), qrp::domain::TradeType::FxForward);
    EXPECT_EQ(qrp::domain::parse_trade_type("fx_swap"), qrp::domain::TradeType::FxSwap);
    EXPECT_EQ(qrp::domain::parse_trade_type("ndf"), qrp::domain::TradeType::Ndf);
    EXPECT_EQ(qrp::domain::parse_trade_type("fx_option"), qrp::domain::TradeType::FxOption);
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
}

TEST(TradeModelTest, SerializesAllTradeTypesAndBuildsConcreteTradeDtos) {
    const std::vector<std::tuple<domain::TradeType, std::string, domain::ProductType>> cases = {
        {domain::TradeType::CommoditySpot, "commodity_spot", domain::ProductType::CommoditySpot},
        {domain::TradeType::CommodityForward, "commodity_forward", domain::ProductType::CommodityForward},
        {domain::TradeType::CommodityFuture, "commodity_future", domain::ProductType::CommodityFuture},
        {domain::TradeType::CommodityFutureStrip, "commodity_future_strip", domain::ProductType::CommodityFutureStrip},
        {domain::TradeType::CommodityFutureOption, "commodity_future_option", domain::ProductType::CommodityFutureOption},
        {domain::TradeType::CommodityCalendarSpreadOption, "commodity_calendar_spread_option", domain::ProductType::CommodityCalendarSpreadOption},
        {domain::TradeType::CommoditySwing, "commodity_swing", domain::ProductType::CommoditySwing},
        {domain::TradeType::CreditBond, "credit_bond", domain::ProductType::CreditBond},
        {domain::TradeType::Cds, "cds", domain::ProductType::Cds},
        {domain::TradeType::CdsIndex, "cds_index", domain::ProductType::CdsIndex},
        {domain::TradeType::CdsOption, "cds_option", domain::ProductType::CdsOption},
        {domain::TradeType::CreditIndexOption, "credit_index_option", domain::ProductType::CreditIndexOption},
        {domain::TradeType::EquitySpot, "equity_spot", domain::ProductType::EquitySpot},
        {domain::TradeType::EquityForward, "equity_forward", domain::ProductType::EquityForward},
        {domain::TradeType::EquityFuture, "equity_future", domain::ProductType::EquityFuture},
        {domain::TradeType::EquityOption, "equity_option", domain::ProductType::EquityOption},
        {domain::TradeType::FxSpot, "fx_spot", domain::ProductType::FxSpot},
        {domain::TradeType::FxForward, "fx_forward", domain::ProductType::FxForward},
        {domain::TradeType::FxSwap, "fx_swap", domain::ProductType::FxSwap},
        {domain::TradeType::Ndf, "ndf", domain::ProductType::Ndf},
        {domain::TradeType::FxOption, "fx_option", domain::ProductType::FxOption},
        {domain::TradeType::Deposit, "deposit", domain::ProductType::Deposit},
        {domain::TradeType::Fra, "fra", domain::ProductType::Fra},
        {domain::TradeType::InterestRateFuture, "interest_rate_future", domain::ProductType::InterestRateFuture},
        {domain::TradeType::VanillaSwap, "vanilla_swap", domain::ProductType::VanillaSwap},
        {domain::TradeType::OisSwap, "ois_swap", domain::ProductType::OisSwap},
        {domain::TradeType::FixedRateBond, "fixed_rate_bond", domain::ProductType::FixedRateBond},
        {domain::TradeType::FloatingRateNote, "floating_rate_note", domain::ProductType::FloatingRateNote},
        {domain::TradeType::CapFloor, "cap_floor", domain::ProductType::CapFloor},
        {domain::TradeType::EuropeanSwaption, "european_swaption", domain::ProductType::EuropeanSwaption},
        {domain::TradeType::BermudanSwaption, "bermudan_swaption", domain::ProductType::BermudanSwaption}
    };

    for (const auto& [trade_type, type_name, product_type] : cases) {
        EXPECT_EQ(domain::to_string(trade_type), type_name);
        EXPECT_EQ(domain::product_type_from_trade_type(trade_type), product_type);
        EXPECT_EQ(domain::make_trade(trade_type)->trade_type, trade_type);
        EXPECT_EQ(domain::make_trade(type_name)->type, type_name);
    }

    EXPECT_EQ(domain::to_string(domain::TradeType::Unknown), "unknown");
    EXPECT_EQ(domain::product_type_from_trade_type(domain::TradeType::Unknown), domain::ProductType::Unknown);
    EXPECT_THROW(domain::make_trade(domain::TradeType::Unknown), std::runtime_error);
}

TEST(TradeModelTest, SerializesMarketQuoteTaxonomy) {
    const std::vector<std::pair<domain::QuoteInstrumentType, std::string>> instrument_types = {
        {domain::QuoteInstrumentType::CommodityForward, "CommodityForward"},
        {domain::QuoteInstrumentType::CommodityFuture, "CommodityFuture"},
        {domain::QuoteInstrumentType::CommoditySpot, "CommoditySpot"},
        {domain::QuoteInstrumentType::CommodityVol, "CommodityVol"},
        {domain::QuoteInstrumentType::ConvenienceYield, "ConvenienceYield"},
        {domain::QuoteInstrumentType::Bond, "Bond"},
        {domain::QuoteInstrumentType::BondPrice, "BondPrice"},
        {domain::QuoteInstrumentType::BondSpread, "BondSpread"},
        {domain::QuoteInstrumentType::CDS, "CDS"},
        {domain::QuoteInstrumentType::CreditIndex, "CreditIndex"},
        {domain::QuoteInstrumentType::CreditSpread, "CreditSpread"},
        {domain::QuoteInstrumentType::HazardRate, "HazardRate"},
        {domain::QuoteInstrumentType::RecoveryRate, "RecoveryRate"},
        {domain::QuoteInstrumentType::BorrowRate, "BorrowRate"},
        {domain::QuoteInstrumentType::DividendYield, "DividendYield"},
        {domain::QuoteInstrumentType::EquitySpot, "EquitySpot"},
        {domain::QuoteInstrumentType::EquityVol, "EquityVol"},
        {domain::QuoteInstrumentType::FXForward, "FXForward"},
        {domain::QuoteInstrumentType::FXForwardPoint, "FXForwardPoint"},
        {domain::QuoteInstrumentType::FXSpot, "FXSpot"},
        {domain::QuoteInstrumentType::FXVol, "FXVol"},
        {domain::QuoteInstrumentType::Future, "Future"},
        {domain::QuoteInstrumentType::CapFloorVol, "CapFloorVol"},
        {domain::QuoteInstrumentType::Deposit, "Deposit"},
        {domain::QuoteInstrumentType::FRA, "FRA"},
        {domain::QuoteInstrumentType::InterestRateFuture, "InterestRateFuture"},
        {domain::QuoteInstrumentType::IRS, "IRS"},
        {domain::QuoteInstrumentType::OIS, "OIS"},
        {domain::QuoteInstrumentType::SwaptionVol, "SwaptionVol"}
    };
    for (const auto& [type, label] : instrument_types) {
        EXPECT_EQ(domain::to_string(type), label);
        EXPECT_EQ(domain::parse_quote_instrument_type(label), type);
    }
    EXPECT_EQ(domain::to_string(domain::QuoteInstrumentType::UNKNOWN), "UNKNOWN");
    EXPECT_EQ(domain::parse_quote_instrument_type("UnsupportedQuoteInstrument"), domain::QuoteInstrumentType::UNKNOWN);

    const std::vector<std::pair<domain::QuoteType, std::string>> quote_types = {
        {domain::QuoteType::CommodityForward, "CommodityForward"},
        {domain::QuoteType::BondYield, "BondYield"},
        {domain::QuoteType::CreditSpread, "CreditSpread"},
        {domain::QuoteType::HazardRate, "HazardRate"},
        {domain::QuoteType::RecoveryRate, "RecoveryRate"},
        {domain::QuoteType::BorrowRate, "BorrowRate"},
        {domain::QuoteType::DividendYield, "DividendYield"},
        {domain::QuoteType::EquitySpot, "EquitySpot"},
        {domain::QuoteType::FXForward, "FXForward"},
        {domain::QuoteType::FXForwardPoint, "FXForwardPoint"},
        {domain::QuoteType::FXSpot, "FXSpot"},
        {domain::QuoteType::Future, "Future"},
        {domain::QuoteType::Price, "Price"},
        {domain::QuoteType::Volatility, "Volatility"},
        {domain::QuoteType::BasisSwap, "BasisSwap"},
        {domain::QuoteType::Deposit, "Deposit"},
        {domain::QuoteType::FRA, "FRA"},
        {domain::QuoteType::InterestRateFuture, "InterestRateFuture"},
        {domain::QuoteType::IRS, "IRS"},
        {domain::QuoteType::OIS, "OIS"},
        {domain::QuoteType::Swap, "Swap"}
    };
    for (const auto& [type, label] : quote_types) {
        EXPECT_EQ(domain::to_string(type), label);
        EXPECT_EQ(domain::parse_quote_type(label), type);
    }
    EXPECT_EQ(domain::to_string(domain::QuoteType::UNKNOWN), "UNKNOWN");
    EXPECT_EQ(domain::parse_quote_type("UnsupportedQuoteType"), domain::QuoteType::UNKNOWN);
}

TEST(TradeModelTest, ParsesCanonicalAssetClassesAndProductTypes) {
    EXPECT_EQ(qrp::domain::parse_asset_class("rates"), qrp::domain::AssetClass::Rates);
    EXPECT_EQ(qrp::domain::parse_asset_class("FX"), qrp::domain::AssetClass::FX);
    EXPECT_EQ(qrp::domain::parse_asset_class("commodities"), qrp::domain::AssetClass::Commodity);
    EXPECT_EQ(qrp::domain::parse_asset_class("unknown_asset_class"), qrp::domain::AssetClass::Unknown);

    EXPECT_EQ(qrp::domain::parse_product_type("commodity_spot"), qrp::domain::ProductType::CommoditySpot);
    EXPECT_EQ(qrp::domain::parse_product_type("commodity_forward"), qrp::domain::ProductType::CommodityForward);
    EXPECT_EQ(qrp::domain::parse_product_type("commodity_future_strip"), qrp::domain::ProductType::CommodityFutureStrip);
    EXPECT_EQ(qrp::domain::parse_product_type("commodity_calendar_spread_option"), qrp::domain::ProductType::CommodityCalendarSpreadOption);
    EXPECT_EQ(qrp::domain::parse_product_type("commodity_future_option"), qrp::domain::ProductType::CommodityFutureOption);
    EXPECT_EQ(qrp::domain::parse_product_type("credit_bond"), qrp::domain::ProductType::CreditBond);
    EXPECT_EQ(qrp::domain::parse_product_type("cds"), qrp::domain::ProductType::Cds);
    EXPECT_EQ(qrp::domain::parse_product_type("cds_index"), qrp::domain::ProductType::CdsIndex);
    EXPECT_EQ(qrp::domain::parse_product_type("cds_option"), qrp::domain::ProductType::CdsOption);
    EXPECT_EQ(qrp::domain::parse_product_type("credit_index_option"), qrp::domain::ProductType::CreditIndexOption);
    EXPECT_EQ(qrp::domain::parse_product_type("equity_forward"), qrp::domain::ProductType::EquityForward);
    EXPECT_EQ(qrp::domain::parse_product_type("equity_future"), qrp::domain::ProductType::EquityFuture);
    EXPECT_EQ(qrp::domain::parse_product_type("equity_option"), qrp::domain::ProductType::EquityOption);
    EXPECT_EQ(qrp::domain::parse_product_type("fx_spot"), qrp::domain::ProductType::FxSpot);
    EXPECT_EQ(qrp::domain::parse_product_type("fx_forward"), qrp::domain::ProductType::FxForward);
    EXPECT_EQ(qrp::domain::parse_product_type("fx_swap"), qrp::domain::ProductType::FxSwap);
    EXPECT_EQ(qrp::domain::parse_product_type("ndf"), qrp::domain::ProductType::Ndf);
    EXPECT_EQ(qrp::domain::parse_product_type("fx_option"), qrp::domain::ProductType::FxOption);
    EXPECT_EQ(qrp::domain::parse_product_type("interest_rate_future"), qrp::domain::ProductType::InterestRateFuture);
    EXPECT_EQ(qrp::domain::parse_product_type("european_swaption"), qrp::domain::ProductType::EuropeanSwaption);
    EXPECT_EQ(qrp::domain::parse_product_type("bermudan_swaption"), qrp::domain::ProductType::BermudanSwaption);
    EXPECT_EQ(qrp::domain::parse_product_type("unknown_product_type"), qrp::domain::ProductType::Unknown);
}

TEST(TradeModelTest, SerializesAssetClassesAndDeclaredProductTypes) {
    const std::vector<std::pair<domain::AssetClass, std::string>> asset_classes = {
        {domain::AssetClass::Commodity, "commodity"},
        {domain::AssetClass::Credit, "credit"},
        {domain::AssetClass::Equity, "equity"},
        {domain::AssetClass::FX, "fx"},
        {domain::AssetClass::Inflation, "inflation"},
        {domain::AssetClass::Rates, "rates"},
        {domain::AssetClass::Unknown, "unknown"}
    };
    for (const auto& [asset_class, label] : asset_classes) {
        EXPECT_EQ(domain::to_string(asset_class), label);
    }

    const auto product_types = domain::all_product_types();
    EXPECT_EQ(product_types.front(), domain::ProductType::CommoditySpot);
    EXPECT_EQ(product_types.back(), domain::ProductType::Unknown);
    EXPECT_NE(
        std::find(product_types.begin(), product_types.end(), domain::ProductType::CreditIndexOption),
        product_types.end());
}

TEST(TradeModelTest, BuildsCanonicalFactorIds) {
    EXPECT_EQ(
        qrp::domain::make_commodity_spot_factor_id("WTI"),
        "RF:COM:WTI:SPOT");
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
        qrp::domain::make_credit_recovery_factor_id("CDX_IG"),
        "RF:CREDIT:CDX_IG:RECOVERY:SPOT");
    EXPECT_EQ(
        qrp::domain::make_credit_vol_factor_id("ITRAXX_MAIN", "6M", "100"),
        "RF:CREDITVOL:ITRAXX_MAIN:6M:100");
    EXPECT_EQ(
        qrp::domain::make_equity_spot_factor_id("AAPL"),
        "RF:EQ:AAPL:SPOT");
    EXPECT_EQ(
        qrp::domain::make_equity_carry_factor_id("AAPL", "DIVYLD", "1Y"),
        "RF:EQ:AAPL:DIVYLD:1Y");
    EXPECT_EQ(
        qrp::domain::make_equity_forward_factor_id("SPX", "FUT", "6M"),
        "RF:EQ:SPX:FUT:6M");
    EXPECT_EQ(
        qrp::domain::make_equity_vol_factor_id("SPX", "3M", "ATM"),
        "RF:EQVOL:SPX:3M:ATM");
    EXPECT_EQ(
        qrp::domain::make_fx_spot_factor_id("EURUSD"),
        "RF:FX:EURUSD:SPOT");
    EXPECT_EQ(
        qrp::domain::make_fx_forward_points_factor_id("EURUSD", "6M"),
        "RF:FX:EURUSD:FWDPTS_6M");
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
    const std::vector<std::pair<domain::FactorType, std::string>> factor_types = {
        {domain::FactorType::CommodityForward, "CommodityForward"},
        {domain::FactorType::CommoditySpot, "CommoditySpot"},
        {domain::FactorType::CreditRecovery, "CreditRecovery"},
        {domain::FactorType::CreditSpread, "CreditSpread"},
        {domain::FactorType::HazardRate, "HazardRate"},
        {domain::FactorType::EquityBorrowRate, "EquityBorrowRate"},
        {domain::FactorType::EquityDividendYield, "EquityDividendYield"},
        {domain::FactorType::EquityForward, "EquityForward"},
        {domain::FactorType::EquitySpot, "EquitySpot"},
        {domain::FactorType::FXForwardPoint, "FXForwardPoint"},
        {domain::FactorType::FXSpot, "FXSpot"},
        {domain::FactorType::Custom, "Custom"},
        {domain::FactorType::Volatility, "Volatility"},
        {domain::FactorType::BasisSpread, "BasisSpread"},
        {domain::FactorType::RateForward, "RateForward"},
        {domain::FactorType::RateZero, "RateZero"}
    };
    for (const auto& [factor_type, label] : factor_types) {
        EXPECT_EQ(domain::parse_factor_type(label), factor_type);
        EXPECT_EQ(domain::to_string(factor_type), label);
    }

    EXPECT_EQ(
        qrp::domain::parse_shock_measure("BasisPoints"),
        qrp::domain::ShockMeasure::BasisPoints);
    EXPECT_EQ(
        qrp::domain::to_string(qrp::domain::ShockMeasure::VolPoints),
        "VolPoints");

    EXPECT_THROW(qrp::domain::parse_factor_type("UnsupportedFactor"), std::invalid_argument);
    EXPECT_THROW(qrp::domain::parse_shock_measure("UnsupportedShock"), std::invalid_argument);
}

TEST(TradeModelTest, FactorIdHelpersValidateAndJoinTokens) {
    EXPECT_EQ(
        qrp::domain::factor_id_detail::join({"RF", "RATES", "USD", "OIS", "5Y"}),
        "RF:RATES:USD:OIS:5Y");
    EXPECT_EQ(qrp::domain::factor_id_detail::currency_token(domain::Currency::USD), "USD");
    EXPECT_THROW(qrp::domain::factor_id_detail::currency_token(domain::Currency::UNKNOWN), std::invalid_argument);
}

TEST(TradeModelTest, RejectsMalformedFactorTokens) {
    EXPECT_THROW(
        qrp::domain::make_rates_factor_id(qrp::domain::Currency::USD, "USD:OIS", "5Y"),
        std::invalid_argument);
    EXPECT_THROW(
        qrp::domain::make_fx_spot_factor_id(""),
        std::invalid_argument);
    EXPECT_THROW(
        qrp::domain::make_equity_carry_factor_id("AAPL", "DVD", "1Y"),
        std::invalid_argument);
    EXPECT_THROW(
        qrp::domain::make_equity_forward_factor_id("SPX", "FUTURE", "6M"),
        std::invalid_argument);
    EXPECT_THROW(
        qrp::domain::make_rates_factor_id(qrp::domain::Currency::UNKNOWN, "OIS", "5Y"),
        std::invalid_argument);
}

TEST(TradeModelTest, ValidatesCanonicalFactorIds) {
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:COM:WTI:SPOT"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:COM:WTI:FWD:3M"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:COMVOL:TTF:1Y:ATM"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:CREDIT:CDX_IG:SPREAD:5Y"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:CREDIT:CDX_IG:RECOVERY:SPOT"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:CREDITVOL:ITRAXX_MAIN:6M:100"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:EQ:AAPL:SPOT"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:EQ:AAPL:DIVYLD:1Y"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:EQ:AAPL:BORROW:1Y"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:EQ:SPX:FUT:6M"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:EQVOL:SPX:3M:ATM"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:FX:EURUSD:SPOT"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:FX:EURUSD:FWDPTS_6M"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:FXVOL:EURUSD:1M:25D_RR"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:RATES:USD:OIS:5Y"));
    EXPECT_TRUE(qrp::domain::is_canonical_factor_id("RF:RATESVOL:USD:SWAPTION:1Y:5Y"));

    EXPECT_FALSE(qrp::domain::is_canonical_factor_id("RF:COM:WTI:PRICE:3M"));
    EXPECT_FALSE(qrp::domain::is_canonical_factor_id("RF:CREDIT:CDX_IG:RECOVERY"));
    EXPECT_FALSE(qrp::domain::is_canonical_factor_id("RF:EQ:AAPL:DVD:1Y"));
    EXPECT_FALSE(qrp::domain::is_canonical_factor_id("RF:FX:EURUSD:FWDPTS"));
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

TEST(TradeModelTest, PortfolioJsonParsesAllSupportedTradeEconomics) {
    nlohmann::json payload = {
        {"portfolio_id", "P_ALL"},
        {"trades", nlohmann::json::array({
            trade_payload("T01", "rates", "deposit", "USD", {{"rate", 0.0525}}),
            trade_payload("T02", "rates", "fra", "USD", {{"rate", 0.0475}, {"index_family", "IBOR_3M"}}),
            trade_payload("T03", "rates", "interest_rate_future", "USD", {
                {"contract_size", 2500.0},
                {"index_family", "SOFR_3M"},
                {"quote_id", "SFRZ6"},
                {"trade_price", 95.125}
            }),
            trade_payload("T04", "rates", "vanilla_swap", "USD", {
                {"fixed_rate", 0.045},
                {"floating_index", "USD_LIBOR_3M"}
            }),
            trade_payload("T05", "rates", "ois_swap", "USD", {
                {"rate", 0.043},
                {"floating_index", "SOFR"},
                {"spread", 0.0005}
            }),
            trade_payload("T06", "rates", "fixed_rate_bond", "USD", {
                {"coupon_rate", 0.050},
                {"frequency", "Annual"}
            }),
            trade_payload("T07", "rates", "floating_rate_note", "USD", {
                {"index_family", "IBOR_3M"},
                {"frequency", "Quarterly"},
                {"spread", 0.0012}
            }),
            trade_payload("T08", "rates", "cap_floor", "USD", {
                {"option_type", "floor"},
                {"index_family", "IBOR_3M"},
                {"strike", 0.035},
                {"volatility", 0.21},
                {"vol_quote_id", "USD_CAP_VOL_2Y"}
            }),
            trade_payload("T09", "rates", "european_swaption", "USD", {
                {"strike", 0.041},
                {"index_family", "IBOR_3M"},
                {"expiry_date", "2027-03-24"},
                {"volatility", 0.19},
                {"vol_quote_id", "USD_SWAPTION_VOL_1Y_5Y"}
            }),
            trade_payload("T10", "rates", "bermudan_swaption", "USD", {
                {"exercise_dates", std::vector<std::string>{"2027-03-24", "2028-03-24"}},
                {"strike", 0.040},
                {"index_family", "IBOR_6M"},
                {"mean_reversion", 0.025},
                {"volatility", 0.18},
                {"vol_quote_id", "USD_BERM_VOL"}
            }),
            trade_payload("T11", "fx", "fx_spot", "USD", {
                {"base_currency", "EUR"},
                {"quote_currency", "USD"},
                {"spot_rate", 1.085},
                {"quote_id", "EURUSD"}
            }),
            trade_payload("T12", "fx", "fx_forward", "USD", {
                {"base_currency", "EUR"},
                {"quote_currency", "USD"},
                {"forward_rate", 1.1025},
                {"forward_points_quote_id", "EURUSD_FWDPTS_6M"},
                {"forward_quote_id", "EURUSD_FWD_6M"},
                {"quote_id", "EURUSD"}
            }),
            trade_payload("T13", "fx", "fx_swap", "USD", {
                {"base_currency", "EUR"},
                {"quote_currency", "USD"},
                {"spot_rate", 1.085},
                {"forward_rate", 1.101},
                {"far_forward_points_quote_id", "EURUSD_FWDPTS_1Y"},
                {"far_forward_quote_id", "EURUSD_FWD_1Y"},
                {"near_forward_points_quote_id", "EURUSD_FWDPTS_1M"},
                {"near_forward_quote_id", "EURUSD_FWD_1M"},
                {"quote_id", "EURUSD"}
            }),
            trade_payload("T14", "fx", "ndf", "USD", {
                {"base_currency", "BRL"},
                {"quote_currency", "USD"},
                {"forward_rate", 0.205},
                {"fixing_date", "2026-09-28"},
                {"fixing_quote_id", "BRLUSD_FIX"},
                {"forward_points_quote_id", "BRLUSD_FWDPTS"},
                {"forward_quote_id", "BRLUSD_FWD"},
                {"quote_id", "BRLUSD"}
            }),
            trade_payload("T15", "fx", "fx_option", "USD", {
                {"base_currency", "EUR"},
                {"quote_currency", "USD"},
                {"strike", 1.12},
                {"volatility", 0.105},
                {"option_expiry_date", "2026-09-24"},
                {"call_put", "put"},
                {"maturity_date", "2026-09-26"},
                {"quote_id", "EURUSD"},
                {"vol_quote_id", "EURUSD_VOL_6M"}
            }),
            trade_payload("T16", "credit", "credit_bond", "USD", {
                {"fixed_rate", 0.0575},
                {"z_spread", 0.012},
                {"frequency", "Semiannual"},
                {"reference_entity", "ACME"},
                {"quote_id", "ACME_BOND_SPREAD_5Y"}
            }),
            trade_payload("T17", "credit", "cds", "USD", {
                {"running_spread", 0.0100},
                {"frequency", "Quarterly"},
                {"reference_entity", "ACME"},
                {"recovery_quote_id", "ACME_RECOVERY"},
                {"recovery_rate", 0.38},
                {"cds_quote_id", "ACME_CDS_5Y"}
            }),
            trade_payload("T18", "credit", "cds_index", "USD", {
                {"spread", 0.0080},
                {"frequency", "Quarterly"},
                {"index_factor", 0.97},
                {"index_name", "CDX_IG"},
                {"recovery_quote_id", "CDX_RECOVERY"},
                {"recovery_rate", 0.40},
                {"cds_quote_id", "CDX_IG_5Y"}
            }),
            trade_payload("T19", "credit", "cds_option", "USD", {
                {"option_expiry_date", "2026-09-24"},
                {"frequency", "Quarterly"},
                {"underlier", "ACME"},
                {"payer_receiver", "receiver"},
                {"recovery_quote_id", "ACME_RECOVERY"},
                {"recovery_rate", 0.38},
                {"cds_quote_id", "ACME_CDS_5Y"},
                {"strike_rate", 0.0110},
                {"volatility", 0.36},
                {"vol_quote_id", "ACME_CDS_VOL_6M"}
            }),
            trade_payload("T20", "credit", "credit_index_option", "USD", {
                {"option_expiry_date", "2026-09-24"},
                {"frequency", "Quarterly"},
                {"index_factor", 0.95},
                {"underlier", "CDX_IG"},
                {"payer_receiver", "call"},
                {"recovery_quote_id", "CDX_RECOVERY"},
                {"recovery_rate", 0.40},
                {"cds_quote_id", "CDX_IG_5Y"},
                {"strike_rate", 0.0090},
                {"volatility", 0.30},
                {"vol_quote_id", "CDX_IG_VOL_6M"}
            }),
            trade_payload("T21", "equity", "equity_spot", "USD", {
                {"reference_price", 187.20},
                {"underlier", "AAPL"}
            })
        })}
    };

    auto portfolio = payload.get<domain::Portfolio>();
    ASSERT_EQ(portfolio.trades.size(), 21U);

    std::map<std::string, std::shared_ptr<domain::Trade>> trades_by_id;
    for (const auto& trade : portfolio.trades) {
        trades_by_id.emplace(trade->id, trade);
        EXPECT_EQ(trade->book, "BOOK:UNIT");
        EXPECT_EQ(trade->strategy, "STRATEGY:UNIT");
    }

    EXPECT_DOUBLE_EQ(require_trade<domain::DepositTrade>(trades_by_id, "T01")->deposit_rate, 0.0525);
    EXPECT_EQ(require_trade<domain::FraTrade>(trades_by_id, "T02")->floating_index, "IBOR_3M");
    EXPECT_EQ(require_trade<domain::InterestRateFutureTrade>(trades_by_id, "T03")->future_quote_id, "SFRZ6");
    EXPECT_DOUBLE_EQ(require_trade<domain::VanillaSwapTrade>(trades_by_id, "T04")->fixed_rate, 0.045);
    EXPECT_EQ(require_trade<domain::OisSwapTrade>(trades_by_id, "T05")->overnight_index, "SOFR");
    EXPECT_EQ(require_trade<domain::FixedRateBondTrade>(trades_by_id, "T06")->frequency, "Annual");
    EXPECT_DOUBLE_EQ(require_trade<domain::FloatingRateNoteTrade>(trades_by_id, "T07")->spread, 0.0012);
    EXPECT_EQ(require_trade<domain::CapFloorTrade>(trades_by_id, "T08")->cap_floor_type, "floor");
    EXPECT_EQ(require_trade<domain::EuropeanSwaptionTrade>(trades_by_id, "T09")->option_expiry_date, "2027-03-24");
    EXPECT_EQ(require_trade<domain::BermudanSwaptionTrade>(trades_by_id, "T10")->exercise_dates.size(), 2U);
    EXPECT_EQ(require_trade<domain::FxSpotTrade>(trades_by_id, "T11")->spot_quote_id, "EURUSD");
    EXPECT_EQ(require_trade<domain::FxForwardTrade>(trades_by_id, "T12")->forward_quote_id, "EURUSD_FWD_6M");
    EXPECT_EQ(require_trade<domain::FxSwapTrade>(trades_by_id, "T13")->far_forward_quote_id, "EURUSD_FWD_1Y");
    EXPECT_EQ(require_trade<domain::NdfTrade>(trades_by_id, "T14")->fixing_quote_id, "BRLUSD_FIX");
    EXPECT_EQ(require_trade<domain::FxOptionTrade>(trades_by_id, "T15")->settlement_date, "2026-09-26");
    EXPECT_EQ(require_trade<domain::CreditBondTrade>(trades_by_id, "T16")->issuer, "ACME");
    EXPECT_EQ(require_trade<domain::CdsTrade>(trades_by_id, "T17")->recovery_quote_id, "ACME_RECOVERY");
    EXPECT_DOUBLE_EQ(require_trade<domain::CdsIndexTrade>(trades_by_id, "T18")->index_factor, 0.97);
    EXPECT_EQ(require_trade<domain::CdsOptionTrade>(trades_by_id, "T19")->option_type, "receiver");
    EXPECT_EQ(require_trade<domain::CreditIndexOptionTrade>(trades_by_id, "T20")->index_name, "CDX_IG");
    EXPECT_EQ(require_trade<domain::EquitySpotTrade>(trades_by_id, "T21")->underlier, "AAPL");
}

TEST(TradeModelTest, PortfolioJsonParsesCommodityAndEquityTradeEconomics) {
    nlohmann::json payload = {
        {"portfolio_id", "P_COMMODITY_EQUITY"},
        {"trades", nlohmann::json::array({
            trade_payload("C01", "commodity", "commodity_spot", "USD", {
                {"underlier", "WTI"}, {"reference_price", 77.0}, {"quote_id", "WTI_SPOT"}, {"unit", "bbl"}
            }),
            trade_payload("C02", "commodity", "commodity_forward", "USD", {
                {"underlier", "WTI"}, {"contract_price", 78.0}, {"quote_id", "WTI_FWD_6M"}, {"tenor", "6M"}, {"unit", "bbl"}
            }),
            trade_payload("C03", "commodity", "commodity_future", "USD", {
                {"underlier", "WTI"}, {"contract_size", 1000.0}, {"trade_price", 78.0}, {"quote_id", "WTI_FUT_6M"}, {"tenor", "6M"}
            }),
            trade_payload("C04", "commodity", "commodity_future_strip", "USD", {
                {"underlier", "WTI"},
                {"contract_size", 1000.0},
                {"quote_ids", std::vector<std::string>{"WTI_FUT_6M", "WTI_FUT_9M"}},
                {"weights", std::vector<double>{0.4, 0.6}},
                {"trade_price", 79.0}
            }),
            trade_payload("C05", "commodity", "commodity_future_option", "USD", {
                {"underlier", "WTI"}, {"strike", 80.0}, {"option_expiry_date", "2026-09-24"},
                {"quote_id", "WTI_FUT_6M"}, {"vol_quote_id", "WTI_VOL_6M_ATM"}, {"option_type", "call"}
            }),
            trade_payload("C06", "commodity", "commodity_calendar_spread_option", "USD", {
                {"underlier", "WTI"}, {"strike", 0.25}, {"option_expiry_date", "2026-09-24"},
                {"near_quote_id", "WTI_FUT_6M"}, {"far_quote_id", "WTI_FUT_9M"}, {"volatility", 0.20}
            }),
            trade_payload("C07", "commodity", "commodity_swing", "USD", {
                {"underlier", "TTF"}, {"min_quantity", 500.0}, {"max_quantity", 1000.0}, {"strike", 31.0},
                {"quote_ids", std::vector<std::string>{"TTF_FWD_Q1", "TTF_FWD_Q2"}},
                {"exercise_dates", std::vector<std::string>{"2026-06-24", "2026-09-24"}},
                {"volatility", 0.40}
            }),
            trade_payload("E01", "equity", "equity_forward", "USD", {
                {"underlier", "AAPL"}, {"forward_price", 190.0}, {"dividend_quote_id", "AAPL_DIVYLD_1Y"},
                {"borrow_quote_id", "AAPL_BORROW_1Y"}
            }),
            trade_payload("E02", "equity", "equity_future", "USD", {
                {"underlier", "SPX"}, {"contract_size", 50.0}, {"trade_price", 5350.0}, {"quote_id", "ES_FUT_6M"}
            }),
            trade_payload("E03", "equity", "equity_option", "USD", {
                {"underlier", "AAPL"}, {"strike", 185.0}, {"option_expiry_date", "2026-09-24"},
                {"exercise_style", "american"}, {"call_put", "put"}, {"vol_quote_id", "AAPL_VOL_6M_ATM"}
            })
        })}
    };

    auto portfolio = payload.get<domain::Portfolio>();
    ASSERT_EQ(portfolio.trades.size(), 10U);

    std::map<std::string, std::shared_ptr<domain::Trade>> trades_by_id;
    for (const auto& trade : portfolio.trades) {
        trades_by_id.emplace(trade->id, trade);
    }

    EXPECT_EQ(require_trade<domain::CommoditySpotTrade>(trades_by_id, "C01")->spot_quote_id, "WTI_SPOT");
    EXPECT_EQ(require_trade<domain::CommodityForwardTrade>(trades_by_id, "C02")->forward_quote_id, "WTI_FWD_6M");
    EXPECT_DOUBLE_EQ(require_trade<domain::CommodityFutureTrade>(trades_by_id, "C03")->contract_size, 1000.0);
    EXPECT_EQ(require_trade<domain::CommodityFutureStripTrade>(trades_by_id, "C04")->future_quote_ids.size(), 2U);
    EXPECT_EQ(require_trade<domain::CommodityFutureOptionTrade>(trades_by_id, "C05")->volatility_quote_id, "WTI_VOL_6M_ATM");
    EXPECT_EQ(require_trade<domain::CommodityCalendarSpreadOptionTrade>(trades_by_id, "C06")->far_future_quote_id, "WTI_FUT_9M");
    EXPECT_EQ(require_trade<domain::CommoditySwingTrade>(trades_by_id, "C07")->exercise_dates.size(), 2U);
    EXPECT_EQ(require_trade<domain::EquityForwardTrade>(trades_by_id, "E01")->dividend_yield_quote_id, "AAPL_DIVYLD_1Y");
    EXPECT_DOUBLE_EQ(require_trade<domain::EquityFutureTrade>(trades_by_id, "E02")->contract_size, 50.0);
    EXPECT_EQ(require_trade<domain::EquityOptionTrade>(trades_by_id, "E03")->exercise_style, "american");
}

TEST(TradeModelTest, CoversPortfolioDetailHelpersAndTaxonomyFallbacks) {
    const nlohmann::json details = {
        {"exercise_dates", std::vector<std::string>{"2027-03-24", "2028-03-24"}},
        {"index_family", "IBOR_6M"},
        {"rate", 0.0525}
    };

    EXPECT_DOUBLE_EQ(domain::portfolio_detail::optional_double(details, {"missing", "rate"}, 0.01), 0.0525);
    EXPECT_DOUBLE_EQ(domain::portfolio_detail::optional_double(details, {"missing"}, 0.01), 0.01);
    EXPECT_EQ(domain::portfolio_detail::optional_string(details, {"missing", "index_family"}, "OIS"), "IBOR_6M");
    EXPECT_EQ(domain::portfolio_detail::optional_string(details, {"missing"}, "OIS"), "OIS");
    EXPECT_EQ(domain::portfolio_detail::optional_string_vector(details, {"exercise_dates"}).size(), 2U);
    EXPECT_TRUE(domain::portfolio_detail::optional_string_vector(details, {"missing"}).empty());

    EXPECT_EQ(domain::to_string(domain::Currency::CHF), "CHF");
    EXPECT_EQ(domain::to_string(domain::Currency::EUR), "EUR");
    EXPECT_EQ(domain::to_string(domain::Currency::GBP), "GBP");
    EXPECT_EQ(domain::to_string(domain::Currency::JPY), "JPY");
    EXPECT_EQ(domain::to_string(static_cast<domain::Currency>(-1)), "UNKNOWN");

    EXPECT_EQ(domain::to_string(static_cast<domain::AssetClass>(-1)), "unknown");
    EXPECT_EQ(domain::to_string(domain::ProductType::CallableBond), "callable_bond");
    EXPECT_EQ(domain::to_string(domain::ProductType::CrossCurrencySwap), "cross_currency_swap");
    EXPECT_EQ(domain::to_string(domain::ProductType::EquityOption), "equity_option");
    EXPECT_EQ(domain::to_string(static_cast<domain::ProductType>(-1)), "unknown");
    EXPECT_EQ(domain::to_string(domain::SupportStatus::Failed), "failed");
    EXPECT_EQ(domain::to_string(static_cast<domain::SupportStatus>(-1)), "failed");

    EXPECT_EQ(domain::parse_product_type("callable_bond"), domain::ProductType::CallableBond);
    EXPECT_EQ(domain::asset_class_from_product_type(domain::ProductType::CallableBond), domain::AssetClass::Rates);
    EXPECT_EQ(domain::asset_class_from_product_type(domain::ProductType::CommodityFuture), domain::AssetClass::Commodity);
    EXPECT_EQ(domain::asset_class_from_product_type(domain::ProductType::CrossCurrencySwap), domain::AssetClass::FX);
    EXPECT_EQ(domain::asset_class_from_product_type(domain::ProductType::EquityOption), domain::AssetClass::Equity);
    EXPECT_EQ(domain::asset_class_from_product_type(static_cast<domain::ProductType>(-1)), domain::AssetClass::Unknown);

    std::shared_ptr<domain::Trade> trade = std::make_shared<domain::DepositTrade>();
    trade.reset();
}
