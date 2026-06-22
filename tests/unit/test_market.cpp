#include <gtest/gtest.h>
#include <qrp/market/market_snapshot.hpp>
#include <ql/termstructures/yield/piecewiseyieldcurve.hpp>
#include <ql/termstructures/yield/oisratehelper.hpp>
#include <ql/indexes/ibor/sofr.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/market/market_state.hpp>
#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace qrp::market;
using namespace qrp;

TEST(MarketTest, TestOISBootstrap) {
    domain::CurveSpec spec;
    spec.id = {domain::Currency::USD, "OIS"};
    spec.day_count = domain::DayCount::ACT360;
    spec.quote_ids = {"USD_OIS_1M", "USD_OIS_3M"};

    std::map<std::string, domain::MarketQuote> quotes;
    
    domain::MarketQuote q1;
    q1.id = "USD_OIS_1M";
    q1.instrument_type = domain::QuoteInstrumentType::OIS;
    q1.currency = domain::Currency::USD;
    q1.tenor = "1M";
    q1.value = 0.05;
    quotes["USD_OIS_1M"] = q1;

    domain::MarketQuote q2;
    q2.id = "USD_OIS_3M";
    q2.instrument_type = domain::QuoteInstrumentType::OIS;
    q2.currency = domain::Currency::USD;
    q2.tenor = "3M";
    q2.value = 0.051;
    quotes["USD_OIS_3M"] = q2;

    QuantLib::Date valuation_date(24, QuantLib::March, 2024);
    
    try {
        auto curve = CurveBuilder::build_curve(spec, quotes, valuation_date, nullptr);
        ASSERT_NE(curve, nullptr);
        std::cout << "Curve built successfully. NPV at 1M: " << curve->discount(valuation_date + QuantLib::Period(1, QuantLib::Months)) << std::endl;
    } catch (const std::exception& e) {
        FAIL() << "Bootstrap failed with exception: " << e.what();
    }
}

TEST(MarketTest, ParseDateRejectsInvalidInput) {
    EXPECT_THROW(CurveBuilder::parse_date("2024-13-01"), std::invalid_argument);
    EXPECT_THROW(CurveBuilder::parse_date("20240324"), std::invalid_argument);
    EXPECT_THROW(CurveBuilder::parse_date(""), std::invalid_argument);
}

TEST(MarketTest, ParseTenorRejectsInvalidInput) {
    EXPECT_THROW(CurveBuilder::parse_tenor("SPOT"), std::invalid_argument);
    EXPECT_THROW(CurveBuilder::parse_tenor("0D"), std::invalid_argument);
    EXPECT_THROW(CurveBuilder::parse_tenor("3Q"), std::invalid_argument);
    EXPECT_THROW(CurveBuilder::parse_tenor(""), std::invalid_argument);
}

TEST(MarketTest, MarketStateResetRestoresQuoteValues) {
    MarketState state(QuantLib::Date(24, QuantLib::March, 2024));
    state.add_quote("USD_OIS_5Y", 0.03);

    domain::MarketSnapshot snapshot;
    domain::MarketQuote quote;
    quote.id = "USD_OIS_5Y";
    quote.value = 0.031;
    snapshot.quotes.push_back(quote);

    state.reset_to_snapshot(snapshot);

    EXPECT_DOUBLE_EQ(state.get_quote("USD_OIS_5Y"), 0.031);
}

TEST(MarketTest, MarketStateCaptureIncludesQuotesAndFixings) {
    MarketState state(QuantLib::Date(24, QuantLib::March, 2024));
    state.add_quote("USD_OIS_2Y", 0.025);

    domain::MarketQuote quote;
    quote.id = "USD_OIS_5Y";
    quote.instrument_type = domain::QuoteInstrumentType::OIS;
    quote.currency = domain::Currency::USD;
    quote.tenor = "5Y";
    quote.value = 0.03;
    quote.index_family = "OIS";
    quote.day_count = domain::DayCount::ACT360;
    quote.calendar = domain::BusinessCalendar::US;
    quote.bdc = domain::BusinessDayConvention::ModifiedFollowing;
    quote.settlement_days = 2;
    state.add_quote(quote);

    state.add_fixing("USD_LIBOR_3M", QuantLib::Date(20, QuantLib::March, 2024), 0.052);

    auto snapshot = state.capture_snapshot();

    EXPECT_EQ(snapshot.valuation_date, "2024-03-24");
    ASSERT_EQ(snapshot.quotes.size(), 2U);

    auto quote_it = std::find_if(snapshot.quotes.begin(), snapshot.quotes.end(), [](const auto& quote) {
        return quote.id == "USD_OIS_5Y";
    });
    ASSERT_NE(quote_it, snapshot.quotes.end());
    EXPECT_DOUBLE_EQ(quote_it->value, 0.03);
    EXPECT_EQ(quote_it->instrument_type, domain::QuoteInstrumentType::OIS);
    EXPECT_EQ(quote_it->currency, domain::Currency::USD);
    EXPECT_EQ(quote_it->tenor, "5Y");
    EXPECT_EQ(quote_it->index_family, "OIS");
    EXPECT_EQ(quote_it->day_count, domain::DayCount::ACT360);
    EXPECT_EQ(quote_it->calendar, domain::BusinessCalendar::US);
    EXPECT_EQ(quote_it->bdc, domain::BusinessDayConvention::ModifiedFollowing);
    EXPECT_EQ(quote_it->settlement_days, 2);
    EXPECT_DOUBLE_EQ(snapshot.fixings.at("USD_LIBOR_3M").at("2024-03-20"), 0.052);
}

TEST(MarketTest, MarketStateResetPreservesQuoteMetadata) {
    MarketState state(QuantLib::Date(24, QuantLib::March, 2024));

    domain::MarketSnapshot snapshot;
    domain::MarketQuote quote;
    quote.id = "AAPL";
    quote.instrument_type = domain::QuoteInstrumentType::Future;
    quote.currency = domain::Currency::USD;
    quote.tenor = "SPOT";
    quote.value = 185.5;
    snapshot.quotes.push_back(quote);

    state.reset_to_snapshot(snapshot);
    state.add_quote("AAPL", 190.0);

    auto captured = state.capture_snapshot();

    ASSERT_EQ(captured.quotes.size(), 1U);
    EXPECT_EQ(captured.quotes[0].id, "AAPL");
    EXPECT_EQ(captured.quotes[0].instrument_type, domain::QuoteInstrumentType::Future);
    EXPECT_EQ(captured.quotes[0].currency, domain::Currency::USD);
    EXPECT_EQ(captured.quotes[0].tenor, "SPOT");
    EXPECT_DOUBLE_EQ(captured.quotes[0].value, 190.0);
}
