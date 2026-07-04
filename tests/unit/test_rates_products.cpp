// Verifies Phase 3 rates product valuation coverage across cash, linear, bond, and option products.

#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/market/market_snapshot.hpp>

#include <gtest/gtest.h>

#include <cmath>
#include <map>
#include <memory>
#include <string>

namespace {

qrp::domain::MarketQuote make_quote(
    const std::string& id,
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
    market.snapshot_id = "UNIT_RATES_PHASE3";
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
        make_quote("USD_LIBOR_3M_FRA_3M", qrp::domain::QuoteInstrumentType::FRA, qrp::domain::QuoteType::FRA, "3M", 0.0540, "IBOR_3M"),
        make_quote("USD_LIBOR_3M_IRS_2Y", qrp::domain::QuoteInstrumentType::IRS, qrp::domain::QuoteType::IRS, "2Y", 0.0550, "IBOR_3M"),
        make_quote("USD_LIBOR_3M_IRS_5Y", qrp::domain::QuoteInstrumentType::IRS, qrp::domain::QuoteType::IRS, "5Y", 0.0560, "IBOR_3M"),
        make_quote("USD_IR_FUT_JUN26", qrp::domain::QuoteInstrumentType::InterestRateFuture, qrp::domain::QuoteType::InterestRateFuture, "3M", 94.50, "IBOR_3M"),
        make_quote("USD_CAP_VOL_2Y", qrp::domain::QuoteInstrumentType::CapFloorVol, qrp::domain::QuoteType::Volatility, "2Y", 0.20),
        make_quote("USD_SWAPTION_VOL_1Y_5Y", qrp::domain::QuoteInstrumentType::SwaptionVol, qrp::domain::QuoteType::Volatility, "5Y", 0.18)
    };
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
    ois.quote_ids = {"USD_ON", "USD_OIS_1M", "USD_OIS_3M", "USD_OIS_6M", "USD_OIS_1Y", "USD_OIS_2Y", "USD_OIS_5Y", "USD_OIS_10Y"};
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
    trade->strategy = "PHASE3";
    return trade;
}

qrp::domain::Portfolio make_phase3_rates_portfolio() {
    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "phase3_rates";

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

TEST(RatesProductsTest, PricesPhase3RatesProducts) {
    qrp::market::MarketSnapshot market(make_rates_market());
    qrp::analytics::PricingContext context(market.built_state());
    const auto portfolio = make_phase3_rates_portfolio();

    const auto results = qrp::analytics::ValuationService::price_portfolio(portfolio, context);

    ASSERT_EQ(results.size(), 10);

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
    EXPECT_EQ(by_trade.at("frn_usd").product_type, qrp::domain::ProductType::FloatingRateNote);
    EXPECT_EQ(by_trade.at("cap_usd").product_type, qrp::domain::ProductType::CapFloor);
    EXPECT_EQ(by_trade.at("european_swaption_usd").product_type, qrp::domain::ProductType::EuropeanSwaption);
    EXPECT_EQ(by_trade.at("bermudan_swaption_usd").product_type, qrp::domain::ProductType::BermudanSwaption);

    EXPECT_NEAR(by_trade.at("future_usd").npv, 6250.0, 1.0e-10);
    EXPECT_GE(by_trade.at("cap_usd").npv, 0.0);
    EXPECT_GE(by_trade.at("european_swaption_usd").npv, 0.0);
    EXPECT_GE(by_trade.at("bermudan_swaption_usd").npv, 0.0);
}
