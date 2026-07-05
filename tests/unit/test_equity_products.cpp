// Verifies equity product valuation coverage.

#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/domain/portfolio.hpp>
#include <qrp/instruments/instrument_factory.hpp>
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

qrp::domain::MarketSnapshot make_equity_market() {
    qrp::domain::MarketSnapshot market;
    market.valuation_date = "2026-03-24";
    market.snapshot_id = "UNIT_EQUITY_PRODUCTS";
    market.base_currency = qrp::domain::Currency::USD;
    market.default_stale_after_days = 10;

    market.quotes = {
        make_quote("USD_ON", qrp::domain::QuoteInstrumentType::Deposit, qrp::domain::QuoteType::Deposit, "1D", 0.0525),
        make_quote("USD_OIS_1Y", qrp::domain::QuoteInstrumentType::OIS, qrp::domain::QuoteType::OIS, "1Y", 0.0535),
        make_quote("AAPL", qrp::domain::QuoteInstrumentType::EquitySpot, qrp::domain::QuoteType::EquitySpot, "SPOT", 185.50, "AAPL"),
        make_quote("AAPL_DIVYLD_1Y", qrp::domain::QuoteInstrumentType::DividendYield, qrp::domain::QuoteType::DividendYield, "1Y", 0.0050, "AAPL"),
        make_quote("AAPL_BORROW_1Y", qrp::domain::QuoteInstrumentType::BorrowRate, qrp::domain::QuoteType::BorrowRate, "1Y", 0.0020, "AAPL"),
        make_quote("AAPL_VOL_6M_ATM", qrp::domain::QuoteInstrumentType::EquityVol, qrp::domain::QuoteType::Volatility, "6M", 0.2450, "AAPL"),
        make_quote("SPX", qrp::domain::QuoteInstrumentType::EquitySpot, qrp::domain::QuoteType::EquitySpot, "SPOT", 5350.0, "SPX"),
        make_quote("ES_FUT_6M", qrp::domain::QuoteInstrumentType::Future, qrp::domain::QuoteType::Future, "6M", 5355.0, "SPX")
    };
    market.quotes[5].expiry = "6M";
    market.quotes[5].strike = "ATM";
    market.curves = {make_usd_ois_curve()};
    return market;
}

template <typename TradeT>
std::shared_ptr<TradeT> make_equity_trade(const std::string& id, const std::string& direction) {
    auto trade = std::make_shared<TradeT>();
    trade->id = id;
    trade->asset_class = "equity";
    trade->asset_class_type = qrp::domain::AssetClass::Equity;
    trade->currency = "USD";
    trade->direction = direction;
    trade->book = "BOOK:EQUITY";
    trade->strategy = "EQUITY_PRODUCTS";
    return trade;
}

qrp::domain::Portfolio make_equity_portfolio() {
    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "equity_products";

    auto spot = make_equity_trade<qrp::domain::EquitySpotTrade>("equity_spot_aapl", "long");
    spot->quantity = 100.0;
    spot->reference_price = 180.0;
    spot->underlier = "AAPL";
    portfolio.trades.push_back(spot);

    auto forward = make_equity_trade<qrp::domain::EquityForwardTrade>("equity_forward_aapl", "long");
    forward->quantity = 100.0;
    forward->forward_price = 184.0;
    forward->maturity_date = "2026-09-24";
    forward->underlier = "AAPL";
    forward->dividend_yield_quote_id = "AAPL_DIVYLD_1Y";
    forward->borrow_rate_quote_id = "AAPL_BORROW_1Y";
    portfolio.trades.push_back(forward);

    auto future = make_equity_trade<qrp::domain::EquityFutureTrade>("equity_future_spx", "long");
    future->quantity = 2.0;
    future->contract_size = 50.0;
    future->reference_price = 5350.0;
    future->maturity_date = "2026-09-24";
    future->future_quote_id = "ES_FUT_6M";
    future->underlier = "SPX";
    portfolio.trades.push_back(future);

    auto european_call = make_equity_trade<qrp::domain::EquityOptionTrade>("equity_option_aapl_call", "long");
    european_call->quantity = 100.0;
    european_call->strike_price = 185.0;
    european_call->expiry_date = "2026-09-24";
    european_call->settlement_date = "2026-09-24";
    european_call->option_type = "call";
    european_call->exercise_style = "european";
    european_call->underlier = "AAPL";
    european_call->dividend_yield_quote_id = "AAPL_DIVYLD_1Y";
    european_call->borrow_rate_quote_id = "AAPL_BORROW_1Y";
    european_call->volatility_quote_id = "AAPL_VOL_6M_ATM";
    portfolio.trades.push_back(european_call);

    auto european_put = make_equity_trade<qrp::domain::EquityOptionTrade>("equity_option_aapl_euro_put", "long");
    european_put->quantity = 100.0;
    european_put->strike_price = 190.0;
    european_put->expiry_date = "2026-09-24";
    european_put->settlement_date = "2026-09-24";
    european_put->option_type = "put";
    european_put->exercise_style = "european";
    european_put->underlier = "AAPL";
    european_put->dividend_yield_quote_id = "AAPL_DIVYLD_1Y";
    european_put->borrow_rate_quote_id = "AAPL_BORROW_1Y";
    european_put->volatility_quote_id = "AAPL_VOL_6M_ATM";
    portfolio.trades.push_back(european_put);

    auto american_put = make_equity_trade<qrp::domain::EquityOptionTrade>("equity_option_aapl_american_put", "long");
    american_put->quantity = 100.0;
    american_put->strike_price = 190.0;
    american_put->expiry_date = "2026-09-24";
    american_put->settlement_date = "2026-09-24";
    american_put->option_type = "put";
    american_put->exercise_style = "american";
    american_put->underlier = "AAPL";
    american_put->dividend_yield_quote_id = "AAPL_DIVYLD_1Y";
    american_put->borrow_rate_quote_id = "AAPL_BORROW_1Y";
    american_put->volatility_quote_id = "AAPL_VOL_6M_ATM";
    portfolio.trades.push_back(american_put);

    return portfolio;
}

} // namespace

TEST(EquityProductsTest, PricesEquityProducts) {
    qrp::market::MarketSnapshot market(make_equity_market());
    qrp::analytics::PricingContext context(market.built_state());
    const auto portfolio = make_equity_portfolio();

    const auto results = qrp::analytics::ValuationService::price_portfolio(portfolio, context);

    ASSERT_EQ(results.size(), 6U);

    std::map<std::string, qrp::analytics::ValuationResult> by_trade;
    for (const auto& result : results) {
        by_trade[result.trade_id] = result;
        EXPECT_EQ(result.asset_class, qrp::domain::AssetClass::Equity) << result.trade_id;
        EXPECT_EQ(result.support_status, qrp::domain::SupportStatus::Supported) << result.trade_id;
        EXPECT_TRUE(std::isfinite(result.npv)) << result.trade_id;
    }

    EXPECT_EQ(by_trade.at("equity_spot_aapl").product_type, qrp::domain::ProductType::EquitySpot);
    EXPECT_EQ(by_trade.at("equity_forward_aapl").product_type, qrp::domain::ProductType::EquityForward);
    EXPECT_EQ(by_trade.at("equity_future_spx").product_type, qrp::domain::ProductType::EquityFuture);
    EXPECT_EQ(by_trade.at("equity_option_aapl_call").product_type, qrp::domain::ProductType::EquityOption);

    EXPECT_NEAR(by_trade.at("equity_spot_aapl").npv, 550.0, 1.0e-10);
    EXPECT_GT(by_trade.at("equity_forward_aapl").npv, 0.0);
    EXPECT_GT(by_trade.at("equity_future_spx").npv, 0.0);
    EXPECT_GT(by_trade.at("equity_option_aapl_call").npv, 0.0);
    EXPECT_GE(
        by_trade.at("equity_option_aapl_american_put").npv,
        by_trade.at("equity_option_aapl_euro_put").npv);
}

TEST(EquityProductsTest, FactoriesReturnNullWhenEquityQuotesAreMissing) {
    qrp::domain::MarketSnapshot market_dto;
    market_dto.valuation_date = "2026-03-24";
    qrp::market::MarketSnapshot market(market_dto);
    qrp::analytics::PricingContext context(market.built_state());

    qrp::domain::EquityForwardTrade forward;
    forward.underlier = "AAPL";
    forward.maturity_date = "2026-09-24";
    EXPECT_FALSE(qrp::instruments::EquityInstrumentFactory::create_equity_forward(forward, context));

    qrp::domain::EquityFutureTrade future;
    future.underlier = "SPX";
    future.maturity_date = "2026-09-24";
    future.future_quote_id = "MISSING";
    EXPECT_FALSE(qrp::instruments::EquityInstrumentFactory::create_equity_future(future, context));

    qrp::domain::EquityOptionTrade option;
    option.underlier = "AAPL";
    option.expiry_date = "2026-09-24";
    EXPECT_FALSE(qrp::instruments::EquityInstrumentFactory::create_equity_option(option, context));
}
