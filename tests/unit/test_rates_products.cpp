// Verifies Rates product valuation coverage across cash, linear, bond, and option products.

#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/instruments/instrument_factory.hpp>
#include <qrp/market/market_snapshot.hpp>

#include <gtest/gtest.h>

#include <cmath>
#include <map>
#include <memory>
#include <string>

namespace {

qrp::domain::MarketQuote make_quote(const std::string& id,
                                    qrp::domain::QuoteInstrumentType instrument_type,
                                    qrp::domain::QuoteType quote_type,
                                    const std::string& tenor,
                                    double value,
                                    const std::string& index_family = {}) {
    qrp::domain::MarketQuote quote;
    quote.id = id;
    quote.instrument_type = instrument_type;
    quote.quote_type = quote_type;
    quote.currency = qrp::domain::Currency::USD;
    quote.tenor = tenor;
    quote.value = value;
    quote.index_family = index_family;
    quote.source_name = "UnitTest";
    quote.source_ts = "2026-03-24T12:00:00Z";
    return quote;
}

qrp::domain::MarketSnapshot make_rates_market() {
    qrp::domain::MarketSnapshot market;
    market.valuation_date = "2026-03-24";
    market.snapshot_id = "UNIT_RATES_PRODUCTS";
    market.base_currency = qrp::domain::Currency::USD;
    market.default_stale_after_days = 10;

    market.quotes = {
        make_quote("USD_ON", qrp::domain::QuoteInstrumentType::Deposit, qrp::domain::QuoteType::Deposit, "1D", 0.0525),
        make_quote("USD_OIS_1M", qrp::domain::QuoteInstrumentType::OIS, qrp::domain::QuoteType::OIS, "1M", 0.0528),
        make_quote("USD_OIS_3M", qrp::domain::QuoteInstrumentType::OIS, qrp::domain::QuoteType::OIS, "3M", 0.0530),
        make_quote("USD_OIS_6M", qrp::domain::QuoteInstrumentType::OIS, qrp::domain::QuoteType::OIS, "6M", 0.0532),
        make_quote("USD_OIS_1Y", qrp::domain::QuoteInstrumentType::OIS, qrp::domain::QuoteType::OIS, "1Y", 0.0535),
        make_quote("USD_OIS_2Y", qrp::domain::QuoteInstrumentType::OIS, qrp::domain::QuoteType::OIS, "2Y", 0.0537),
        make_quote("USD_OIS_5Y", qrp::domain::QuoteInstrumentType::OIS, qrp::domain::QuoteType::OIS, "5Y", 0.0540),
        make_quote("USD_OIS_10Y", qrp::domain::QuoteInstrumentType::OIS, qrp::domain::QuoteType::OIS, "10Y", 0.0545),
        make_quote("USD_LIBOR_3M_FRA_3M",
                   qrp::domain::QuoteInstrumentType::FRA,
                   qrp::domain::QuoteType::FRA,
                   "3M",
                   0.0540,
                   "IBOR_3M"),
        make_quote("USD_LIBOR_3M_IRS_2Y",
                   qrp::domain::QuoteInstrumentType::IRS,
                   qrp::domain::QuoteType::IRS,
                   "2Y",
                   0.0550,
                   "IBOR_3M"),
        make_quote("USD_LIBOR_3M_IRS_5Y",
                   qrp::domain::QuoteInstrumentType::IRS,
                   qrp::domain::QuoteType::IRS,
                   "5Y",
                   0.0560,
                   "IBOR_3M"),
        make_quote("USD_IR_FUT_JUN26",
                   qrp::domain::QuoteInstrumentType::InterestRateFuture,
                   qrp::domain::QuoteType::InterestRateFuture,
                   "3M",
                   94.50,
                   "IBOR_3M"),
        make_quote("USD_CAP_VOL_2Y",
                   qrp::domain::QuoteInstrumentType::CapFloorVol,
                   qrp::domain::QuoteType::Volatility,
                   "2Y",
                   0.20),
        make_quote("USD_SWAPTION_VOL_1Y_5Y",
                   qrp::domain::QuoteInstrumentType::SwaptionVol,
                   qrp::domain::QuoteType::Volatility,
                   "5Y",
                   0.18)};
    market.quotes[11].expiry = "2026-06-17";
    market.quotes[12].expiry = "2Y";
    market.quotes[12].strike = "ATM";
    market.quotes[13].expiry = "1Y";
    market.quotes[13].strike = "5Y";

    qrp::domain::CurveSpec ibor;
    ibor.id = {qrp::domain::Currency::USD, "IBOR_3M"};
    ibor.purpose = qrp::domain::CurvePurpose::Forward3M;
    ibor.quote_ids = {"USD_ON", "USD_LIBOR_3M_FRA_3M", "USD_LIBOR_3M_IRS_2Y", "USD_LIBOR_3M_IRS_5Y"};
    ibor.day_count = qrp::domain::DayCount::ACT360;
    ibor.calendar = qrp::domain::BusinessCalendar::US;
    ibor.interpolation = qrp::domain::InterpolationType::LogLinear;

    qrp::domain::CurveSpec ois;
    ois.id = {qrp::domain::Currency::USD, "OIS"};
    ois.purpose = qrp::domain::CurvePurpose::OISDiscount;
    ois.quote_ids =
        {"USD_ON", "USD_OIS_1M", "USD_OIS_3M", "USD_OIS_6M", "USD_OIS_1Y", "USD_OIS_2Y", "USD_OIS_5Y", "USD_OIS_10Y"};
    ois.day_count = qrp::domain::DayCount::ACT360;
    ois.calendar = qrp::domain::BusinessCalendar::US;
    ois.interpolation = qrp::domain::InterpolationType::LogLinear;

    market.curves = {ibor, ois};
    market.fixings["USD_LIBOR_3M"]["2026-03-24"] = 0.0540;
    return market;
}

template <typename TradeT>
std::shared_ptr<TradeT> make_rates_trade(const std::string& id, const std::string& direction) {
    auto trade = std::make_shared<TradeT>();
    trade->id = id;
    trade->asset_class = "rates";
    trade->asset_class_type = qrp::domain::AssetClass::Rates;
    trade->currency = "USD";
    trade->direction = direction;
    trade->book = "BOOK:RATES";
    trade->strategy = "RATES_PRODUCTS";
    return trade;
}

qrp::domain::Portfolio make_rates_portfolio() {
    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "rates_products";

    auto deposit = make_rates_trade<qrp::domain::DepositTrade>("deposit_usd", "lend");
    deposit->notional = 1'000'000.0;
    deposit->start_date = "2026-03-26";
    deposit->maturity_date = "2026-06-26";
    deposit->deposit_rate = 0.0520;
    portfolio.trades.push_back(deposit);

    auto fra = make_rates_trade<qrp::domain::FraTrade>("fra_usd", "long");
    fra->notional = 5'000'000.0;
    fra->start_date = "2026-06-24";
    fra->maturity_date = "2026-09-24";
    fra->strike_rate = 0.0535;
    fra->floating_index = "USD_LIBOR_3M";
    portfolio.trades.push_back(fra);

    auto future = make_rates_trade<qrp::domain::InterestRateFutureTrade>("future_usd", "long");
    future->quantity = 10.0;
    future->contract_size = 2500.0;
    future->reference_price = 94.25;
    future->start_date = "2026-06-17";
    future->maturity_date = "2026-09-17";
    future->floating_index = "USD_LIBOR_3M";
    future->future_quote_id = "USD_IR_FUT_JUN26";
    portfolio.trades.push_back(future);

    auto swap = make_rates_trade<qrp::domain::VanillaSwapTrade>("swap_usd", "pay_fixed");
    swap->notional = 5'000'000.0;
    swap->start_date = "2026-03-26";
    swap->maturity_date = "2031-03-26";
    swap->fixed_rate = 0.0520;
    swap->floating_index = "USD_LIBOR_3M";
    portfolio.trades.push_back(swap);

    auto ois = make_rates_trade<qrp::domain::OisSwapTrade>("ois_usd", "receive_fixed");
    ois->notional = 5'000'000.0;
    ois->start_date = "2026-03-26";
    ois->maturity_date = "2031-03-26";
    ois->fixed_rate = 0.0520;
    ois->overnight_index = "SOFR";
    portfolio.trades.push_back(ois);

    auto fixed_bond = make_rates_trade<qrp::domain::FixedRateBondTrade>("fixed_bond_usd", "long");
    fixed_bond->notional = 2'000'000.0;
    fixed_bond->start_date = "2026-03-26";
    fixed_bond->maturity_date = "2031-03-26";
    fixed_bond->coupon_rate = 0.0550;
    fixed_bond->frequency = "Annual";
    portfolio.trades.push_back(fixed_bond);

    auto callable_bond = make_rates_trade<qrp::domain::CallableBondTrade>("callable_bond_usd", "long");
    callable_bond->notional = 2'000'000.0;
    callable_bond->start_date = "2026-03-26";
    callable_bond->maturity_date = "2031-03-26";
    callable_bond->coupon_rate = 0.0625;
    callable_bond->frequency = "Annual";
    callable_bond->call_dates = {"2027-03-26", "2028-03-26", "2029-03-26"};
    callable_bond->call_prices = {101.0, 100.5, 100.0};
    callable_bond->volatility = 0.0110;
    portfolio.trades.push_back(callable_bond);

    auto floater = make_rates_trade<qrp::domain::FloatingRateNoteTrade>("frn_usd", "long");
    floater->notional = 2'000'000.0;
    floater->start_date = "2026-03-26";
    floater->maturity_date = "2029-03-26";
    floater->floating_index = "USD_LIBOR_3M";
    floater->frequency = "Quarterly";
    floater->spread = 0.0010;
    portfolio.trades.push_back(floater);

    auto cap = make_rates_trade<qrp::domain::CapFloorTrade>("cap_usd", "long");
    cap->notional = 3'000'000.0;
    cap->start_date = "2026-06-24";
    cap->maturity_date = "2028-06-24";
    cap->strike_rate = 0.0550;
    cap->cap_floor_type = "cap";
    cap->floating_index = "USD_LIBOR_3M";
    cap->volatility_quote_id = "USD_CAP_VOL_2Y";
    portfolio.trades.push_back(cap);

    auto european_swaption = make_rates_trade<qrp::domain::EuropeanSwaptionTrade>("european_swaption_usd", "payer");
    european_swaption->notional = 4'000'000.0;
    european_swaption->option_expiry_date = "2026-06-24";
    european_swaption->start_date = "2026-06-24";
    european_swaption->maturity_date = "2031-06-24";
    european_swaption->fixed_rate = 0.0550;
    european_swaption->floating_index = "USD_LIBOR_3M";
    european_swaption->volatility_quote_id = "USD_SWAPTION_VOL_1Y_5Y";
    portfolio.trades.push_back(european_swaption);

    auto bermudan_swaption = make_rates_trade<qrp::domain::BermudanSwaptionTrade>("bermudan_swaption_usd", "payer");
    bermudan_swaption->notional = 4'000'000.0;
    bermudan_swaption->start_date = "2026-06-24";
    bermudan_swaption->maturity_date = "2031-06-24";
    bermudan_swaption->fixed_rate = 0.0550;
    bermudan_swaption->floating_index = "USD_LIBOR_3M";
    bermudan_swaption->volatility = 0.0120;
    bermudan_swaption->exercise_dates = {"2026-06-24", "2027-06-24", "2028-06-24", "2029-06-24"};
    portfolio.trades.push_back(bermudan_swaption);

    return portfolio;
}

} // namespace

TEST(RatesProductsTest, PricesRatesProducts) {
    qrp::market::MarketSnapshot market(make_rates_market());
    qrp::analytics::PricingContext context(market.built_state());
    const auto portfolio = make_rates_portfolio();

    const auto results = qrp::analytics::ValuationService::price_portfolio(portfolio, context);

    ASSERT_EQ(results.size(), 11);

    std::map<std::string, qrp::analytics::ValuationResult> by_trade;
    for (const auto& result : results) {
        by_trade[result.trade_id] = result;
        EXPECT_EQ(result.asset_class, qrp::domain::AssetClass::Rates) << result.trade_id;
        EXPECT_EQ(result.support_status, qrp::domain::SupportStatus::Supported) << result.trade_id;
        EXPECT_TRUE(std::isfinite(result.npv)) << result.trade_id;
    }

    EXPECT_EQ(by_trade.at("deposit_usd").product_type, qrp::domain::ProductType::Deposit);
    EXPECT_EQ(by_trade.at("fra_usd").product_type, qrp::domain::ProductType::Fra);
    EXPECT_EQ(by_trade.at("future_usd").product_type, qrp::domain::ProductType::InterestRateFuture);
    EXPECT_EQ(by_trade.at("swap_usd").product_type, qrp::domain::ProductType::VanillaSwap);
    EXPECT_EQ(by_trade.at("ois_usd").product_type, qrp::domain::ProductType::OisSwap);
    EXPECT_EQ(by_trade.at("fixed_bond_usd").product_type, qrp::domain::ProductType::FixedRateBond);
    EXPECT_EQ(by_trade.at("callable_bond_usd").product_type, qrp::domain::ProductType::CallableBond);
    EXPECT_EQ(by_trade.at("frn_usd").product_type, qrp::domain::ProductType::FloatingRateNote);
    EXPECT_EQ(by_trade.at("cap_usd").product_type, qrp::domain::ProductType::CapFloor);
    EXPECT_EQ(by_trade.at("european_swaption_usd").product_type, qrp::domain::ProductType::EuropeanSwaption);
    EXPECT_EQ(by_trade.at("bermudan_swaption_usd").product_type, qrp::domain::ProductType::BermudanSwaption);

    EXPECT_NEAR(by_trade.at("future_usd").npv, 6250.0, 1.0e-10);
    EXPECT_GE(by_trade.at("cap_usd").npv, 0.0);
    EXPECT_GE(by_trade.at("european_swaption_usd").npv, 0.0);
    EXPECT_GE(by_trade.at("bermudan_swaption_usd").npv, 0.0);
    EXPECT_LE(by_trade.at("callable_bond_usd").npv, by_trade.at("fixed_bond_usd").npv * 1.1);
}

TEST(RatesProductsTest, BermudanSwaptionsUseReproducibleExercisePolicyLsmcPath) {
    qrp::market::MarketSnapshot market(make_rates_market());
    qrp::analytics::PricingContext context(market.built_state());

    auto bermudan_swaption = make_rates_trade<qrp::domain::BermudanSwaptionTrade>("bermudan_swaption_usd", "payer");
    bermudan_swaption->notional = 4'000'000.0;
    bermudan_swaption->start_date = "2026-06-24";
    bermudan_swaption->maturity_date = "2031-06-24";
    bermudan_swaption->fixed_rate = 0.0520;
    bermudan_swaption->floating_index = "USD_LIBOR_3M";
    bermudan_swaption->volatility = 0.0120;
    bermudan_swaption->exercise_dates = {"2026-06-24", "2027-06-24", "2028-06-24", "2029-06-24"};

    const auto first = qrp::instruments::RatesInstrumentFactory::create_bermudan_swaption(*bermudan_swaption, context);
    const auto second = qrp::instruments::RatesInstrumentFactory::create_bermudan_swaption(*bermudan_swaption, context);
    ASSERT_TRUE(first);
    ASSERT_TRUE(second);

    const double first_npv = first->NPV();
    const double first_error = first->errorEstimate();

    EXPECT_DOUBLE_EQ(first_npv, second->NPV());
    EXPECT_DOUBLE_EQ(first_error, second->errorEstimate());
    EXPECT_GT(first_npv, 0.0);
    EXPECT_GT(first_error, 0.0);
}

TEST(RatesProductsTest, CallableBondsSubtractIssuerCallValueFromStraightBond) {
    qrp::market::MarketSnapshot market(make_rates_market());
    qrp::analytics::PricingContext context(market.built_state());

    auto straight_bond = make_rates_trade<qrp::domain::FixedRateBondTrade>("straight_bond_usd", "long");
    straight_bond->notional = 2'000'000.0;
    straight_bond->start_date = "2026-03-26";
    straight_bond->maturity_date = "2031-03-26";
    straight_bond->coupon_rate = 0.0650;
    straight_bond->frequency = "Annual";

    auto callable_bond = make_rates_trade<qrp::domain::CallableBondTrade>("callable_bond_usd", "long");
    callable_bond->notional = straight_bond->notional;
    callable_bond->start_date = straight_bond->start_date;
    callable_bond->maturity_date = straight_bond->maturity_date;
    callable_bond->coupon_rate = straight_bond->coupon_rate;
    callable_bond->frequency = straight_bond->frequency;
    callable_bond->call_dates = {"2027-03-26", "2028-03-26", "2029-03-26", "2030-03-26"};
    callable_bond->call_prices = {101.0, 100.75, 100.5, 100.0};
    callable_bond->volatility = 0.0120;

    const auto straight = qrp::instruments::RatesInstrumentFactory::create_bond(*straight_bond, context);
    const auto first_callable = qrp::instruments::RatesInstrumentFactory::create_callable_bond(*callable_bond, context);
    const auto second_callable =
        qrp::instruments::RatesInstrumentFactory::create_callable_bond(*callable_bond, context);
    ASSERT_TRUE(straight);
    ASSERT_TRUE(first_callable);
    ASSERT_TRUE(second_callable);

    const double straight_npv = straight->NPV();
    const double callable_npv = first_callable->NPV();

    EXPECT_GT(straight_npv, 0.0);
    EXPECT_GT(callable_npv, 0.0);
    EXPECT_LE(callable_npv, straight_npv);
    EXPECT_DOUBLE_EQ(callable_npv, second_callable->NPV());
    EXPECT_DOUBLE_EQ(first_callable->errorEstimate(), second_callable->errorEstimate());
    EXPECT_GT(first_callable->errorEstimate(), 0.0);
}

TEST(RatesProductsTest, PricesAlternativeDirectionsAndFallbackInputs) {
    auto market_dto = make_rates_market();
    market_dto.quotes.push_back(make_quote("USD_IR_FUT_DEC26",
                                           qrp::domain::QuoteInstrumentType::InterestRateFuture,
                                           qrp::domain::QuoteType::InterestRateFuture,
                                           "3M",
                                           0.0550,
                                           "IBOR_3M"));
    market_dto.fixings["USD_LIBOR_1M"]["2026-03-24"] = 0.0535;
    market_dto.fixings["USD_LIBOR_6M"]["2026-03-24"] = 0.0545;
    market_dto.fixings["OIS"]["2026-03-24"] = 0.0525;

    qrp::market::MarketSnapshot market(market_dto);
    qrp::analytics::PricingContext context(market.built_state());

    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "rates_edge_cases";

    auto borrow_deposit = make_rates_trade<qrp::domain::DepositTrade>("deposit_borrow_usd", "borrow");
    borrow_deposit->notional = 1'000'000.0;
    borrow_deposit->start_date = "2026-03-26";
    borrow_deposit->maturity_date = "2026-06-26";
    borrow_deposit->deposit_rate = 0.0520;
    portfolio.trades.push_back(borrow_deposit);

    auto implied_future = make_rates_trade<qrp::domain::InterestRateFutureTrade>("future_implied_usd", "long");
    implied_future->quantity = 5.0;
    implied_future->contract_size = 2500.0;
    implied_future->reference_price = 94.25;
    implied_future->start_date = "2026-12-16";
    implied_future->maturity_date = "2027-03-16";
    implied_future->floating_index = "USD_LIBOR_3M";
    portfolio.trades.push_back(implied_future);

    auto quoted_rate_future =
        make_rates_trade<qrp::domain::InterestRateFutureTrade>("future_decimal_quote_usd", "long");
    quoted_rate_future->quantity = 5.0;
    quoted_rate_future->contract_size = 2500.0;
    quoted_rate_future->reference_price = 94.25;
    quoted_rate_future->start_date = "2026-12-16";
    quoted_rate_future->maturity_date = "2027-03-16";
    quoted_rate_future->floating_index = "USD_LIBOR_3M";
    quoted_rate_future->future_quote_id = "USD_IR_FUT_DEC26";
    portfolio.trades.push_back(quoted_rate_future);

    auto short_fra_6m = make_rates_trade<qrp::domain::FraTrade>("fra_short_6m_usd", "lend");
    short_fra_6m->notional = 1'000'000.0;
    short_fra_6m->start_date = "2026-06-24";
    short_fra_6m->maturity_date = "2026-12-24";
    short_fra_6m->strike_rate = 0.0540;
    short_fra_6m->floating_index = "USD_LIBOR_6M";
    portfolio.trades.push_back(short_fra_6m);

    auto receiver_swap = make_rates_trade<qrp::domain::VanillaSwapTrade>("swap_default_index_usd", "receiver");
    receiver_swap->notional = 2'000'000.0;
    receiver_swap->start_date = "2026-03-26";
    receiver_swap->maturity_date = "2029-03-26";
    receiver_swap->fixed_rate = 0.0525;
    receiver_swap->floating_index = "";
    portfolio.trades.push_back(receiver_swap);

    auto ois = make_rates_trade<qrp::domain::OisSwapTrade>("ois_pay_fixed_usd", "pay_fixed");
    ois->notional = 2'000'000.0;
    ois->start_date = "2026-03-26";
    ois->maturity_date = "2029-03-26";
    ois->fixed_rate = 0.0525;
    ois->overnight_index = "OIS";
    ois->spread = 0.0001;
    portfolio.trades.push_back(ois);

    auto semiannual_bond = make_rates_trade<qrp::domain::FixedRateBondTrade>("bond_semiannual_usd", "long");
    semiannual_bond->notional = 1'000'000.0;
    semiannual_bond->start_date = "2026-03-26";
    semiannual_bond->maturity_date = "2029-03-26";
    semiannual_bond->coupon_rate = 0.0530;
    semiannual_bond->frequency = "semi-annual";
    portfolio.trades.push_back(semiannual_bond);

    auto monthly_floater = make_rates_trade<qrp::domain::FloatingRateNoteTrade>("frn_monthly_1m_usd", "long");
    monthly_floater->notional = 1'000'000.0;
    monthly_floater->start_date = "2026-03-26";
    monthly_floater->maturity_date = "2027-03-26";
    monthly_floater->floating_index = "USD_LIBOR_1M";
    monthly_floater->frequency = "Monthly";
    monthly_floater->spread = 0.0005;
    portfolio.trades.push_back(monthly_floater);

    auto floor = make_rates_trade<qrp::domain::CapFloorTrade>("floor_default_vol_usd", "long");
    floor->notional = 1'500'000.0;
    floor->start_date = "2026-06-24";
    floor->maturity_date = "2028-06-24";
    floor->strike_rate = 0.0520;
    floor->cap_floor_type = "floor";
    floor->floating_index = "USD_LIBOR_3M";
    portfolio.trades.push_back(floor);

    auto bermudan_default_exercise =
        make_rates_trade<qrp::domain::BermudanSwaptionTrade>("bermudan_default_exercise_usd", "receiver");
    bermudan_default_exercise->notional = 1'000'000.0;
    bermudan_default_exercise->start_date = "2026-06-24";
    bermudan_default_exercise->maturity_date = "2029-06-24";
    bermudan_default_exercise->fixed_rate = 0.0520;
    bermudan_default_exercise->floating_index = "USD_LIBOR_3M";
    portfolio.trades.push_back(bermudan_default_exercise);

    const auto results = qrp::analytics::ValuationService::price_portfolio(portfolio, context);

    ASSERT_EQ(results.size(), 10U);
    for (const auto& result : results) {
        EXPECT_EQ(result.support_status, qrp::domain::SupportStatus::Supported) << result.trade_id;
        EXPECT_TRUE(std::isfinite(result.npv)) << result.trade_id;
    }
}

TEST(RatesProductsTest, FactoriesReturnNullWhenRatesCurvesAreMissing) {
    qrp::domain::MarketSnapshot market_dto;
    market_dto.valuation_date = "2026-03-24";
    qrp::market::MarketSnapshot market(market_dto);
    qrp::analytics::PricingContext context(market.built_state());

    qrp::domain::InterestRateFutureTrade future;
    future.currency = "USD";
    future.start_date = "2026-06-17";
    future.maturity_date = "2026-09-17";
    future.floating_index = "USD_LIBOR_3M";
    EXPECT_FALSE(qrp::instruments::RatesInstrumentFactory::create_interest_rate_future(future, context));

    qrp::domain::OisSwapTrade ois;
    ois.currency = "USD";
    ois.start_date = "2026-03-26";
    ois.maturity_date = "2029-03-26";
    ois.overnight_index = "SOFR";
    EXPECT_FALSE(qrp::instruments::RatesInstrumentFactory::create_ois_swap(ois, context));

    qrp::domain::EuropeanSwaptionTrade european_swaption;
    european_swaption.currency = "USD";
    european_swaption.start_date = "2026-06-24";
    european_swaption.maturity_date = "2031-06-24";
    european_swaption.floating_index = "USD_LIBOR_3M";
    EXPECT_FALSE(qrp::instruments::RatesInstrumentFactory::create_european_swaption(european_swaption, context));

    qrp::domain::BermudanSwaptionTrade bermudan_swaption;
    bermudan_swaption.currency = "USD";
    bermudan_swaption.start_date = "2026-06-24";
    bermudan_swaption.maturity_date = "2031-06-24";
    bermudan_swaption.floating_index = "USD_LIBOR_3M";
    EXPECT_FALSE(qrp::instruments::RatesInstrumentFactory::create_bermudan_swaption(bermudan_swaption, context));

    qrp::domain::CallableBondTrade callable_bond;
    callable_bond.currency = "USD";
    callable_bond.start_date = "2026-03-26";
    callable_bond.maturity_date = "2031-03-26";
    callable_bond.frequency = "Annual";
    EXPECT_FALSE(qrp::instruments::RatesInstrumentFactory::create_callable_bond(callable_bond, context));
}
