// Verifies Credit product valuation, CDS spread-curve inputs, CS01, and spread duration.

#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/risk_service.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/domain/factors.hpp>
#include <qrp/instruments/instrument_factory.hpp>
#include <qrp/market/market_snapshot.hpp>

#include <gtest/gtest.h>

#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace {

qrp::domain::MarketQuote make_quote(
    const std::string& id,
    qrp::domain::QuoteInstrumentType instrument_type,
    qrp::domain::QuoteType quote_type,
    const std::string& tenor,
    double value,
    const std::string& underlier = {}) {
    qrp::domain::MarketQuote quote;
    quote.id = id;
    quote.instrument_type = instrument_type;
    quote.quote_type = quote_type;
    quote.currency = qrp::domain::Currency::USD;
    quote.tenor = tenor;
    quote.underlier = underlier;
    quote.value = value;
    quote.source_name = "UnitTest";
    quote.source_ts = "2026-03-24T12:00:00Z";
    return quote;
}

qrp::domain::MarketSnapshot make_credit_market() {
    qrp::domain::MarketSnapshot market;
    market.valuation_date = "2026-03-24";
    market.snapshot_id = "UNIT_CREDIT_PRODUCTS";
    market.base_currency = qrp::domain::Currency::USD;
    market.default_stale_after_days = 10;

    market.quotes = {
        make_quote("ACME_BOND_SPREAD_5Y", qrp::domain::QuoteInstrumentType::BondSpread, qrp::domain::QuoteType::CreditSpread, "5Y", 0.0160, "ACME"),
        make_quote("ACME_CDS_1Y", qrp::domain::QuoteInstrumentType::CDS, qrp::domain::QuoteType::CreditSpread, "1Y", 0.0120, "ACME"),
        make_quote("ACME_CDS_3Y", qrp::domain::QuoteInstrumentType::CDS, qrp::domain::QuoteType::CreditSpread, "3Y", 0.0150, "ACME"),
        make_quote("ACME_CDS_5Y", qrp::domain::QuoteInstrumentType::CDS, qrp::domain::QuoteType::CreditSpread, "5Y", 0.0180, "ACME"),
        make_quote("ACME_CDS_OPTION_VOL_1Y_5Y", qrp::domain::QuoteInstrumentType::CreditSpread, qrp::domain::QuoteType::Volatility, "5Y", 0.35, "ACME"),
        make_quote("ACME_RECOVERY", qrp::domain::QuoteInstrumentType::RecoveryRate, qrp::domain::QuoteType::RecoveryRate, "RECOVERY", 0.40, "ACME"),
        make_quote("CDX_IG_5Y", qrp::domain::QuoteInstrumentType::CreditIndex, qrp::domain::QuoteType::CreditSpread, "5Y", 0.0100, "CDX_IG"),
        make_quote("CDX_IG_OPTION_VOL_1Y_5Y", qrp::domain::QuoteInstrumentType::CreditSpread, qrp::domain::QuoteType::Volatility, "5Y", 0.30, "CDX_IG"),
        make_quote("CDX_IG_RECOVERY", qrp::domain::QuoteInstrumentType::RecoveryRate, qrp::domain::QuoteType::RecoveryRate, "RECOVERY", 0.40, "CDX_IG"),
        make_quote("USD_ON", qrp::domain::QuoteInstrumentType::Deposit, qrp::domain::QuoteType::Deposit, "1D", 0.0525),
        make_quote("USD_OIS_1Y", qrp::domain::QuoteInstrumentType::OIS, qrp::domain::QuoteType::OIS, "1Y", 0.0535),
        make_quote("USD_OIS_2Y", qrp::domain::QuoteInstrumentType::OIS, qrp::domain::QuoteType::OIS, "2Y", 0.0537),
        make_quote("USD_OIS_5Y", qrp::domain::QuoteInstrumentType::OIS, qrp::domain::QuoteType::OIS, "5Y", 0.0540),
        make_quote("USD_OIS_10Y", qrp::domain::QuoteInstrumentType::OIS, qrp::domain::QuoteType::OIS, "10Y", 0.0545)
    };
    market.quotes[4].expiry = "1Y";
    market.quotes[4].strike = "5Y";
    market.quotes[7].expiry = "1Y";
    market.quotes[7].strike = "5Y";

    qrp::domain::CurveSpec ois;
    ois.id = {qrp::domain::Currency::USD, "OIS"};
    ois.purpose = qrp::domain::CurvePurpose::OISDiscount;
    ois.quote_ids = {"USD_ON", "USD_OIS_1Y", "USD_OIS_2Y", "USD_OIS_5Y", "USD_OIS_10Y"};
    ois.day_count = qrp::domain::DayCount::ACT360;
    ois.calendar = qrp::domain::BusinessCalendar::US;
    ois.interpolation = qrp::domain::InterpolationType::LogLinear;

    market.curves = {ois};
    return market;
}

template <typename TradeT>
std::shared_ptr<TradeT> make_credit_trade(const std::string& id, const std::string& direction) {
    auto trade = std::make_shared<TradeT>();
    trade->id = id;
    trade->asset_class = "credit";
    trade->asset_class_type = qrp::domain::AssetClass::Credit;
    trade->currency = "USD";
    trade->direction = direction;
    trade->book = "BOOK:CREDIT";
    trade->strategy = "CREDIT_PRODUCTS";
    return trade;
}

qrp::domain::Portfolio make_credit_portfolio() {
    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "credit_products";

    auto credit_bond = make_credit_trade<qrp::domain::CreditBondTrade>("credit_bond_acme", "long");
    credit_bond->notional = 2'000'000.0;
    credit_bond->start_date = "2026-03-24";
    credit_bond->maturity_date = "2031-03-24";
    credit_bond->coupon_rate = 0.0550;
    credit_bond->frequency = "Annual";
    credit_bond->issuer = "ACME";
    credit_bond->spread_quote_id = "ACME_BOND_SPREAD_5Y";
    portfolio.trades.push_back(credit_bond);

    auto cds = make_credit_trade<qrp::domain::CdsTrade>("cds_acme", "buy_protection");
    cds->notional = 5'000'000.0;
    cds->start_date = "2026-03-24";
    cds->maturity_date = "2031-03-24";
    cds->coupon_rate = 0.0100;
    cds->frequency = "Quarterly";
    cds->issuer = "ACME";
    cds->recovery_quote_id = "ACME_RECOVERY";
    cds->spread_quote_id = "ACME_CDS_5Y";
    portfolio.trades.push_back(cds);

    auto cds_index = make_credit_trade<qrp::domain::CdsIndexTrade>("cds_index_cdx_ig", "buy_protection");
    cds_index->notional = 10'000'000.0;
    cds_index->start_date = "2026-03-24";
    cds_index->maturity_date = "2031-03-24";
    cds_index->coupon_rate = 0.0090;
    cds_index->frequency = "Quarterly";
    cds_index->index_factor = 0.98;
    cds_index->index_name = "CDX_IG";
    cds_index->recovery_quote_id = "CDX_IG_RECOVERY";
    cds_index->spread_quote_id = "CDX_IG_5Y";
    portfolio.trades.push_back(cds_index);

    auto cds_option = make_credit_trade<qrp::domain::CdsOptionTrade>("cds_option_acme", "long");
    cds_option->notional = 5'000'000.0;
    cds_option->expiry_date = "2027-03-24";
    cds_option->maturity_date = "2031-03-24";
    cds_option->strike_spread = 0.0150;
    cds_option->frequency = "Quarterly";
    cds_option->issuer = "ACME";
    cds_option->option_type = "call";
    cds_option->recovery_quote_id = "ACME_RECOVERY";
    cds_option->spread_quote_id = "ACME_CDS_5Y";
    cds_option->volatility_quote_id = "ACME_CDS_OPTION_VOL_1Y_5Y";
    portfolio.trades.push_back(cds_option);

    auto credit_index_option = make_credit_trade<qrp::domain::CreditIndexOptionTrade>("credit_index_option_cdx_ig", "long");
    credit_index_option->notional = 10'000'000.0;
    credit_index_option->expiry_date = "2027-03-24";
    credit_index_option->maturity_date = "2031-03-24";
    credit_index_option->index_factor = 0.98;
    credit_index_option->index_name = "CDX_IG";
    credit_index_option->strike_spread = 0.0090;
    credit_index_option->frequency = "Quarterly";
    credit_index_option->option_type = "call";
    credit_index_option->recovery_quote_id = "CDX_IG_RECOVERY";
    credit_index_option->spread_quote_id = "CDX_IG_5Y";
    credit_index_option->volatility_quote_id = "CDX_IG_OPTION_VOL_1Y_5Y";
    portfolio.trades.push_back(credit_index_option);

    return portfolio;
}

std::vector<qrp::domain::FactorDefinition> make_credit_factors() {
    qrp::domain::FactorDefinition acme;
    acme.factor_id = qrp::domain::make_credit_spread_factor_id("ACME", "5Y");
    acme.factor_type = qrp::domain::FactorType::CreditSpread;
    acme.shock_measure = qrp::domain::ShockMeasure::Absolute;
    acme.currency = qrp::domain::Currency::USD;
    acme.tenor = "5Y";
    acme.description = "ACME 5Y credit spread curve";

    qrp::domain::FactorDefinition cdx;
    cdx.factor_id = qrp::domain::make_credit_spread_factor_id("CDX_IG", "5Y");
    cdx.factor_type = qrp::domain::FactorType::CreditSpread;
    cdx.shock_measure = qrp::domain::ShockMeasure::Absolute;
    cdx.currency = qrp::domain::Currency::USD;
    cdx.tenor = "5Y";
    cdx.description = "CDX IG 5Y index spread";

    return {acme, cdx};
}

std::vector<qrp::domain::FactorBinding> make_credit_bindings(
    const std::vector<qrp::domain::FactorDefinition>& factors) {
    return {
        {factors[0].factor_id, "ACME_BOND_SPREAD_5Y", qrp::domain::ShockMeasure::Absolute, 1.0},
        {factors[0].factor_id, "ACME_CDS_1Y", qrp::domain::ShockMeasure::Absolute, 1.0},
        {factors[0].factor_id, "ACME_CDS_3Y", qrp::domain::ShockMeasure::Absolute, 1.0},
        {factors[0].factor_id, "ACME_CDS_5Y", qrp::domain::ShockMeasure::Absolute, 1.0},
        {factors[1].factor_id, "CDX_IG_5Y", qrp::domain::ShockMeasure::Absolute, 1.0}
    };
}

} // namespace

TEST(CreditProductsTest, PricesCreditProducts) {
    qrp::market::MarketSnapshot market(make_credit_market());
    qrp::analytics::PricingContext context(market.built_state());
    const auto portfolio = make_credit_portfolio();

    const auto results = qrp::analytics::ValuationService::price_portfolio(portfolio, context);

    ASSERT_EQ(results.size(), 5);

    std::map<std::string, qrp::analytics::ValuationResult> by_trade;
    for (const auto& result : results) {
        by_trade[result.trade_id] = result;
        EXPECT_EQ(result.asset_class, qrp::domain::AssetClass::Credit) << result.trade_id;
        EXPECT_EQ(result.support_status, qrp::domain::SupportStatus::Supported) << result.trade_id;
        EXPECT_TRUE(std::isfinite(result.npv)) << result.trade_id;
    }

    EXPECT_EQ(by_trade.at("credit_bond_acme").product_type, qrp::domain::ProductType::CreditBond);
    EXPECT_EQ(by_trade.at("cds_acme").product_type, qrp::domain::ProductType::Cds);
    EXPECT_EQ(by_trade.at("cds_index_cdx_ig").product_type, qrp::domain::ProductType::CdsIndex);
    EXPECT_EQ(by_trade.at("cds_option_acme").product_type, qrp::domain::ProductType::CdsOption);
    EXPECT_EQ(by_trade.at("credit_index_option_cdx_ig").product_type, qrp::domain::ProductType::CreditIndexOption);

    EXPECT_GT(by_trade.at("credit_bond_acme").npv, 0.0);
    EXPECT_GT(by_trade.at("cds_acme").npv, 0.0);
    EXPECT_GT(by_trade.at("cds_index_cdx_ig").npv, 0.0);
    EXPECT_GT(by_trade.at("cds_option_acme").npv, 0.0);
    EXPECT_GT(by_trade.at("credit_index_option_cdx_ig").npv, 0.0);
}

TEST(CreditProductsTest, ComputesCs01AndSpreadDuration) {
    const auto market = make_credit_market();
    const auto portfolio = make_credit_portfolio();
    const auto factors = make_credit_factors();
    const auto bindings = make_credit_bindings(factors);

    const auto results = qrp::analytics::RiskService::compute_risk(portfolio, market, factors, bindings);

    std::map<std::string, qrp::analytics::RiskResult> by_trade;
    for (const auto& result : results) {
        by_trade[result.trade_id] = result;
    }

    const auto& credit_bond = by_trade.at("credit_bond_acme");
    EXPECT_LT(credit_bond.cs01, 0.0);
    EXPECT_GT(credit_bond.spread_duration, 0.0);
    EXPECT_LT(credit_bond.bucketed_risk.at("5Y"), 0.0);

    EXPECT_GT(by_trade.at("cds_acme").cs01, 0.0);
    EXPECT_GT(by_trade.at("cds_index_cdx_ig").cs01, 0.0);
    EXPECT_GT(by_trade.at("cds_option_acme").cs01, 0.0);
    EXPECT_GT(by_trade.at("credit_index_option_cdx_ig").cs01, 0.0);
}

TEST(CreditProductsTest, PricesFallbackCreditInputsAndAlternativeDirections) {
    auto market_dto = make_credit_market();
    market_dto.quotes.push_back(make_quote(
        "ACME_ZERO_VOL",
        qrp::domain::QuoteInstrumentType::CreditSpread,
        qrp::domain::QuoteType::Volatility,
        "5Y",
        0.0,
        "ACME"));
    market_dto.quotes.back().expiry = "6M";
    market_dto.quotes.back().strike = "5Y";

    qrp::market::MarketSnapshot market(market_dto);
    qrp::analytics::PricingContext context(market.built_state());

    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "credit_edge_cases";

    auto short_bond = make_credit_trade<qrp::domain::CreditBondTrade>("credit_bond_short_fallback", "sell");
    short_bond->notional = 1'000'000.0;
    short_bond->start_date = "2026-03-24";
    short_bond->maturity_date = "2030-03-24";
    short_bond->coupon_rate = 0.0520;
    short_bond->credit_spread = 0.0140;
    short_bond->frequency = "Semiannual";
    short_bond->issuer = "NO_MARKET_QUOTES";
    portfolio.trades.push_back(short_bond);

    auto sell_protection = make_credit_trade<qrp::domain::CdsTrade>("cds_sell_fallback", "sell_protection");
    sell_protection->notional = 2'000'000.0;
    sell_protection->start_date = "2026-03-24";
    sell_protection->maturity_date = "2031-03-24";
    sell_protection->coupon_rate = 0.0120;
    sell_protection->recovery_rate = 1.20;
    sell_protection->issuer = "NO_MARKET_QUOTES";
    portfolio.trades.push_back(sell_protection);

    auto zero_vol_option = make_credit_trade<qrp::domain::CdsOptionTrade>("cds_option_zero_vol", "written");
    zero_vol_option->notional = 2'000'000.0;
    zero_vol_option->expiry_date = "2026-09-24";
    zero_vol_option->maturity_date = "2031-03-24";
    zero_vol_option->strike_spread = 0.0200;
    zero_vol_option->issuer = "ACME";
    zero_vol_option->option_type = "receiver";
    zero_vol_option->recovery_quote_id = "";
    zero_vol_option->spread_quote_id = "ACME_CDS_5Y";
    zero_vol_option->volatility_quote_id = "ACME_ZERO_VOL";
    portfolio.trades.push_back(zero_vol_option);

    auto default_vol_option = make_credit_trade<qrp::domain::CdsOptionTrade>("cds_option_default_vol", "long");
    default_vol_option->notional = 2'000'000.0;
    default_vol_option->expiry_date = "2026-09-24";
    default_vol_option->maturity_date = "2031-03-24";
    default_vol_option->strike_spread = 0.0180;
    default_vol_option->issuer = "ACME";
    default_vol_option->option_type = "call";
    default_vol_option->spread_quote_id = "ACME_CDS_5Y";
    portfolio.trades.push_back(default_vol_option);

    auto default_vol_index_option = make_credit_trade<qrp::domain::CreditIndexOptionTrade>("credit_index_option_default_vol", "long");
    default_vol_index_option->notional = 2'000'000.0;
    default_vol_index_option->expiry_date = "2026-09-24";
    default_vol_index_option->maturity_date = "2031-03-24";
    default_vol_index_option->index_name = "CDX_IG";
    default_vol_index_option->index_factor = 1.0;
    default_vol_index_option->strike_spread = 0.0100;
    default_vol_index_option->option_type = "call";
    default_vol_index_option->spread_quote_id = "CDX_IG_5Y";
    portfolio.trades.push_back(default_vol_index_option);

    auto missing_discount_curve = make_credit_trade<qrp::domain::CreditBondTrade>("credit_bond_missing_curve", "long");
    missing_discount_curve->notional = 1'000'000.0;
    missing_discount_curve->currency = "EUR";
    missing_discount_curve->start_date = "2026-03-24";
    missing_discount_curve->maturity_date = "2030-03-24";
    missing_discount_curve->coupon_rate = 0.04;
    missing_discount_curve->issuer = "ACME";
    portfolio.trades.push_back(missing_discount_curve);

    const auto results = qrp::analytics::ValuationService::price_portfolio(portfolio, context);

    ASSERT_EQ(results.size(), 6U);
    std::map<std::string, qrp::analytics::ValuationResult> by_trade;
    for (const auto& result : results) {
        by_trade[result.trade_id] = result;
    }

    EXPECT_LT(by_trade.at("credit_bond_short_fallback").npv, 0.0);
    EXPECT_LT(by_trade.at("cds_sell_fallback").npv, 0.0);
    EXPECT_TRUE(std::isfinite(by_trade.at("cds_option_zero_vol").npv));
    EXPECT_TRUE(std::isfinite(by_trade.at("cds_option_default_vol").npv));
    EXPECT_TRUE(std::isfinite(by_trade.at("credit_index_option_default_vol").npv));
    EXPECT_EQ(by_trade.at("credit_bond_missing_curve").tags.at("status"), "failed");
}

TEST(CreditProductsTest, FactoriesReturnNullWhenCreditDiscountCurveIsMissing) {
    qrp::domain::MarketSnapshot market_dto;
    market_dto.valuation_date = "2026-03-24";
    qrp::market::MarketSnapshot market(market_dto);
    qrp::analytics::PricingContext context(market.built_state());

    qrp::domain::CreditIndexOptionTrade credit_index_option;
    credit_index_option.currency = "USD";
    credit_index_option.expiry_date = "2026-09-24";
    credit_index_option.maturity_date = "2031-03-24";
    credit_index_option.index_name = "CDX_IG";

    EXPECT_FALSE(qrp::instruments::CreditInstrumentFactory::create_credit_index_option(credit_index_option, context));
}
