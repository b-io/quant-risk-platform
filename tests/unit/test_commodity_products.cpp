// Verifies commodity product valuation coverage.

#include <qrp/analytics/dynamic_programming/gas_storage_policy.hpp>
#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/domain/portfolio.hpp>
#include <qrp/instruments/instrument_factory.hpp>
#include <qrp/market/market_snapshot.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace {

qrp::domain::MarketQuote make_quote(const std::string& id,
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

qrp::domain::CurveSpec make_usd_ois_curve() {
    qrp::domain::CurveSpec curve;
    curve.id = {qrp::domain::Currency::USD, "OIS"};
    curve.purpose = qrp::domain::CurvePurpose::OISDiscount;
    curve.quote_ids = {"USD_ON", "USD_OIS_1Y"};
    curve.day_count = qrp::domain::DayCount::ACT360;
    curve.calendar = qrp::domain::BusinessCalendar::US;
    curve.interpolation = qrp::domain::InterpolationType::LogLinear;
    return curve;
}

qrp::domain::MarketSnapshot make_commodity_market() {
    qrp::domain::MarketSnapshot market;
    market.valuation_date = "2026-03-24";
    market.snapshot_id = "UNIT_COMMODITY_PRODUCTS";
    market.base_currency = qrp::domain::Currency::USD;
    market.default_stale_after_days = 10;

    market.quotes = {
        make_quote("USD_ON", qrp::domain::QuoteInstrumentType::Deposit, qrp::domain::QuoteType::Deposit, "1D", 0.0525),
        make_quote("USD_OIS_1Y", qrp::domain::QuoteInstrumentType::OIS, qrp::domain::QuoteType::OIS, "1Y", 0.0535),
        make_quote("WTI_SPOT",
                   qrp::domain::QuoteInstrumentType::CommoditySpot,
                   qrp::domain::QuoteType::Price,
                   "SPOT",
                   77.50,
                   "WTI"),
        make_quote("WTI_FUT_6M",
                   qrp::domain::QuoteInstrumentType::CommodityFuture,
                   qrp::domain::QuoteType::CommodityForward,
                   "6M",
                   78.25,
                   "WTI"),
        make_quote("WTI_FUT_9M",
                   qrp::domain::QuoteInstrumentType::CommodityFuture,
                   qrp::domain::QuoteType::CommodityForward,
                   "9M",
                   79.10,
                   "WTI"),
        make_quote("WTI_VOL_6M_ATM",
                   qrp::domain::QuoteInstrumentType::CommodityVol,
                   qrp::domain::QuoteType::Volatility,
                   "6M",
                   0.30,
                   "WTI"),
        make_quote("TTF_FWD_Q1",
                   qrp::domain::QuoteInstrumentType::CommodityForward,
                   qrp::domain::QuoteType::CommodityForward,
                   "Q1",
                   32.00,
                   "TTF"),
        make_quote("TTF_FWD_Q2",
                   qrp::domain::QuoteInstrumentType::CommodityForward,
                   qrp::domain::QuoteType::CommodityForward,
                   "Q2",
                   34.00,
                   "TTF")};
    market.quotes[5].expiry = "6M";
    market.quotes[5].strike = "ATM";
    market.curves = {make_usd_ois_curve()};
    return market;
}

template <typename TradeT>
std::shared_ptr<TradeT> make_commodity_trade(const std::string& id, const std::string& direction) {
    auto trade = std::make_shared<TradeT>();
    trade->id = id;
    trade->asset_class = "commodity";
    trade->asset_class_type = qrp::domain::AssetClass::Commodity;
    trade->currency = "USD";
    trade->direction = direction;
    trade->book = "BOOK:COMMODITY";
    trade->strategy = "COMMODITY_PRODUCTS";
    return trade;
}

qrp::domain::Portfolio make_commodity_portfolio() {
    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "commodity_products";

    auto spot = make_commodity_trade<qrp::domain::CommoditySpotTrade>("commodity_spot_wti", "long");
    spot->quantity = 1000.0;
    spot->reference_price = 77.00;
    spot->spot_quote_id = "WTI_SPOT";
    spot->underlier = "WTI";
    portfolio.trades.push_back(spot);

    auto forward = make_commodity_trade<qrp::domain::CommodityForwardTrade>("commodity_forward_wti", "long");
    forward->quantity = 1000.0;
    forward->contract_price = 78.00;
    forward->maturity_date = "2026-09-24";
    forward->forward_quote_id = "WTI_FUT_6M";
    forward->underlier = "WTI";
    portfolio.trades.push_back(forward);

    auto future = make_commodity_trade<qrp::domain::CommodityFutureTrade>("commodity_future_wti", "long");
    future->quantity = 3.0;
    future->contract_size = 1000.0;
    future->reference_price = 78.00;
    future->maturity_date = "2026-09-24";
    future->future_quote_id = "WTI_FUT_6M";
    future->underlier = "WTI";
    portfolio.trades.push_back(future);

    auto strip = make_commodity_trade<qrp::domain::CommodityFutureStripTrade>("commodity_strip_wti", "long");
    strip->quantity = 1.0;
    strip->contract_size = 1000.0;
    strip->reference_price = 78.50;
    strip->maturity_date = "2026-12-24";
    strip->future_quote_ids = {"WTI_FUT_6M", "WTI_FUT_9M"};
    strip->weights = {0.5, 0.5};
    strip->underlier = "WTI";
    portfolio.trades.push_back(strip);

    auto option = make_commodity_trade<qrp::domain::CommodityFutureOptionTrade>("commodity_future_option_wti", "long");
    option->quantity = 1.0;
    option->contract_size = 1000.0;
    option->strike_price = 78.00;
    option->expiry_date = "2026-09-24";
    option->maturity_date = "2026-09-24";
    option->future_quote_id = "WTI_FUT_6M";
    option->volatility_quote_id = "WTI_VOL_6M_ATM";
    option->underlier = "WTI";
    portfolio.trades.push_back(option);

    auto spread_option =
        make_commodity_trade<qrp::domain::CommodityCalendarSpreadOptionTrade>("commodity_calendar_spread_wti", "long");
    spread_option->quantity = 1.0;
    spread_option->contract_size = 1000.0;
    spread_option->strike_spread = 0.50;
    spread_option->expiry_date = "2026-09-24";
    spread_option->near_future_quote_id = "WTI_FUT_6M";
    spread_option->far_future_quote_id = "WTI_FUT_9M";
    spread_option->volatility_quote_id = "WTI_VOL_6M_ATM";
    spread_option->underlier = "WTI";
    portfolio.trades.push_back(spread_option);

    auto swing = make_commodity_trade<qrp::domain::CommoditySwingTrade>("commodity_swing_ttf", "long");
    swing->min_quantity = 100.0;
    swing->max_quantity = 200.0;
    swing->max_exercise_quantity = 125.0;
    swing->strike_price = 31.00;
    swing->maturity_date = "2026-12-24";
    swing->forward_quote_ids = {"TTF_FWD_Q1", "TTF_FWD_Q2"};
    swing->exercise_dates = {"2026-06-24", "2026-09-24"};
    swing->volatility = 0.40;
    swing->underlier = "TTF";
    portfolio.trades.push_back(swing);

    auto storage = make_commodity_trade<qrp::domain::GasStorageTrade>("gas_storage_ttf", "long");
    storage->min_inventory = 0.0;
    storage->max_inventory = 200.0;
    storage->initial_inventory = 50.0;
    storage->terminal_inventory_target = 50.0;
    storage->terminal_inventory_penalty = 100.0;
    storage->max_injection_quantity = 100.0;
    storage->max_withdrawal_quantity = 100.0;
    storage->injection_cost = 0.20;
    storage->withdrawal_cost = 0.20;
    storage->maturity_date = "2026-12-24";
    storage->forward_quote_ids = {"TTF_FWD_Q1", "TTF_FWD_Q2"};
    storage->exercise_dates = {"2026-06-24", "2026-09-24"};
    storage->underlier = "TTF";
    portfolio.trades.push_back(storage);

    return portfolio;
}

} // namespace

TEST(CommodityProductsTest, PricesCommodityProducts) {
    qrp::market::MarketSnapshot market(make_commodity_market());
    qrp::analytics::PricingContext context(market.built_state());
    const auto portfolio = make_commodity_portfolio();

    const auto results = qrp::analytics::ValuationService::price_portfolio(portfolio, context);

    ASSERT_EQ(results.size(), 8U);

    std::map<std::string, qrp::analytics::ValuationResult> by_trade;
    for (const auto& result : results) {
        by_trade[result.trade_id] = result;
        EXPECT_EQ(result.asset_class, qrp::domain::AssetClass::Commodity) << result.trade_id;
        EXPECT_TRUE(std::isfinite(result.npv)) << result.trade_id;
    }

    EXPECT_EQ(by_trade.at("commodity_spot_wti").product_type, qrp::domain::ProductType::CommoditySpot);
    EXPECT_EQ(by_trade.at("commodity_forward_wti").product_type, qrp::domain::ProductType::CommodityForward);
    EXPECT_EQ(by_trade.at("commodity_future_wti").product_type, qrp::domain::ProductType::CommodityFuture);
    EXPECT_EQ(by_trade.at("commodity_strip_wti").product_type, qrp::domain::ProductType::CommodityFutureStrip);
    EXPECT_EQ(by_trade.at("commodity_future_option_wti").product_type, qrp::domain::ProductType::CommodityFutureOption);
    EXPECT_EQ(by_trade.at("commodity_calendar_spread_wti").product_type,
              qrp::domain::ProductType::CommodityCalendarSpreadOption);
    EXPECT_EQ(by_trade.at("commodity_swing_ttf").product_type, qrp::domain::ProductType::CommoditySwing);
    EXPECT_EQ(by_trade.at("gas_storage_ttf").product_type, qrp::domain::ProductType::GasStorage);

    EXPECT_EQ(by_trade.at("commodity_swing_ttf").support_status, qrp::domain::SupportStatus::Supported);
    EXPECT_EQ(by_trade.at("gas_storage_ttf").support_status, qrp::domain::SupportStatus::Supported);
    EXPECT_NEAR(by_trade.at("commodity_spot_wti").npv, 500.0, 1.0e-10);
    EXPECT_NEAR(by_trade.at("commodity_future_wti").npv, 750.0, 1.0e-10);
    EXPECT_GT(by_trade.at("commodity_forward_wti").npv, 0.0);
    EXPECT_GT(by_trade.at("commodity_future_option_wti").npv, 0.0);
    EXPECT_GT(by_trade.at("commodity_calendar_spread_wti").npv, 0.0);
    EXPECT_GT(by_trade.at("commodity_swing_ttf").npv, 0.0);
    EXPECT_GT(by_trade.at("gas_storage_ttf").npv, 0.0);
}

TEST(CommodityProductsTest, GasStoragePolicyAppliesInventoryActionsAndTerminalPenalty) {
    qrp::analytics::dynamic_programming::GasStoragePolicyParams params;
    params.min_inventory = 0.0;
    params.max_inventory = 100.0;
    params.max_injection_quantity = 40.0;
    params.max_withdrawal_quantity = 30.0;
    params.injection_cost = 0.25;
    params.withdrawal_cost = 0.10;
    params.terminal_inventory_target = 50.0;
    params.terminal_inventory_penalty = 2.0;
    params.discount_factors = {0.99, 0.98};

    qrp::analytics::dynamic_programming::GasStorageDecisionProblem problem(params);
    const qrp::analytics::dynamic_programming::State state{{32.0}, {40.0}};
    const auto actions = problem.feasibleActions(state, 0U);
    ASSERT_GE(actions.size(), 3U);

    const auto inject =
        std::find_if(actions.begin(), actions.end(), [](const auto& action) { return action.name == "Inject"; });
    const auto withdraw =
        std::find_if(actions.begin(), actions.end(), [](const auto& action) { return action.name == "Withdraw"; });
    ASSERT_NE(inject, actions.end());
    ASSERT_NE(withdraw, actions.end());
    EXPECT_LT(problem.immediateCashflow(state, *inject, 0U), 0.0);
    EXPECT_GT(problem.immediateCashflow(state, *withdraw, 0U), 0.0);

    const auto next_state = problem.nextState(state, *inject, {34.0}, 0U);
    ASSERT_EQ(next_state.operational_variables.size(), 1U);
    EXPECT_GT(next_state.operational_variables.front(), state.operational_variables.front());
    EXPECT_LT(problem.terminalValue({{}, {30.0}}), problem.terminalValue({{}, {50.0}}));
    EXPECT_EQ(problem.regressionFeatureNames(0U).size(), problem.regressionFeatures(state, 0U).size());

    qrp::analytics::dynamic_programming::GasStoragePolicyParams normalized_params;
    normalized_params.min_inventory = -10.0;
    normalized_params.max_inventory = -5.0;
    normalized_params.max_injection_quantity = -1.0;
    normalized_params.max_withdrawal_quantity = -1.0;
    normalized_params.terminal_inventory_target = 25.0;
    normalized_params.terminal_inventory_penalty = -2.0;
    qrp::analytics::dynamic_programming::GasStorageDecisionProblem normalized_problem(normalized_params);
    const qrp::analytics::dynamic_programming::Action empty_action{0, "Idle", {}};
    EXPECT_DOUBLE_EQ(normalized_problem.immediateCashflow({{}, {}}, empty_action, 5U), 0.0);
    EXPECT_DOUBLE_EQ(normalized_problem.terminalValue({{}, {10.0}}), 0.0);

    const qrp::analytics::dynamic_programming::Action oversize_inject{1, "Inject", {{"inventory_delta", 100.0}}};
    const auto clamped_state = normalized_problem.nextState({{}, {}}, oversize_inject, {25.0}, 0U);
    ASSERT_EQ(clamped_state.operational_variables.size(), 1U);
    EXPECT_DOUBLE_EQ(clamped_state.operational_variables.front(), 0.0);
}

TEST(CommodityProductsTest, FactoriesReturnNullWhenCommodityQuotesAreMissing) {
    qrp::domain::MarketSnapshot market_dto;
    market_dto.valuation_date = "2026-03-24";
    qrp::market::MarketSnapshot market(market_dto);
    qrp::analytics::PricingContext context(market.built_state());

    qrp::domain::CommoditySpotTrade spot;
    spot.underlier = "WTI";
    EXPECT_FALSE(qrp::instruments::CommodityInstrumentFactory::create_commodity_spot(spot, context));

    qrp::domain::CommodityForwardTrade forward;
    forward.underlier = "WTI";
    EXPECT_FALSE(qrp::instruments::CommodityInstrumentFactory::create_commodity_forward(forward, context));

    forward.maturity_date = "2026-09-24";
    forward.forward_quote_id = "MISSING";
    forward.spot_quote_id = "MISSING_SPOT";
    EXPECT_FALSE(qrp::instruments::CommodityInstrumentFactory::create_commodity_forward(forward, context));

    qrp::domain::CommodityFutureTrade future_without_maturity;
    future_without_maturity.future_quote_id = "MISSING";
    EXPECT_FALSE(
        qrp::instruments::CommodityInstrumentFactory::create_commodity_future(future_without_maturity, context));

    qrp::domain::CommodityFutureTrade future;
    future.maturity_date = "2026-09-24";
    future.future_quote_id = "MISSING";
    EXPECT_FALSE(qrp::instruments::CommodityInstrumentFactory::create_commodity_future(future, context));

    qrp::domain::CommodityFutureStripTrade strip;
    strip.maturity_date = "2026-12-24";
    EXPECT_FALSE(qrp::instruments::CommodityInstrumentFactory::create_commodity_future_strip(strip, context));

    strip.future_quote_ids = {"MISSING"};
    EXPECT_FALSE(qrp::instruments::CommodityInstrumentFactory::create_commodity_future_strip(strip, context));

    qrp::domain::CommodityFutureOptionTrade option_without_expiry;
    option_without_expiry.future_quote_id = "MISSING";
    EXPECT_FALSE(
        qrp::instruments::CommodityInstrumentFactory::create_commodity_future_option(option_without_expiry, context));

    qrp::domain::CommodityFutureOptionTrade option;
    option.expiry_date = "2026-09-24";
    option.future_quote_id = "MISSING";
    EXPECT_FALSE(qrp::instruments::CommodityInstrumentFactory::create_commodity_future_option(option, context));

    qrp::domain::CommodityCalendarSpreadOptionTrade spread_without_expiry;
    spread_without_expiry.near_future_quote_id = "MISSING_NEAR";
    spread_without_expiry.far_future_quote_id = "MISSING_FAR";
    EXPECT_FALSE(
        qrp::instruments::CommodityInstrumentFactory::create_commodity_calendar_spread_option(spread_without_expiry,
                                                                                              context));

    qrp::domain::CommodityCalendarSpreadOptionTrade spread_option;
    spread_option.expiry_date = "2026-09-24";
    spread_option.near_future_quote_id = "MISSING_NEAR";
    spread_option.far_future_quote_id = "MISSING_FAR";
    EXPECT_FALSE(
        qrp::instruments::CommodityInstrumentFactory::create_commodity_calendar_spread_option(spread_option, context));

    qrp::domain::CommoditySwingTrade swing_without_maturity;
    swing_without_maturity.underlier = "TTF";
    EXPECT_FALSE(qrp::instruments::CommodityInstrumentFactory::create_commodity_swing(swing_without_maturity, context));

    qrp::domain::CommoditySwingTrade swing_without_quotes;
    swing_without_quotes.underlier = "TTF";
    swing_without_quotes.maturity_date = "2026-12-24";
    EXPECT_FALSE(qrp::instruments::CommodityInstrumentFactory::create_commodity_swing(swing_without_quotes, context));

    qrp::domain::GasStorageTrade storage_without_maturity;
    storage_without_maturity.underlier = "TTF";
    storage_without_maturity.max_inventory = 100.0;
    EXPECT_FALSE(qrp::instruments::CommodityInstrumentFactory::create_gas_storage(storage_without_maturity, context));

    qrp::domain::GasStorageTrade storage_without_quotes;
    storage_without_quotes.underlier = "TTF";
    storage_without_quotes.maturity_date = "2026-12-24";
    storage_without_quotes.max_inventory = 100.0;
    EXPECT_FALSE(qrp::instruments::CommodityInstrumentFactory::create_gas_storage(storage_without_quotes, context));
}

TEST(CommodityProductsTest, SwingFactoryFallsBackToUnderlierForwardQuote) {
    qrp::market::MarketSnapshot market(make_commodity_market());
    qrp::analytics::PricingContext context(market.built_state());

    qrp::domain::CommoditySwingTrade swing;
    swing.currency = "USD";
    swing.direction = "long";
    swing.underlier = "TTF";
    swing.maturity_date = "2026-12-24";
    swing.min_quantity = 100.0;
    swing.max_quantity = 200.0;
    swing.strike_price = 31.0;
    swing.volatility = 0.40;

    auto instrument = qrp::instruments::CommodityInstrumentFactory::create_commodity_swing(swing, context);

    ASSERT_TRUE(instrument);
    EXPECT_GT(instrument->NPV(), 0.0);
}

TEST(CommodityProductsTest, CommodityOptionsUseIntrinsicFallbackWhenVolatilityIsZero) {
    auto market_dto = make_commodity_market();
    for (auto& quote : market_dto.quotes) {
        if (quote.id == "WTI_VOL_6M_ATM") {
            quote.value = 0.0;
        }
    }
    qrp::market::MarketSnapshot market(market_dto);
    qrp::analytics::PricingContext context(market.built_state());

    auto future_option =
        make_commodity_trade<qrp::domain::CommodityFutureOptionTrade>("commodity_future_option_intrinsic", "long");
    future_option->quantity = 1.0;
    future_option->contract_size = 1000.0;
    future_option->strike_price = 78.00;
    future_option->expiry_date = "2026-09-24";
    future_option->maturity_date = "2026-09-24";
    future_option->future_quote_id = "WTI_FUT_6M";
    future_option->volatility_quote_id = "WTI_VOL_6M_ATM";

    auto spread_option =
        make_commodity_trade<qrp::domain::CommodityCalendarSpreadOptionTrade>("commodity_calendar_spread_intrinsic",
                                                                              "long");
    spread_option->quantity = 1.0;
    spread_option->contract_size = 1000.0;
    spread_option->strike_spread = 0.50;
    spread_option->expiry_date = "2026-09-24";
    spread_option->near_future_quote_id = "WTI_FUT_6M";
    spread_option->far_future_quote_id = "WTI_FUT_9M";
    spread_option->volatility_quote_id = "WTI_VOL_6M_ATM";

    const auto future_option_instrument =
        qrp::instruments::CommodityInstrumentFactory::create_commodity_future_option(*future_option, context);
    const auto spread_option_instrument =
        qrp::instruments::CommodityInstrumentFactory::create_commodity_calendar_spread_option(*spread_option, context);
    ASSERT_TRUE(future_option_instrument);
    ASSERT_TRUE(spread_option_instrument);

    EXPECT_GT(future_option_instrument->NPV(), 0.0);
    EXPECT_GT(spread_option_instrument->NPV(), future_option_instrument->NPV());
}

TEST(CommodityProductsTest, SwingStatefulDpValuesAdditionalExerciseRights) {
    qrp::market::MarketSnapshot market(make_commodity_market());
    qrp::analytics::PricingContext context(market.built_state());

    qrp::domain::CommoditySwingTrade limited;
    limited.currency = "USD";
    limited.direction = "long";
    limited.underlier = "TTF";
    limited.maturity_date = "2026-12-24";
    limited.min_quantity = 100.0;
    limited.max_quantity = 150.0;
    limited.max_exercise_quantity = 100.0;
    limited.strike_price = 31.0;
    limited.forward_quote_ids = {"TTF_FWD_Q1", "TTF_FWD_Q2"};
    limited.exercise_dates = {"2026-06-24", "2026-09-24"};
    limited.volatility = 0.40;

    qrp::domain::CommoditySwingTrade flexible = limited;
    flexible.max_quantity = 250.0;
    flexible.max_exercise_quantity = 125.0;

    const auto limited_instrument =
        qrp::instruments::CommodityInstrumentFactory::create_commodity_swing(limited, context);
    const auto flexible_instrument =
        qrp::instruments::CommodityInstrumentFactory::create_commodity_swing(flexible, context);
    ASSERT_TRUE(limited_instrument);
    ASSERT_TRUE(flexible_instrument);

    EXPECT_GT(limited_instrument->NPV(), 0.0);
    EXPECT_GT(flexible_instrument->NPV(), limited_instrument->NPV());
}

TEST(CommodityProductsTest, GasStorageStatefulDpValuesAdditionalCapacity) {
    qrp::market::MarketSnapshot market(make_commodity_market());
    qrp::analytics::PricingContext context(market.built_state());

    qrp::domain::GasStorageTrade limited;
    limited.currency = "USD";
    limited.direction = "long";
    limited.underlier = "TTF";
    limited.maturity_date = "2026-12-24";
    limited.min_inventory = 0.0;
    limited.max_inventory = 100.0;
    limited.initial_inventory = 50.0;
    limited.terminal_inventory_target = 50.0;
    limited.terminal_inventory_penalty = 100.0;
    limited.max_injection_quantity = 50.0;
    limited.max_withdrawal_quantity = 50.0;
    limited.injection_cost = 0.20;
    limited.withdrawal_cost = 0.20;
    limited.forward_quote_ids = {"TTF_FWD_Q1", "TTF_FWD_Q2"};
    limited.exercise_dates = {"2026-06-24", "2026-09-24"};

    qrp::domain::GasStorageTrade flexible = limited;
    flexible.max_inventory = 200.0;
    flexible.max_injection_quantity = 100.0;
    flexible.max_withdrawal_quantity = 100.0;

    const auto limited_instrument = qrp::instruments::CommodityInstrumentFactory::create_gas_storage(limited, context);
    const auto flexible_instrument =
        qrp::instruments::CommodityInstrumentFactory::create_gas_storage(flexible, context);
    ASSERT_TRUE(limited_instrument);
    ASSERT_TRUE(flexible_instrument);

    EXPECT_GT(limited_instrument->NPV(), 0.0);
    EXPECT_GT(flexible_instrument->NPV(), limited_instrument->NPV());
}

TEST(CommodityProductsTest, GasStorageFactoryHandlesQuoteFallbackAndExerciseDateDefaults) {
    qrp::market::MarketSnapshot market(make_commodity_market());
    qrp::analytics::PricingContext context(market.built_state());

    qrp::domain::GasStorageTrade fallback;
    fallback.currency = "USD";
    fallback.direction = "long";
    fallback.underlier = "TTF";
    fallback.maturity_date = "2026-12-24";
    fallback.min_inventory = 0.0;
    fallback.max_inventory = 120.0;
    fallback.initial_inventory = 50.0;
    fallback.terminal_inventory_target = 50.0;
    fallback.terminal_inventory_penalty = 100.0;
    fallback.max_injection_quantity = 70.0;
    fallback.max_withdrawal_quantity = 70.0;
    fallback.injection_cost = 0.20;
    fallback.withdrawal_cost = 0.20;

    const auto fallback_instrument =
        qrp::instruments::CommodityInstrumentFactory::create_gas_storage(fallback, context);
    ASSERT_TRUE(fallback_instrument);
    EXPECT_TRUE(std::isfinite(fallback_instrument->NPV()));

    qrp::domain::GasStorageTrade sparse_dates = fallback;
    sparse_dates.forward_quote_ids = {"TTF_FWD_Q1", "TTF_FWD_Q2"};
    sparse_dates.exercise_dates = {"2026-06-24"};

    const auto sparse_dates_instrument =
        qrp::instruments::CommodityInstrumentFactory::create_gas_storage(sparse_dates, context);
    ASSERT_TRUE(sparse_dates_instrument);
    EXPECT_TRUE(std::isfinite(sparse_dates_instrument->NPV()));
}
