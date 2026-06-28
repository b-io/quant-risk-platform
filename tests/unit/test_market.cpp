// Verifies market-data parsing, validation gates, market-state reset, and rates-curve construction.

#include <gtest/gtest.h>
#include <qrp/market/market_snapshot.hpp>
#include <ql/termstructures/yield/piecewiseyieldcurve.hpp>
#include <ql/termstructures/yield/oisratehelper.hpp>
#include <ql/indexes/ibor/sofr.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/market/market_state.hpp>
#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>
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
        auto curve = CurveBuilder::build_rate_curve(spec, quotes, valuation_date, nullptr);
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

TEST(MarketTest, ParsesPhase2MarketSnapshotMetadata) {
    const auto json = nlohmann::json::parse(R"json(
        {
          "snapshot_id": "MKT_2026_03_24",
          "schema_version": 2,
          "valuation_date": "2026-03-24",
          "base_currency": "USD",
          "source_name": "unit-test",
          "recorded_ts": "2026-03-24T18:00:00Z",
          "default_stale_after_days": 1,
          "quotes": [
            {
              "id": "EURUSD.SPOT",
              "instrument_type": "FXSpot",
              "quote_type": "FXSpot",
              "currency": "USD",
              "tenor": "SPOT",
              "underlier": "EURUSD",
              "risk_factor_id": "RF:FX:EURUSD:SPOT",
              "value": 1.085,
              "source_name": "Composite",
              "source_ts": "2026-03-24T17:00:00Z"
            },
            {
              "id": "USD_OIS_2Y",
              "instrument_type": "OIS",
              "quote_type": "OIS",
              "currency": "USD",
              "tenor": "2Y",
              "risk_factor_id": "RF:RATES:USD:OIS:2Y",
              "value": 0.0538,
              "source_name": "Composite",
              "source_ts": "2026-03-22T17:00:00Z"
            }
          ],
          "curves": [
            {
              "id": { "currency": "USD", "family": "OIS" },
              "purpose": "OISDiscount",
              "construction_family": "RatesOIS",
              "quote_ids": ["USD_OIS_2Y", "USD_OIS_30Y"],
              "day_count": "ACT360",
              "calendar": "US",
              "interpolation": "LogLinear"
            }
          ]
        }
    )json");

    auto snapshot = json.get<domain::MarketSnapshot>();

    EXPECT_EQ(snapshot.snapshot_id, "MKT_2026_03_24");
    EXPECT_EQ(snapshot.schema_version, 2);
    EXPECT_EQ(snapshot.base_currency, domain::Currency::USD);
    ASSERT_EQ(snapshot.quotes.size(), 2U);
    EXPECT_EQ(snapshot.quotes[0].instrument_type, domain::QuoteInstrumentType::FXSpot);
    EXPECT_EQ(snapshot.quotes[0].quote_type, domain::QuoteType::FXSpot);
    EXPECT_EQ(snapshot.quotes[0].underlier, "EURUSD");
    ASSERT_EQ(snapshot.curves.size(), 1U);
    EXPECT_EQ(snapshot.curves[0].purpose, domain::CurvePurpose::OISDiscount);
    EXPECT_EQ(snapshot.curves[0].construction_family, "RatesOIS");

    const auto diagnostics = domain::validate_market_snapshot(snapshot);
    ASSERT_EQ(diagnostics.size(), 2U);
    EXPECT_EQ(diagnostics[0].severity, "ERROR");
    EXPECT_EQ(diagnostics[0].code, "MISSING_CURVE_QUOTE");
    EXPECT_EQ(diagnostics[0].quote_id, "USD_OIS_30Y");
    EXPECT_EQ(diagnostics[1].severity, "WARNING");
    EXPECT_EQ(diagnostics[1].code, "STALE_QUOTE");
    EXPECT_EQ(diagnostics[1].quote_id, "USD_OIS_2Y");
}

TEST(MarketTest, MarketSnapshotGateRejectsMissingCurveQuote) {
    domain::MarketSnapshot snapshot;
    snapshot.snapshot_id = "MISSING_CURVE_QUOTE";
    snapshot.valuation_date = "2026-03-24";
    snapshot.default_stale_after_days = -1;

    domain::MarketQuote quote;
    quote.id = "USD_OIS_1Y";
    quote.instrument_type = domain::QuoteInstrumentType::OIS;
    quote.currency = domain::Currency::USD;
    quote.tenor = "1Y";
    quote.value = 0.05;
    quote.source_name = "unit-test";
    quote.source_ts = "2026-03-24T17:00:00Z";
    snapshot.quotes.push_back(quote);

    domain::CurveSpec curve;
    curve.id = {domain::Currency::USD, "OIS"};
    curve.purpose = domain::CurvePurpose::OISDiscount;
    curve.day_count = domain::DayCount::ACT360;
    curve.quote_ids = {"USD_OIS_1Y", "USD_OIS_2Y"};
    snapshot.curves.push_back(curve);

    const auto blocking = domain::blocking_market_snapshot_diagnostics(snapshot);
    ASSERT_EQ(blocking.size(), 1U);
    EXPECT_EQ(blocking[0].code, "MISSING_CURVE_QUOTE");
    EXPECT_THROW({
        qrp::market::MarketSnapshot runtime_snapshot(snapshot);
    }, std::runtime_error);
}

TEST(MarketTest, MarketSnapshotGateRejectsStaleQuote) {
    domain::MarketSnapshot snapshot;
    snapshot.snapshot_id = "STALE_MARKET";
    snapshot.valuation_date = "2026-03-24";
    snapshot.default_stale_after_days = 1;

    domain::MarketQuote quote;
    quote.id = "USD_OIS_1Y";
    quote.instrument_type = domain::QuoteInstrumentType::OIS;
    quote.currency = domain::Currency::USD;
    quote.tenor = "1Y";
    quote.value = 0.05;
    quote.source_name = "unit-test";
    quote.source_ts = "2026-03-20T17:00:00Z";
    snapshot.quotes.push_back(quote);

    domain::CurveSpec curve;
    curve.id = {domain::Currency::USD, "OIS"};
    curve.purpose = domain::CurvePurpose::OISDiscount;
    curve.day_count = domain::DayCount::ACT360;
    curve.quote_ids = {"USD_OIS_1Y"};
    snapshot.curves.push_back(curve);

    const auto diagnostics = domain::validate_market_snapshot(snapshot);
    ASSERT_EQ(diagnostics.size(), 1U);
    EXPECT_EQ(diagnostics[0].severity, "WARNING");
    EXPECT_EQ(diagnostics[0].code, "STALE_QUOTE");

    const auto blocking = domain::blocking_market_snapshot_diagnostics(snapshot);
    ASSERT_EQ(blocking.size(), 1U);
    EXPECT_EQ(blocking[0].code, "STALE_QUOTE");
    EXPECT_THROW({
        qrp::market::MarketSnapshot runtime_snapshot(snapshot);
    }, std::runtime_error);
}

TEST(MarketTest, MarketSnapshotGateAllowsNonBlockingProvenanceWarnings) {
    domain::MarketSnapshot snapshot;
    snapshot.valuation_date = "2026-03-24";
    snapshot.default_stale_after_days = -1;

    domain::MarketQuote quote;
    quote.id = "USD_OIS_1Y";
    quote.instrument_type = domain::QuoteInstrumentType::OIS;
    quote.currency = domain::Currency::USD;
    quote.tenor = "1Y";
    quote.value = 0.05;
    snapshot.quotes.push_back(quote);

    const auto diagnostics = domain::validate_market_snapshot(snapshot);
    ASSERT_EQ(diagnostics.size(), 2U);
    EXPECT_TRUE(domain::blocking_market_snapshot_diagnostics(snapshot).empty());
}

TEST(MarketTest, RatesMarketBuilderReportsBuiltCurvesAndRatesVolQuotes) {
    domain::MarketSnapshot snapshot;
    snapshot.valuation_date = "2026-03-24";
    snapshot.default_stale_after_days = -1;

    domain::MarketQuote ois_1y;
    ois_1y.id = "USD_OIS_1Y";
    ois_1y.instrument_type = domain::QuoteInstrumentType::OIS;
    ois_1y.currency = domain::Currency::USD;
    ois_1y.tenor = "1Y";
    ois_1y.value = 0.05;
    ois_1y.source_name = "unit-test";
    ois_1y.source_ts = "2026-03-24T17:00:00Z";
    snapshot.quotes.push_back(ois_1y);

    domain::MarketQuote ois_2y = ois_1y;
    ois_2y.id = "USD_OIS_2Y";
    ois_2y.tenor = "2Y";
    ois_2y.value = 0.051;
    snapshot.quotes.push_back(ois_2y);

    domain::MarketQuote cap_vol;
    cap_vol.id = "USD_CAP_VOL_1Y";
    cap_vol.instrument_type = domain::QuoteInstrumentType::CapFloorVol;
    cap_vol.quote_type = domain::QuoteType::Volatility;
    cap_vol.currency = domain::Currency::USD;
    cap_vol.tenor = "1Y";
    cap_vol.value = 0.20;
    cap_vol.source_name = "unit-test";
    cap_vol.source_ts = "2026-03-24T17:00:00Z";
    snapshot.quotes.push_back(cap_vol);

    domain::CurveSpec curve;
    curve.id = {domain::Currency::USD, "OIS"};
    curve.purpose = domain::CurvePurpose::OISDiscount;
    curve.day_count = domain::DayCount::ACT360;
    curve.quote_ids = {"USD_OIS_1Y", "USD_OIS_2Y"};
    snapshot.curves.push_back(curve);

    const auto result = RatesMarketBuilder::build(snapshot);
    ASSERT_NE(result.state, nullptr);
    ASSERT_EQ(result.curve_results.size(), 1U);
    EXPECT_TRUE(result.curve_results[0].built);
    EXPECT_EQ(result.curve_results[0].status_message, "Built");
    ASSERT_EQ(result.rates_vol_quote_ids.size(), 1U);
    EXPECT_EQ(result.rates_vol_quote_ids[0], "USD_CAP_VOL_1Y");
    EXPECT_EQ(result.state->get_quote("USD_CAP_VOL_1Y"), 0.20);
}

TEST(MarketTest, ParsesInterestRateFutureQuoteTaxonomy) {
    EXPECT_EQ(
        domain::parse_quote_instrument_type("InterestRateFuture"),
        domain::QuoteInstrumentType::InterestRateFuture);
    EXPECT_EQ(
        domain::parse_quote_type("InterestRateFuture"),
        domain::QuoteType::InterestRateFuture);
    EXPECT_TRUE(CurveBuilder::supports_rates_curve_quote(domain::QuoteInstrumentType::InterestRateFuture));
}
