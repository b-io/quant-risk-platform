// Verifies FX product valuation and FX spot/volatility risk coverage.

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

qrp::domain::FactorBinding make_factor_binding(const std::string& factor_id,
                                               const std::string& quote_id,
                                               qrp::domain::ShockMeasure shock_measure) {
    qrp::domain::FactorBinding binding;
    binding.factor_id = factor_id;
    binding.quote_id = quote_id;
    binding.shock_measure = shock_measure;
    binding.weight = 1.0;
    return binding;
}

qrp::domain::MarketQuote make_quote(const std::string& id,
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

qrp::domain::CurveSpec make_ois_curve(qrp::domain::Currency currency, std::vector<std::string> quote_ids) {
    qrp::domain::CurveSpec curve;
    curve.id = {currency, "OIS"};
    curve.purpose = qrp::domain::CurvePurpose::OISDiscount;
    curve.quote_ids = std::move(quote_ids);
    curve.day_count = qrp::domain::DayCount::ACT360;
    curve.calendar = currency == qrp::domain::Currency::USD ? qrp::domain::BusinessCalendar::US
                                                            : qrp::domain::BusinessCalendar::Target;
    curve.interpolation = qrp::domain::InterpolationType::LogLinear;
    return curve;
}

qrp::domain::MarketSnapshot make_fx_market() {
    qrp::domain::MarketSnapshot market;
    market.valuation_date = "2026-03-24";
    market.snapshot_id = "UNIT_FX_PRODUCTS";
    market.base_currency = qrp::domain::Currency::USD;
    market.default_stale_after_days = 10;

    market.quotes = {make_quote("EUR_ON",
                                qrp::domain::QuoteInstrumentType::Deposit,
                                qrp::domain::QuoteType::Deposit,
                                qrp::domain::Currency::EUR,
                                "1D",
                                0.0340),
                     make_quote("EUR_OIS_1M",
                                qrp::domain::QuoteInstrumentType::OIS,
                                qrp::domain::QuoteType::OIS,
                                qrp::domain::Currency::EUR,
                                "1M",
                                0.0345),
                     make_quote("EUR_OIS_6M",
                                qrp::domain::QuoteInstrumentType::OIS,
                                qrp::domain::QuoteType::OIS,
                                qrp::domain::Currency::EUR,
                                "6M",
                                0.0350),
                     make_quote("EUR_OIS_1Y",
                                qrp::domain::QuoteInstrumentType::OIS,
                                qrp::domain::QuoteType::OIS,
                                qrp::domain::Currency::EUR,
                                "1Y",
                                0.0360),
                     make_quote("USD_ON",
                                qrp::domain::QuoteInstrumentType::Deposit,
                                qrp::domain::QuoteType::Deposit,
                                qrp::domain::Currency::USD,
                                "1D",
                                0.0525),
                     make_quote("USD_OIS_1M",
                                qrp::domain::QuoteInstrumentType::OIS,
                                qrp::domain::QuoteType::OIS,
                                qrp::domain::Currency::USD,
                                "1M",
                                0.0528),
                     make_quote("USD_OIS_6M",
                                qrp::domain::QuoteInstrumentType::OIS,
                                qrp::domain::QuoteType::OIS,
                                qrp::domain::Currency::USD,
                                "6M",
                                0.0532),
                     make_quote("USD_OIS_1Y",
                                qrp::domain::QuoteInstrumentType::OIS,
                                qrp::domain::QuoteType::OIS,
                                qrp::domain::Currency::USD,
                                "1Y",
                                0.0535),
                     make_quote("EURUSD",
                                qrp::domain::QuoteInstrumentType::FXSpot,
                                qrp::domain::QuoteType::FXSpot,
                                qrp::domain::Currency::USD,
                                "SPOT",
                                1.1000,
                                "EURUSD"),
                     make_quote("EURUSD_FWDPTS_3M",
                                qrp::domain::QuoteInstrumentType::FXForwardPoint,
                                qrp::domain::QuoteType::FXForwardPoint,
                                qrp::domain::Currency::USD,
                                "3M",
                                0.0015,
                                "EURUSD"),
                     make_quote("EURUSD_FWDPTS_6M",
                                qrp::domain::QuoteInstrumentType::FXForwardPoint,
                                qrp::domain::QuoteType::FXForwardPoint,
                                qrp::domain::Currency::USD,
                                "6M",
                                0.0020,
                                "EURUSD"),
                     make_quote("EURUSD_FWDPTS_1Y",
                                qrp::domain::QuoteInstrumentType::FXForwardPoint,
                                qrp::domain::QuoteType::FXForwardPoint,
                                qrp::domain::Currency::USD,
                                "1Y",
                                0.0060,
                                "EURUSD"),
                     make_quote("EURUSD_VOL_6M_ATM",
                                qrp::domain::QuoteInstrumentType::FXVol,
                                qrp::domain::QuoteType::Volatility,
                                qrp::domain::Currency::USD,
                                "6M",
                                0.10,
                                "EURUSD")};
    market.quotes.back().expiry = "6M";
    market.quotes.back().strike = "ATM";

    market.curves = {make_ois_curve(qrp::domain::Currency::EUR, {"EUR_ON", "EUR_OIS_1M", "EUR_OIS_6M", "EUR_OIS_1Y"}),
                     make_ois_curve(qrp::domain::Currency::USD, {"USD_ON", "USD_OIS_1M", "USD_OIS_6M", "USD_OIS_1Y"})};
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
    trade->strategy = "FX_PRODUCTS";
    return trade;
}

qrp::domain::Portfolio make_fx_portfolio() {
    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "fx_products";

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
    return {make_factor_binding(factors[0].factor_id, "EURUSD", qrp::domain::ShockMeasure::Relative),
            make_factor_binding(factors[1].factor_id, "EURUSD_VOL_6M_ATM", qrp::domain::ShockMeasure::VolPoints)};
}

} // namespace

TEST(FxProductsTest, PricesFxProducts) {
    qrp::market::MarketSnapshot market(make_fx_market());
    qrp::analytics::PricingContext context(market.built_state());
    const auto portfolio = make_fx_portfolio();

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
    const auto portfolio = make_fx_portfolio();
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

TEST(FxProductsTest, PricesAlternativeDirectionsAndFallbackInputs) {
    auto market_dto = make_fx_market();
    market_dto.quotes.push_back(make_quote("EURUSD_FWD_6M",
                                           qrp::domain::QuoteInstrumentType::FXForward,
                                           qrp::domain::QuoteType::FXForward,
                                           qrp::domain::Currency::USD,
                                           "6M",
                                           1.1040,
                                           "EURUSD"));

    qrp::market::MarketSnapshot market(market_dto);
    qrp::analytics::PricingContext context(market.built_state());

    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "fx_edge_cases";

    auto sell_spot = make_fx_trade<qrp::domain::FxSpotTrade>("fx_spot_sell_eurusd", "sell_base");
    sell_spot->notional = 1'000'000.0;
    sell_spot->base_currency = "EUR";
    sell_spot->quote_currency = "USD";
    sell_spot->reference_rate = 1.0900;
    portfolio.trades.push_back(sell_spot);

    auto direct_forward = make_fx_trade<qrp::domain::FxForwardTrade>("fx_forward_direct_quote_eurusd", "sell");
    direct_forward->notional = 2'000'000.0;
    direct_forward->start_date = "2026-03-24";
    direct_forward->maturity_date = "2026-09-24";
    direct_forward->base_currency = "EUR";
    direct_forward->quote_currency = "USD";
    direct_forward->forward_rate = 1.1050;
    direct_forward->forward_quote_id = "EURUSD_FWD_6M";
    portfolio.trades.push_back(direct_forward);

    auto ndf_missing_forward = make_fx_trade<qrp::domain::NdfTrade>("ndf_missing_forward_quote_eurusd", "buy_base");
    ndf_missing_forward->notional = 500'000.0;
    ndf_missing_forward->maturity_date = "2026-09-24";
    ndf_missing_forward->base_currency = "EUR";
    ndf_missing_forward->quote_currency = "USD";
    ndf_missing_forward->forward_rate = 1.1060;
    ndf_missing_forward->forward_quote_id = "MISSING_EURUSD_FWD";
    portfolio.trades.push_back(ndf_missing_forward);

    auto fallback_option = make_fx_trade<qrp::domain::FxOptionTrade>("fx_option_zero_strike_eurusd", "short");
    fallback_option->notional = 100'000.0;
    fallback_option->expiry_date = "2026-09-24";
    fallback_option->base_currency = "EUR";
    fallback_option->quote_currency = "USD";
    fallback_option->strike_rate = 0.0;
    fallback_option->option_type = "put_base";
    portfolio.trades.push_back(fallback_option);

    auto missing_spot = make_fx_trade<qrp::domain::FxSpotTrade>("fx_spot_missing_quote", "buy_base");
    missing_spot->notional = 1'000'000.0;
    missing_spot->base_currency = "GBP";
    missing_spot->quote_currency = "USD";
    missing_spot->reference_rate = 1.25;
    portfolio.trades.push_back(missing_spot);

    const auto results = qrp::analytics::ValuationService::price_portfolio(portfolio, context);

    ASSERT_EQ(results.size(), 5U);
    std::map<std::string, qrp::analytics::ValuationResult> by_trade;
    for (const auto& result : results) {
        by_trade[result.trade_id] = result;
    }

    EXPECT_LT(by_trade.at("fx_spot_sell_eurusd").npv, 0.0);
    EXPECT_TRUE(std::isfinite(by_trade.at("fx_forward_direct_quote_eurusd").npv));
    EXPECT_TRUE(std::isfinite(by_trade.at("ndf_missing_forward_quote_eurusd").npv));
    EXPECT_TRUE(std::isfinite(by_trade.at("fx_option_zero_strike_eurusd").npv));
    EXPECT_EQ(by_trade.at("fx_spot_missing_quote").tags.at("status"), "failed");
}

TEST(FxProductsTest, FactoriesReturnNullWhenSpotQuoteIsMissing) {
    qrp::domain::MarketSnapshot market_dto;
    market_dto.valuation_date = "2026-03-24";
    qrp::market::MarketSnapshot market(market_dto);
    qrp::analytics::PricingContext context(market.built_state());

    qrp::domain::FxForwardTrade forward;
    forward.base_currency = "EUR";
    forward.quote_currency = "USD";
    forward.maturity_date = "2026-09-24";
    EXPECT_FALSE(qrp::instruments::FxInstrumentFactory::create_fx_forward(forward, context));

    qrp::domain::FxSwapTrade swap;
    swap.base_currency = "EUR";
    swap.quote_currency = "USD";
    swap.maturity_date = "2026-09-24";
    EXPECT_FALSE(qrp::instruments::FxInstrumentFactory::create_fx_swap(swap, context));

    qrp::domain::NdfTrade ndf;
    ndf.base_currency = "EUR";
    ndf.quote_currency = "USD";
    ndf.maturity_date = "2026-09-24";
    EXPECT_FALSE(qrp::instruments::FxInstrumentFactory::create_ndf(ndf, context));

    qrp::domain::FxOptionTrade option;
    option.base_currency = "EUR";
    option.quote_currency = "USD";
    option.expiry_date = "2026-09-24";
    EXPECT_FALSE(qrp::instruments::FxInstrumentFactory::create_fx_option(option, context));
}
