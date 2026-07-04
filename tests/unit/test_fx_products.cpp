// Verifies Phase 4 FX product valuation and FX spot/volatility risk coverage.

#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/risk_service.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/domain/factors.hpp>
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
    qrp::domain::Currency currency,
    const std::string& tenor,
    double value,
    const std::string& underlier = {}) {
    qrp::domain::MarketQuote quote;
    quote.id = id;
    quote.instrument_type = instrument_type;
    quote.quote_type = quote_type;
    quote.currency = currency;
    quote.tenor = tenor;
    quote.underlier = underlier;
    quote.value = value;
    quote.source_name = "UnitTest";
    quote.source_ts = "2026-03-24T12:00:00Z";
    return quote;
}

qrp::domain::CurveSpec make_ois_curve(
    qrp::domain::Currency currency,
    std::vector<std::string> quote_ids) {
    qrp::domain::CurveSpec curve;
    curve.id = {currency, "OIS"};
    curve.purpose = qrp::domain::CurvePurpose::OISDiscount;
    curve.quote_ids = std::move(quote_ids);
    curve.day_count = qrp::domain::DayCount::ACT360;
    curve.calendar = currency == qrp::domain::Currency::USD
        ? qrp::domain::BusinessCalendar::US
        : qrp::domain::BusinessCalendar::Target;
    curve.interpolation = qrp::domain::InterpolationType::LogLinear;
    return curve;
}

qrp::domain::MarketSnapshot make_fx_market() {
    qrp::domain::MarketSnapshot market;
    market.valuation_date = "2026-03-24";
    market.snapshot_id = "UNIT_FX_PHASE4";
    market.base_currency = qrp::domain::Currency::USD;
    market.default_stale_after_days = 10;

    market.quotes = {
        make_quote("EUR_ON", qrp::domain::QuoteInstrumentType::Deposit, qrp::domain::QuoteType::Deposit, qrp::domain::Currency::EUR, "1D", 0.0340),
        make_quote("EUR_OIS_1M", qrp::domain::QuoteInstrumentType::OIS, qrp::domain::QuoteType::OIS, qrp::domain::Currency::EUR, "1M", 0.0345),
        make_quote("EUR_OIS_6M", qrp::domain::QuoteInstrumentType::OIS, qrp::domain::QuoteType::OIS, qrp::domain::Currency::EUR, "6M", 0.0350),
        make_quote("EUR_OIS_1Y", qrp::domain::QuoteInstrumentType::OIS, qrp::domain::QuoteType::OIS, qrp::domain::Currency::EUR, "1Y", 0.0360),
        make_quote("USD_ON", qrp::domain::QuoteInstrumentType::Deposit, qrp::domain::QuoteType::Deposit, qrp::domain::Currency::USD, "1D", 0.0525),
        make_quote("USD_OIS_1M", qrp::domain::QuoteInstrumentType::OIS, qrp::domain::QuoteType::OIS, qrp::domain::Currency::USD, "1M", 0.0528),
        make_quote("USD_OIS_6M", qrp::domain::QuoteInstrumentType::OIS, qrp::domain::QuoteType::OIS, qrp::domain::Currency::USD, "6M", 0.0532),
        make_quote("USD_OIS_1Y", qrp::domain::QuoteInstrumentType::OIS, qrp::domain::QuoteType::OIS, qrp::domain::Currency::USD, "1Y", 0.0535),
        make_quote("EURUSD", qrp::domain::QuoteInstrumentType::FXSpot, qrp::domain::QuoteType::FXSpot, qrp::domain::Currency::USD, "SPOT", 1.1000, "EURUSD"),
        make_quote("EURUSD_FWDPTS_3M", qrp::domain::QuoteInstrumentType::FXForwardPoint, qrp::domain::QuoteType::FXForwardPoint, qrp::domain::Currency::USD, "3M", 0.0015, "EURUSD"),
        make_quote("EURUSD_FWDPTS_6M", qrp::domain::QuoteInstrumentType::FXForwardPoint, qrp::domain::QuoteType::FXForwardPoint, qrp::domain::Currency::USD, "6M", 0.0020, "EURUSD"),
        make_quote("EURUSD_FWDPTS_1Y", qrp::domain::QuoteInstrumentType::FXForwardPoint, qrp::domain::QuoteType::FXForwardPoint, qrp::domain::Currency::USD, "1Y", 0.0060, "EURUSD"),
        make_quote("EURUSD_VOL_6M_ATM", qrp::domain::QuoteInstrumentType::FXVol, qrp::domain::QuoteType::Volatility, qrp::domain::Currency::USD, "6M", 0.10, "EURUSD")
    };
    market.quotes.back().expiry = "6M";
    market.quotes.back().strike = "ATM";

    market.curves = {
        make_ois_curve(qrp::domain::Currency::EUR, {"EUR_ON", "EUR_OIS_1M", "EUR_OIS_6M", "EUR_OIS_1Y"}),
        make_ois_curve(qrp::domain::Currency::USD, {"USD_ON", "USD_OIS_1M", "USD_OIS_6M", "USD_OIS_1Y"})
    };
    return market;
}

template <typename TradeT>
std::shared_ptr<TradeT> make_fx_trade(const std::string& id, const std::string& direction) {
    auto trade = std::make_shared<TradeT>();
    trade->id = id;
    trade->asset_class = "fx";
    trade->asset_class_type = qrp::domain::AssetClass::FX;
    trade->currency = "USD";
    trade->direction = direction;
    trade->book = "BOOK:FX";
    trade->strategy = "PHASE4";
    return trade;
}

qrp::domain::Portfolio make_phase4_fx_portfolio() {
    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "phase4_fx";

    auto spot = make_fx_trade<qrp::domain::FxSpotTrade>("fx_spot_eurusd", "buy_base");
    spot->notional = 1'000'000.0;
    spot->base_currency = "EUR";
    spot->quote_currency = "USD";
    spot->reference_rate = 1.0900;
    portfolio.trades.push_back(spot);

    auto forward = make_fx_trade<qrp::domain::FxForwardTrade>("fx_forward_eurusd", "buy_base");
    forward->notional = 2'000'000.0;
    forward->start_date = "2026-03-24";
    forward->maturity_date = "2026-09-24";
    forward->base_currency = "EUR";
    forward->quote_currency = "USD";
    forward->forward_rate = 1.1050;
    forward->forward_points_quote_id = "EURUSD_FWDPTS_6M";
    portfolio.trades.push_back(forward);

    auto swap = make_fx_trade<qrp::domain::FxSwapTrade>("fx_swap_eurusd", "buy_base");
    swap->notional = 1'500'000.0;
    swap->start_date = "2026-06-24";
    swap->maturity_date = "2027-03-24";
    swap->base_currency = "EUR";
    swap->quote_currency = "USD";
    swap->near_rate = 1.1010;
    swap->far_rate = 1.1075;
    swap->near_forward_points_quote_id = "EURUSD_FWDPTS_3M";
    swap->far_forward_points_quote_id = "EURUSD_FWDPTS_1Y";
    portfolio.trades.push_back(swap);

    auto ndf = make_fx_trade<qrp::domain::NdfTrade>("ndf_eurusd", "buy_base");
    ndf->notional = 2'500'000.0;
    ndf->maturity_date = "2026-09-24";
    ndf->fixing_date = "2026-09-22";
    ndf->base_currency = "EUR";
    ndf->quote_currency = "USD";
    ndf->forward_rate = 1.1060;
    ndf->forward_points_quote_id = "EURUSD_FWDPTS_6M";
    portfolio.trades.push_back(ndf);

    auto option = make_fx_trade<qrp::domain::FxOptionTrade>("fx_option_eurusd", "long");
    option->notional = 1'000'000.0;
    option->expiry_date = "2026-09-24";
    option->settlement_date = "2026-09-28";
    option->base_currency = "EUR";
    option->quote_currency = "USD";
    option->strike_rate = 1.1000;
    option->option_type = "call";
    option->volatility_quote_id = "EURUSD_VOL_6M_ATM";
    portfolio.trades.push_back(option);

    return portfolio;
}

std::vector<qrp::domain::FactorDefinition> make_fx_factors() {
    qrp::domain::FactorDefinition spot;
    spot.factor_id = qrp::domain::make_fx_spot_factor_id("EURUSD");
    spot.factor_type = qrp::domain::FactorType::FXSpot;
    spot.shock_measure = qrp::domain::ShockMeasure::Relative;
    spot.currency = qrp::domain::Currency::USD;
    spot.description = "EURUSD spot";

    qrp::domain::FactorDefinition vol;
    vol.factor_id = qrp::domain::make_fx_vol_factor_id("EURUSD", "6M", "ATM");
    vol.factor_type = qrp::domain::FactorType::Volatility;
    vol.shock_measure = qrp::domain::ShockMeasure::VolPoints;
    vol.currency = qrp::domain::Currency::USD;
    vol.description = "EURUSD 6M ATM volatility";

    return {spot, vol};
}

std::vector<qrp::domain::FactorBinding> make_fx_bindings(const std::vector<qrp::domain::FactorDefinition>& factors) {
    return {
        {factors[0].factor_id, "EURUSD", qrp::domain::ShockMeasure::Relative, 1.0},
        {factors[1].factor_id, "EURUSD_VOL_6M_ATM", qrp::domain::ShockMeasure::VolPoints, 1.0}
    };
}

} // namespace

TEST(FxProductsTest, PricesPhase4FxProducts) {
    qrp::market::MarketSnapshot market(make_fx_market());
    qrp::analytics::PricingContext context(market.built_state());
    const auto portfolio = make_phase4_fx_portfolio();

    const auto results = qrp::analytics::ValuationService::price_portfolio(portfolio, context);

    ASSERT_EQ(results.size(), 5);

    std::map<std::string, qrp::analytics::ValuationResult> by_trade;
    for (const auto& result : results) {
        by_trade[result.trade_id] = result;
        EXPECT_EQ(result.asset_class, qrp::domain::AssetClass::FX) << result.trade_id;
        EXPECT_EQ(result.support_status, qrp::domain::SupportStatus::Supported) << result.trade_id;
        EXPECT_TRUE(std::isfinite(result.npv)) << result.trade_id;
    }

    EXPECT_EQ(by_trade.at("fx_spot_eurusd").product_type, qrp::domain::ProductType::FxSpot);
    EXPECT_EQ(by_trade.at("fx_forward_eurusd").product_type, qrp::domain::ProductType::FxForward);
    EXPECT_EQ(by_trade.at("fx_swap_eurusd").product_type, qrp::domain::ProductType::FxSwap);
    EXPECT_EQ(by_trade.at("ndf_eurusd").product_type, qrp::domain::ProductType::Ndf);
    EXPECT_EQ(by_trade.at("fx_option_eurusd").product_type, qrp::domain::ProductType::FxOption);

    EXPECT_NEAR(by_trade.at("fx_spot_eurusd").npv, 10'000.0, 1.0e-8);
    EXPECT_LT(by_trade.at("fx_forward_eurusd").npv, 0.0);
    EXPECT_LT(by_trade.at("ndf_eurusd").npv, 0.0);
    EXPECT_GT(by_trade.at("fx_option_eurusd").npv, 0.0);
}

TEST(FxProductsTest, ComputesFxSpotAndVolatilityRiskForOptions) {
    const auto market = make_fx_market();
    const auto portfolio = make_phase4_fx_portfolio();
    const auto factors = make_fx_factors();
    const auto bindings = make_fx_bindings(factors);

    const auto results = qrp::analytics::RiskService::compute_risk(portfolio, market, factors, bindings);

    std::map<std::string, qrp::analytics::RiskResult> by_trade;
    for (const auto& result : results) {
        by_trade[result.trade_id] = result;
    }

    const auto& option = by_trade.at("fx_option_eurusd");
    EXPECT_NE(option.fx_delta, 0.0);
    EXPECT_GT(option.fx_vega, 0.0);
    EXPECT_NE(option.bucketed_risk.at(factors[0].factor_id), 0.0);
    EXPECT_NEAR(option.bucketed_risk.at(factors[1].factor_id), option.fx_vega, 1.0e-8);
}
