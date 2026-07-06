// Verifies market-data parsing, validation gates, market-state reset, and rates-curve construction.

#include <qrp/analytics/pricing_context.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <qrp/market/market_state.hpp>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <iostream>
#include <map>
#include <memory>
#include <ql/indexes/ibor/sofr.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/termstructures/yield/oisratehelper.hpp>
#include <ql/termstructures/yield/piecewiseyieldcurve.hpp>
#include <stdexcept>
#include <string>
#include <vector>

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
        std::cout << "Curve built successfully. NPV at 1M: "
                  << curve->discount(valuation_date + QuantLib::Period(1, QuantLib::Months)) << std::endl;
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

TEST(MarketTest, TenorConversionAndRatesSupportPredicatesCoverEdges) {
    EXPECT_DOUBLE_EQ(CurveBuilder::tenor_to_years(""), 0.0);
    EXPECT_NEAR(CurveBuilder::tenor_to_years("7D"), 7.0 / 365.25, 1.0e-12);
    EXPECT_NEAR(CurveBuilder::tenor_to_years("2W"), 2.0 / 52.1786, 1.0e-12);
    EXPECT_DOUBLE_EQ(CurveBuilder::tenor_to_years("6M"), 0.5);
    EXPECT_DOUBLE_EQ(CurveBuilder::tenor_to_years("3Y"), 3.0);

    EXPECT_TRUE(CurveBuilder::supports_rates_curve_purpose(domain::CurvePurpose::Discount));
    EXPECT_TRUE(CurveBuilder::supports_rates_curve_purpose(domain::CurvePurpose::Forward));
    EXPECT_TRUE(CurveBuilder::supports_rates_curve_purpose(domain::CurvePurpose::Forward3M));
    EXPECT_TRUE(CurveBuilder::supports_rates_curve_purpose(domain::CurvePurpose::Forward6M));
    EXPECT_TRUE(CurveBuilder::supports_rates_curve_purpose(domain::CurvePurpose::OISDiscount));
    EXPECT_FALSE(CurveBuilder::supports_rates_curve_purpose(domain::CurvePurpose::CreditSpread));

    EXPECT_TRUE(CurveBuilder::supports_rates_curve_quote(domain::QuoteInstrumentType::Deposit));
    EXPECT_TRUE(CurveBuilder::supports_rates_curve_quote(domain::QuoteInstrumentType::FRA));
    EXPECT_TRUE(CurveBuilder::supports_rates_curve_quote(domain::QuoteInstrumentType::InterestRateFuture));
    EXPECT_TRUE(CurveBuilder::supports_rates_curve_quote(domain::QuoteInstrumentType::IRS));
    EXPECT_TRUE(CurveBuilder::supports_rates_curve_quote(domain::QuoteInstrumentType::OIS));
    EXPECT_FALSE(CurveBuilder::supports_rates_curve_quote(domain::QuoteInstrumentType::FXSpot));

    EXPECT_TRUE(CurveBuilder::supports_rates_vol_quote(domain::QuoteInstrumentType::CapFloorVol));
    EXPECT_TRUE(CurveBuilder::supports_rates_vol_quote(domain::QuoteInstrumentType::SwaptionVol));
    EXPECT_FALSE(CurveBuilder::supports_rates_vol_quote(domain::QuoteInstrumentType::EquityVol));
}

TEST(MarketTest, CurveBuilderMapsDomainEnumsAndIndexFamilies) {
    EXPECT_FALSE(CurveBuilder::parse_day_count(domain::DayCount::ACT360).name().empty());
    EXPECT_FALSE(CurveBuilder::parse_day_count(domain::DayCount::ACT365).name().empty());
    EXPECT_FALSE(CurveBuilder::parse_day_count(domain::DayCount::ACT365F).name().empty());
    EXPECT_FALSE(CurveBuilder::parse_day_count(domain::DayCount::ACTACT).name().empty());
    EXPECT_FALSE(CurveBuilder::parse_day_count(domain::DayCount::ACTACT_AFB).name().empty());
    EXPECT_FALSE(CurveBuilder::parse_day_count(domain::DayCount::ACTACT_EURO).name().empty());
    EXPECT_FALSE(CurveBuilder::parse_day_count(domain::DayCount::ACTACT_ISDA).name().empty());
    EXPECT_FALSE(CurveBuilder::parse_day_count(domain::DayCount::ACTACT_ISMA).name().empty());
    EXPECT_FALSE(CurveBuilder::parse_day_count(domain::DayCount::Thirty360).name().empty());
    EXPECT_FALSE(CurveBuilder::parse_day_count(domain::DayCount::UNKNOWN).name().empty());

    EXPECT_FALSE(CurveBuilder::parse_calendar(domain::BusinessCalendar::CHF).name().empty());
    EXPECT_FALSE(CurveBuilder::parse_calendar(domain::BusinessCalendar::JP).name().empty());
    EXPECT_FALSE(CurveBuilder::parse_calendar(domain::BusinessCalendar::Target).name().empty());
    EXPECT_FALSE(CurveBuilder::parse_calendar(domain::BusinessCalendar::UK).name().empty());
    EXPECT_FALSE(CurveBuilder::parse_calendar(domain::BusinessCalendar::US).name().empty());
    EXPECT_FALSE(CurveBuilder::parse_calendar(domain::BusinessCalendar::WeekendsOnly).name().empty());
    EXPECT_FALSE(CurveBuilder::parse_calendar(domain::BusinessCalendar::UNKNOWN).name().empty());

    EXPECT_EQ(CurveBuilder::parse_business_day_convention(domain::BusinessDayConvention::Following),
              QuantLib::Following);
    EXPECT_EQ(CurveBuilder::parse_business_day_convention(domain::BusinessDayConvention::HalfMonthModifiedFollowing),
              QuantLib::HalfMonthModifiedFollowing);
    EXPECT_EQ(CurveBuilder::parse_business_day_convention(domain::BusinessDayConvention::ModifiedFollowing),
              QuantLib::ModifiedFollowing);
    EXPECT_EQ(CurveBuilder::parse_business_day_convention(domain::BusinessDayConvention::ModifiedPreceding),
              QuantLib::ModifiedPreceding);
    EXPECT_EQ(CurveBuilder::parse_business_day_convention(domain::BusinessDayConvention::Nearest), QuantLib::Nearest);
    EXPECT_EQ(CurveBuilder::parse_business_day_convention(domain::BusinessDayConvention::Preceding),
              QuantLib::Preceding);
    EXPECT_EQ(CurveBuilder::parse_business_day_convention(domain::BusinessDayConvention::Unadjusted),
              QuantLib::Unadjusted);
    EXPECT_EQ(CurveBuilder::parse_business_day_convention(domain::BusinessDayConvention::UNKNOWN), QuantLib::Following);

    EXPECT_EQ(CurveBuilder::parse_frequency(domain::Frequency::Annual), QuantLib::Annual);
    EXPECT_EQ(CurveBuilder::parse_frequency(domain::Frequency::Bimonthly), QuantLib::Bimonthly);
    EXPECT_EQ(CurveBuilder::parse_frequency(domain::Frequency::Biweekly), QuantLib::Biweekly);
    EXPECT_EQ(CurveBuilder::parse_frequency(domain::Frequency::Daily), QuantLib::Daily);
    EXPECT_EQ(CurveBuilder::parse_frequency(domain::Frequency::EveryFourthMonth), QuantLib::EveryFourthMonth);
    EXPECT_EQ(CurveBuilder::parse_frequency(domain::Frequency::EveryFourthWeek), QuantLib::EveryFourthWeek);
    EXPECT_EQ(CurveBuilder::parse_frequency(domain::Frequency::Monthly), QuantLib::Monthly);
    EXPECT_EQ(CurveBuilder::parse_frequency(domain::Frequency::Once), QuantLib::Once);
    EXPECT_EQ(CurveBuilder::parse_frequency(domain::Frequency::OtherFrequency), QuantLib::OtherFrequency);
    EXPECT_EQ(CurveBuilder::parse_frequency(domain::Frequency::Quarterly), QuantLib::Quarterly);
    EXPECT_EQ(CurveBuilder::parse_frequency(domain::Frequency::Semiannual), QuantLib::Semiannual);
    EXPECT_EQ(CurveBuilder::parse_frequency(domain::Frequency::Weekly), QuantLib::Weekly);
    EXPECT_EQ(CurveBuilder::parse_frequency(domain::Frequency::UNKNOWN), QuantLib::Annual);

    EXPECT_EQ(CurveBuilder::parse_date_generation(domain::DateGeneration::Backward),
              QuantLib::DateGeneration::Backward);
    EXPECT_EQ(CurveBuilder::parse_date_generation(domain::DateGeneration::CDS), QuantLib::DateGeneration::CDS);
    EXPECT_EQ(CurveBuilder::parse_date_generation(domain::DateGeneration::CDS2015), QuantLib::DateGeneration::CDS2015);
    EXPECT_EQ(CurveBuilder::parse_date_generation(domain::DateGeneration::Forward), QuantLib::DateGeneration::Forward);
    EXPECT_EQ(CurveBuilder::parse_date_generation(domain::DateGeneration::OldCDS), QuantLib::DateGeneration::OldCDS);
    EXPECT_EQ(CurveBuilder::parse_date_generation(domain::DateGeneration::ThirdWednesday),
              QuantLib::DateGeneration::ThirdWednesday);
    EXPECT_EQ(CurveBuilder::parse_date_generation(domain::DateGeneration::Twentieth),
              QuantLib::DateGeneration::Twentieth);
    EXPECT_EQ(CurveBuilder::parse_date_generation(domain::DateGeneration::TwentiethIMM),
              QuantLib::DateGeneration::TwentiethIMM);
    EXPECT_EQ(CurveBuilder::parse_date_generation(domain::DateGeneration::Zero), QuantLib::DateGeneration::Zero);
    EXPECT_EQ(CurveBuilder::parse_date_generation(domain::DateGeneration::UNKNOWN), QuantLib::DateGeneration::Forward);

    const QuantLib::Handle<QuantLib::YieldTermStructure> empty_curve;
    EXPECT_TRUE(CurveBuilder::create_overnight_index(domain::Currency::CHF, empty_curve));
    EXPECT_TRUE(CurveBuilder::create_overnight_index(domain::Currency::EUR, empty_curve));
    EXPECT_TRUE(CurveBuilder::create_overnight_index(domain::Currency::GBP, empty_curve));
    EXPECT_TRUE(CurveBuilder::create_overnight_index(domain::Currency::JPY, empty_curve));
    EXPECT_TRUE(CurveBuilder::create_overnight_index(domain::Currency::USD, empty_curve));
    EXPECT_TRUE(CurveBuilder::create_overnight_index(domain::Currency::UNKNOWN, empty_curve));

    const auto tenor = QuantLib::Period(3, QuantLib::Months);
    EXPECT_TRUE(CurveBuilder::create_ibor_index(domain::Currency::CHF, tenor, empty_curve));
    EXPECT_TRUE(CurveBuilder::create_ibor_index(domain::Currency::EUR, tenor, empty_curve));
    EXPECT_TRUE(CurveBuilder::create_ibor_index(domain::Currency::GBP, tenor, empty_curve));
    EXPECT_TRUE(CurveBuilder::create_ibor_index(domain::Currency::JPY, tenor, empty_curve));
    EXPECT_TRUE(CurveBuilder::create_ibor_index(domain::Currency::USD, tenor, empty_curve));
    EXPECT_TRUE(CurveBuilder::create_ibor_index(domain::Currency::UNKNOWN, tenor, empty_curve));
}

TEST(MarketTest, ParsesMarketDataJsonTaxonomyHelpers) {
    EXPECT_EQ(nlohmann::json("OISDiscount").get<domain::CurvePurpose>(), domain::CurvePurpose::OISDiscount);
    EXPECT_EQ(nlohmann::json("OIS").get<domain::QuoteInstrumentType>(), domain::QuoteInstrumentType::OIS);
    EXPECT_EQ(nlohmann::json("Swap").get<domain::QuoteType>(), domain::QuoteType::Swap);

    EXPECT_EQ(nlohmann::json("CubicSpline").get<domain::InterpolationType>(), domain::InterpolationType::CubicSpline);
    EXPECT_EQ(nlohmann::json("Linear").get<domain::InterpolationType>(), domain::InterpolationType::Linear);
    EXPECT_EQ(nlohmann::json("LogLinear").get<domain::InterpolationType>(), domain::InterpolationType::LogLinear);
    EXPECT_EQ(nlohmann::json("unsupported").get<domain::InterpolationType>(), domain::InterpolationType::UNKNOWN);

    const auto curve_id = nlohmann::json{{"currency", "USD"}, {"family", "OIS"}}.get<domain::CurveId>();
    EXPECT_EQ(curve_id.currency, domain::Currency::USD);
    EXPECT_EQ(curve_id.family, "OIS");
}

TEST(MarketTest, ParsesAllCanonicalMarketDataTaxonomyValues) {
    const std::vector<std::pair<std::string, domain::CurvePurpose>> curve_purposes = {
        {"CommodityForward", domain::CurvePurpose::CommodityForward},
        {"CommodityVolatility", domain::CurvePurpose::CommodityVolatility},
        {"Credit", domain::CurvePurpose::Credit},
        {"CreditSpread", domain::CurvePurpose::CreditSpread},
        {"Discount", domain::CurvePurpose::Discount},
        {"EquityBorrow", domain::CurvePurpose::EquityBorrow},
        {"EquityDividend", domain::CurvePurpose::EquityDividend},
        {"EquityVolatility", domain::CurvePurpose::EquityVolatility},
        {"Forward", domain::CurvePurpose::Forward},
        {"Forward3M", domain::CurvePurpose::Forward3M},
        {"Forward6M", domain::CurvePurpose::Forward6M},
        {"FXForward", domain::CurvePurpose::FXForward},
        {"FXVolatility", domain::CurvePurpose::FXVolatility},
        {"Hazard", domain::CurvePurpose::Hazard},
        {"Inflation", domain::CurvePurpose::Inflation},
        {"OISDiscount", domain::CurvePurpose::OISDiscount},
        {"Recovery", domain::CurvePurpose::Recovery},
        {"Volatility", domain::CurvePurpose::Volatility},
        {"unsupported", domain::CurvePurpose::UNKNOWN},
    };
    for (const auto& [label, expected] : curve_purposes) {
        EXPECT_EQ(domain::parse_curve_purpose(label), expected) << label;
    }

    const std::vector<std::pair<std::string, domain::QuoteInstrumentType>> instrument_types = {
        {"Bond", domain::QuoteInstrumentType::Bond},
        {"BondPrice", domain::QuoteInstrumentType::BondPrice},
        {"BondSpread", domain::QuoteInstrumentType::BondSpread},
        {"BorrowRate", domain::QuoteInstrumentType::BorrowRate},
        {"CapFloorVol", domain::QuoteInstrumentType::CapFloorVol},
        {"CDS", domain::QuoteInstrumentType::CDS},
        {"CommodityForward", domain::QuoteInstrumentType::CommodityForward},
        {"CommodityFuture", domain::QuoteInstrumentType::CommodityFuture},
        {"CommoditySpot", domain::QuoteInstrumentType::CommoditySpot},
        {"CommodityVol", domain::QuoteInstrumentType::CommodityVol},
        {"ConvenienceYield", domain::QuoteInstrumentType::ConvenienceYield},
        {"CreditIndex", domain::QuoteInstrumentType::CreditIndex},
        {"CreditSpread", domain::QuoteInstrumentType::CreditSpread},
        {"Deposit", domain::QuoteInstrumentType::Deposit},
        {"DividendYield", domain::QuoteInstrumentType::DividendYield},
        {"EquitySpot", domain::QuoteInstrumentType::EquitySpot},
        {"EquityVol", domain::QuoteInstrumentType::EquityVol},
        {"FRA", domain::QuoteInstrumentType::FRA},
        {"Future", domain::QuoteInstrumentType::Future},
        {"FXForward", domain::QuoteInstrumentType::FXForward},
        {"FXForwardPoint", domain::QuoteInstrumentType::FXForwardPoint},
        {"FXSpot", domain::QuoteInstrumentType::FXSpot},
        {"FXVol", domain::QuoteInstrumentType::FXVol},
        {"HazardRate", domain::QuoteInstrumentType::HazardRate},
        {"InterestRateFuture", domain::QuoteInstrumentType::InterestRateFuture},
        {"IRS", domain::QuoteInstrumentType::IRS},
        {"OIS", domain::QuoteInstrumentType::OIS},
        {"RecoveryRate", domain::QuoteInstrumentType::RecoveryRate},
        {"SwaptionVol", domain::QuoteInstrumentType::SwaptionVol},
        {"unsupported", domain::QuoteInstrumentType::UNKNOWN},
    };
    for (const auto& [label, expected] : instrument_types) {
        EXPECT_EQ(domain::parse_quote_instrument_type(label), expected) << label;
    }

    const std::vector<std::pair<std::string, domain::QuoteType>> quote_types = {
        {"BasisSwap", domain::QuoteType::BasisSwap},
        {"BondYield", domain::QuoteType::BondYield},
        {"BorrowRate", domain::QuoteType::BorrowRate},
        {"CommodityForward", domain::QuoteType::CommodityForward},
        {"CreditSpread", domain::QuoteType::CreditSpread},
        {"Deposit", domain::QuoteType::Deposit},
        {"DividendYield", domain::QuoteType::DividendYield},
        {"EquitySpot", domain::QuoteType::EquitySpot},
        {"FRA", domain::QuoteType::FRA},
        {"Future", domain::QuoteType::Future},
        {"FXForward", domain::QuoteType::FXForward},
        {"FXForwardPoint", domain::QuoteType::FXForwardPoint},
        {"FXSpot", domain::QuoteType::FXSpot},
        {"HazardRate", domain::QuoteType::HazardRate},
        {"InterestRateFuture", domain::QuoteType::InterestRateFuture},
        {"IRS", domain::QuoteType::IRS},
        {"OIS", domain::QuoteType::OIS},
        {"Price", domain::QuoteType::Price},
        {"RecoveryRate", domain::QuoteType::RecoveryRate},
        {"Swap", domain::QuoteType::Swap},
        {"Volatility", domain::QuoteType::Volatility},
        {"unsupported", domain::QuoteType::UNKNOWN},
    };
    for (const auto& [label, expected] : quote_types) {
        EXPECT_EQ(domain::parse_quote_type(label), expected) << label;
    }
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

TEST(MarketTest, MarketStateExposesQuoteHandlesCurvesAndFixings) {
    MarketState state(QuantLib::Date(24, QuantLib::March, 2024));

    auto quote = QuantLib::ext::make_shared<QuantLib::SimpleQuote>(0.031);
    state.add_quote_handle("USD_OIS_HANDLE", quote);
    ASSERT_NE(state.get_quote_handle("USD_OIS_HANDLE"), nullptr);
    EXPECT_DOUBLE_EQ(state.get_quote("USD_OIS_HANDLE"), 0.031);

    state.add_quote("USD_OIS_HANDLE", 0.032);
    EXPECT_DOUBLE_EQ(quote->value(), 0.032);
    EXPECT_EQ(state.get_quote_handle("MISSING_QUOTE"), nullptr);
    EXPECT_EQ(state.get_curve({domain::Currency::USD, "MISSING"}), nullptr);

    const QuantLib::Date fixing_date(22, QuantLib::March, 2024);
    state.add_fixing("SOFR", fixing_date, 0.052);
    EXPECT_DOUBLE_EQ(state.get_fixing("SOFR", fixing_date), 0.052);
    EXPECT_DOUBLE_EQ(state.get_fixing("SOFR", QuantLib::Date(21, QuantLib::March, 2024)), 0.0);
    EXPECT_DOUBLE_EQ(state.get_fixing("MISSING", fixing_date), 0.0);
    EXPECT_EQ(state.fixings().size(), 1U);
}

TEST(MarketTest, ParsesMarketSnapshotMetadata) {
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

TEST(MarketTest, ParsesMarketConventionsAndDiagnostics) {
    const std::vector<std::pair<std::string, domain::DayCount>> day_counts = {
        {"ACT360", domain::DayCount::ACT360},
        {"ACT365", domain::DayCount::ACT365},
        {"ACT365F", domain::DayCount::ACT365F},
        {"ACTACT", domain::DayCount::ACTACT},
        {"ACTACT_AFB", domain::DayCount::ACTACT_AFB},
        {"ACTACT_EURO", domain::DayCount::ACTACT_EURO},
        {"ACTACT_ISDA", domain::DayCount::ACTACT_ISDA},
        {"ACTACT_ISMA", domain::DayCount::ACTACT_ISMA},
        {"Thirty360", domain::DayCount::Thirty360},
        {"Unsupported", domain::DayCount::UNKNOWN}};
    for (const auto& [label, expected] : day_counts) {
        EXPECT_EQ(nlohmann::json(label).get<domain::DayCount>(), expected);
    }

    const std::vector<std::pair<std::string, domain::BusinessCalendar>> calendars = {
        {"CHF", domain::BusinessCalendar::CHF},
        {"CHE", domain::BusinessCalendar::CHF},
        {"JP", domain::BusinessCalendar::JP},
        {"JPN", domain::BusinessCalendar::JP},
        {"Target", domain::BusinessCalendar::Target},
        {"UK", domain::BusinessCalendar::UK},
        {"GBR", domain::BusinessCalendar::UK},
        {"US", domain::BusinessCalendar::US},
        {"USA", domain::BusinessCalendar::US},
        {"WeekendsOnly", domain::BusinessCalendar::WeekendsOnly},
        {"Unsupported", domain::BusinessCalendar::UNKNOWN}};
    for (const auto& [label, expected] : calendars) {
        EXPECT_EQ(nlohmann::json(label).get<domain::BusinessCalendar>(), expected);
    }

    const std::vector<std::pair<std::string, domain::BusinessDayConvention>> business_day_conventions = {
        {"Following", domain::BusinessDayConvention::Following},
        {"HalfMonthModifiedFollowing", domain::BusinessDayConvention::HalfMonthModifiedFollowing},
        {"ModifiedFollowing", domain::BusinessDayConvention::ModifiedFollowing},
        {"ModifiedPreceding", domain::BusinessDayConvention::ModifiedPreceding},
        {"Nearest", domain::BusinessDayConvention::Nearest},
        {"Preceding", domain::BusinessDayConvention::Preceding},
        {"Unadjusted", domain::BusinessDayConvention::Unadjusted},
        {"Unsupported", domain::BusinessDayConvention::UNKNOWN}};
    for (const auto& [label, expected] : business_day_conventions) {
        EXPECT_EQ(nlohmann::json(label).get<domain::BusinessDayConvention>(), expected);
    }

    const std::vector<std::pair<std::string, domain::Frequency>> frequencies = {
        {"Annual", domain::Frequency::Annual},
        {"Bimonthly", domain::Frequency::Bimonthly},
        {"Biweekly", domain::Frequency::Biweekly},
        {"Daily", domain::Frequency::Daily},
        {"EveryFourthMonth", domain::Frequency::EveryFourthMonth},
        {"EveryFourthWeek", domain::Frequency::EveryFourthWeek},
        {"Monthly", domain::Frequency::Monthly},
        {"Once", domain::Frequency::Once},
        {"OtherFrequency", domain::Frequency::OtherFrequency},
        {"Quarterly", domain::Frequency::Quarterly},
        {"Semiannual", domain::Frequency::Semiannual},
        {"Weekly", domain::Frequency::Weekly},
        {"Unsupported", domain::Frequency::UNKNOWN}};
    for (const auto& [label, expected] : frequencies) {
        EXPECT_EQ(nlohmann::json(label).get<domain::Frequency>(), expected);
    }

    const std::vector<std::pair<std::string, domain::DateGeneration>> date_generations = {
        {"Backward", domain::DateGeneration::Backward},
        {"CDS", domain::DateGeneration::CDS},
        {"CDS2015", domain::DateGeneration::CDS2015},
        {"Forward", domain::DateGeneration::Forward},
        {"OldCDS", domain::DateGeneration::OldCDS},
        {"ThirdWednesday", domain::DateGeneration::ThirdWednesday},
        {"Twentieth", domain::DateGeneration::Twentieth},
        {"TwentiethIMM", domain::DateGeneration::TwentiethIMM},
        {"Zero", domain::DateGeneration::Zero},
        {"Unsupported", domain::DateGeneration::UNKNOWN}};
    for (const auto& [label, expected] : date_generations) {
        EXPECT_EQ(nlohmann::json(label).get<domain::DateGeneration>(), expected);
    }

    EXPECT_EQ(nlohmann::json("CubicSpline").get<domain::InterpolationType>(), domain::InterpolationType::CubicSpline);
    EXPECT_EQ(nlohmann::json("Linear").get<domain::InterpolationType>(), domain::InterpolationType::Linear);
    EXPECT_EQ(nlohmann::json("LogLinear").get<domain::InterpolationType>(), domain::InterpolationType::LogLinear);
    EXPECT_EQ(nlohmann::json("Unsupported").get<domain::InterpolationType>(), domain::InterpolationType::UNKNOWN);

    EXPECT_EQ(domain::parse_curve_purpose("CreditSpread"), domain::CurvePurpose::CreditSpread);
    EXPECT_EQ(domain::parse_curve_purpose("CommodityForward"), domain::CurvePurpose::CommodityForward);
    EXPECT_EQ(domain::parse_curve_purpose("Unsupported"), domain::CurvePurpose::UNKNOWN);

    const auto diagnostic_json = nlohmann::json::parse(R"json({
        "severity": "ERROR",
        "code": "MISSING_CURVE_QUOTE",
        "message": "Curve quote is absent",
        "quote_id": "USD_OIS_30Y",
        "curve_id": "USD:OIS"
    })json");
    const auto diagnostic = diagnostic_json.get<domain::MarketDataDiagnostic>();
    EXPECT_EQ(domain::format_market_data_diagnostic(diagnostic),
              "ERROR:MISSING_CURVE_QUOTE quote=USD_OIS_30Y curve=USD:OIS - Curve quote is absent");
    EXPECT_TRUE(domain::is_blocking_market_data_diagnostic(diagnostic));
}

TEST(MarketTest, CurveBuilderMapsSupportedConventionsAndIndexFamilies) {
    const std::vector<domain::DayCount> day_counts = {domain::DayCount::ACT360,
                                                      domain::DayCount::ACT365,
                                                      domain::DayCount::ACT365F,
                                                      domain::DayCount::ACTACT,
                                                      domain::DayCount::ACTACT_AFB,
                                                      domain::DayCount::ACTACT_EURO,
                                                      domain::DayCount::ACTACT_ISDA,
                                                      domain::DayCount::ACTACT_ISMA,
                                                      domain::DayCount::Thirty360,
                                                      domain::DayCount::UNKNOWN};
    for (const auto day_count : day_counts) {
        EXPECT_FALSE(CurveBuilder::parse_day_count(day_count).name().empty());
    }

    const std::vector<domain::BusinessCalendar> calendars = {domain::BusinessCalendar::CHF,
                                                             domain::BusinessCalendar::JP,
                                                             domain::BusinessCalendar::Target,
                                                             domain::BusinessCalendar::UK,
                                                             domain::BusinessCalendar::US,
                                                             domain::BusinessCalendar::WeekendsOnly,
                                                             domain::BusinessCalendar::UNKNOWN};
    for (const auto calendar : calendars) {
        EXPECT_FALSE(CurveBuilder::parse_calendar(calendar).name().empty());
    }

    const std::vector<std::pair<domain::BusinessDayConvention, QuantLib::BusinessDayConvention>>
        business_day_conventions = {
            {domain::BusinessDayConvention::Following, QuantLib::Following},
            {domain::BusinessDayConvention::HalfMonthModifiedFollowing, QuantLib::HalfMonthModifiedFollowing},
            {domain::BusinessDayConvention::ModifiedFollowing, QuantLib::ModifiedFollowing},
            {domain::BusinessDayConvention::ModifiedPreceding, QuantLib::ModifiedPreceding},
            {domain::BusinessDayConvention::Nearest, QuantLib::Nearest},
            {domain::BusinessDayConvention::Preceding, QuantLib::Preceding},
            {domain::BusinessDayConvention::Unadjusted, QuantLib::Unadjusted},
            {domain::BusinessDayConvention::UNKNOWN, QuantLib::Following}};
    for (const auto& [business_day_convention, expected] : business_day_conventions) {
        EXPECT_EQ(CurveBuilder::parse_business_day_convention(business_day_convention), expected);
    }

    const std::vector<std::pair<domain::Frequency, QuantLib::Frequency>> frequencies = {
        {domain::Frequency::Annual, QuantLib::Annual},
        {domain::Frequency::Bimonthly, QuantLib::Bimonthly},
        {domain::Frequency::Biweekly, QuantLib::Biweekly},
        {domain::Frequency::Daily, QuantLib::Daily},
        {domain::Frequency::EveryFourthMonth, QuantLib::EveryFourthMonth},
        {domain::Frequency::EveryFourthWeek, QuantLib::EveryFourthWeek},
        {domain::Frequency::Monthly, QuantLib::Monthly},
        {domain::Frequency::Once, QuantLib::Once},
        {domain::Frequency::OtherFrequency, QuantLib::OtherFrequency},
        {domain::Frequency::Quarterly, QuantLib::Quarterly},
        {domain::Frequency::Semiannual, QuantLib::Semiannual},
        {domain::Frequency::Weekly, QuantLib::Weekly},
        {domain::Frequency::UNKNOWN, QuantLib::Annual}};
    for (const auto& [frequency, expected] : frequencies) {
        EXPECT_EQ(CurveBuilder::parse_frequency(frequency), expected);
    }

    const std::vector<std::pair<domain::DateGeneration, QuantLib::DateGeneration::Rule>> date_generation_rules = {
        {domain::DateGeneration::Backward, QuantLib::DateGeneration::Backward},
        {domain::DateGeneration::CDS, QuantLib::DateGeneration::CDS},
        {domain::DateGeneration::CDS2015, QuantLib::DateGeneration::CDS2015},
        {domain::DateGeneration::Forward, QuantLib::DateGeneration::Forward},
        {domain::DateGeneration::OldCDS, QuantLib::DateGeneration::OldCDS},
        {domain::DateGeneration::ThirdWednesday, QuantLib::DateGeneration::ThirdWednesday},
        {domain::DateGeneration::Twentieth, QuantLib::DateGeneration::Twentieth},
        {domain::DateGeneration::TwentiethIMM, QuantLib::DateGeneration::TwentiethIMM},
        {domain::DateGeneration::Zero, QuantLib::DateGeneration::Zero},
        {domain::DateGeneration::UNKNOWN, QuantLib::DateGeneration::Forward}};
    for (const auto& [rule, expected] : date_generation_rules) {
        EXPECT_EQ(CurveBuilder::parse_date_generation(rule), expected);
    }

    const QuantLib::Handle<QuantLib::YieldTermStructure> empty_curve;
    const QuantLib::Period three_months(3, QuantLib::Months);
    for (const auto currency : {domain::Currency::CHF,
                                domain::Currency::EUR,
                                domain::Currency::GBP,
                                domain::Currency::JPY,
                                domain::Currency::USD,
                                domain::Currency::UNKNOWN}) {
        EXPECT_FALSE(CurveBuilder::create_overnight_index(currency, empty_curve)->name().empty());
        EXPECT_FALSE(CurveBuilder::create_ibor_index(currency, three_months, empty_curve)->name().empty());
    }

    EXPECT_TRUE(CurveBuilder::supports_rates_curve_purpose(domain::CurvePurpose::Discount));
    EXPECT_TRUE(CurveBuilder::supports_rates_curve_purpose(domain::CurvePurpose::Forward));
    EXPECT_TRUE(CurveBuilder::supports_rates_curve_purpose(domain::CurvePurpose::Forward3M));
    EXPECT_TRUE(CurveBuilder::supports_rates_curve_purpose(domain::CurvePurpose::Forward6M));
    EXPECT_TRUE(CurveBuilder::supports_rates_curve_purpose(domain::CurvePurpose::OISDiscount));
    EXPECT_FALSE(CurveBuilder::supports_rates_curve_purpose(domain::CurvePurpose::FXForward));

    EXPECT_TRUE(CurveBuilder::supports_rates_curve_quote(domain::QuoteInstrumentType::Deposit));
    EXPECT_TRUE(CurveBuilder::supports_rates_curve_quote(domain::QuoteInstrumentType::FRA));
    EXPECT_TRUE(CurveBuilder::supports_rates_curve_quote(domain::QuoteInstrumentType::InterestRateFuture));
    EXPECT_TRUE(CurveBuilder::supports_rates_curve_quote(domain::QuoteInstrumentType::IRS));
    EXPECT_TRUE(CurveBuilder::supports_rates_curve_quote(domain::QuoteInstrumentType::OIS));
    EXPECT_FALSE(CurveBuilder::supports_rates_curve_quote(domain::QuoteInstrumentType::FXSpot));

    EXPECT_TRUE(CurveBuilder::supports_rates_vol_quote(domain::QuoteInstrumentType::CapFloorVol));
    EXPECT_TRUE(CurveBuilder::supports_rates_vol_quote(domain::QuoteInstrumentType::SwaptionVol));
    EXPECT_FALSE(CurveBuilder::supports_rates_vol_quote(domain::QuoteInstrumentType::OIS));
}

TEST(MarketTest, ParsesOptionalSnapshotFieldsAndValidationEdges) {
    const auto snapshot_json = nlohmann::json::parse(R"json({
        "valuation_date": "2026-03-24",
        "quotes": [
            {
                "id": "USD_OIS_1Y",
                "instrument_type": "OIS",
                "quote_type": "OIS",
                "currency": "USD",
                "tenor": "1Y",
                "value": 0.05,
                "source_name": "unit-test",
                "source_ts": "2026-03-24T17:00:00Z"
            }
        ],
        "curves": [
            {
                "id": { "currency": "USD", "family": "OIS" },
                "purpose": "OISDiscount",
                "quote_ids": ["USD_OIS_1Y"],
                "interpolation": "LogLinear"
            }
        ],
        "diagnostics": [
            {
                "severity": "INFO",
                "code": "SOURCE_INFO",
                "message": "Informational source diagnostic"
            }
        ],
        "fixings": {
            "SOFR": {
                "2026-03-23": 0.052
            }
        }
    })json");

    const auto snapshot = snapshot_json.get<domain::MarketSnapshot>();

    ASSERT_EQ(snapshot.fixings.size(), 1U);
    EXPECT_DOUBLE_EQ(snapshot.fixings.at("SOFR").at("2026-03-23"), 0.052);
    ASSERT_EQ(snapshot.diagnostics.size(), 1U);
    EXPECT_FALSE(domain::is_blocking_market_data_diagnostic(snapshot.diagnostics[0]));
    EXPECT_EQ(domain::market_data_detail::diagnostic_severity_rank("INFO"), 2);
    EXPECT_FALSE(domain::market_data_detail::is_digit_at("X", 0));
    EXPECT_FALSE(domain::market_data_detail::is_digit_at("1", 1));

    domain::MarketSnapshot bad_snapshot;
    bad_snapshot.valuation_date = "2026-03-24";
    bad_snapshot.default_stale_after_days = -1;

    domain::MarketQuote missing_id;
    missing_id.instrument_type = domain::QuoteInstrumentType::OIS;
    missing_id.currency = domain::Currency::USD;
    missing_id.tenor = "1Y";
    missing_id.value = 0.05;
    bad_snapshot.quotes.push_back(missing_id);

    domain::MarketQuote invalid_timestamp = missing_id;
    invalid_timestamp.id = "BAD_TS";
    invalid_timestamp.source_name = "unit-test";
    invalid_timestamp.source_ts = "2026/03/24";
    bad_snapshot.quotes.push_back(invalid_timestamp);

    domain::CurveSpec empty_quote_set;
    empty_quote_set.id = {domain::Currency::USD, "OIS"};
    empty_quote_set.purpose = domain::CurvePurpose::OISDiscount;
    bad_snapshot.curves.push_back(empty_quote_set);

    const auto diagnostics = domain::validate_market_snapshot(bad_snapshot);
    const auto has_code = [&](const std::string& code) {
        return std::any_of(diagnostics.begin(), diagnostics.end(), [&](const auto& diagnostic) {
            return diagnostic.code == code;
        });
    };
    EXPECT_TRUE(has_code("EMPTY_CURVE_QUOTE_SET"));
    EXPECT_TRUE(has_code("INVALID_QUOTE_TIMESTAMP"));
    EXPECT_TRUE(has_code("MISSING_QUOTE_ID"));
}

TEST(MarketTest, MarketSnapshotDiagnosticsSortAndDeduplicateSourceDiagnostics) {
    domain::MarketSnapshot snapshot;
    snapshot.snapshot_id = "DIAGNOSTIC_SORT";
    snapshot.valuation_date = "not-a-date";
    snapshot.default_stale_after_days = -1;

    domain::MarketQuote quote;
    quote.id = "Q1";
    quote.instrument_type = domain::QuoteInstrumentType::UNKNOWN;
    quote.currency = domain::Currency::UNKNOWN;
    quote.tenor = "1Y";
    quote.value = 0.05;
    snapshot.quotes.push_back(quote);
    snapshot.quotes.push_back(quote);

    domain::CurveSpec curve;
    curve.id = {domain::Currency::UNKNOWN, ""};
    curve.purpose = domain::CurvePurpose::UNKNOWN;
    curve.quote_ids = {"Q1", "Q1", "Q_MISSING"};
    snapshot.curves.push_back(curve);

    snapshot.diagnostics.push_back({"WARNING", "SOURCE_WARNING", "Source supplied warning", "Q1", ""});
    snapshot.diagnostics.push_back({"WARNING", "SOURCE_WARNING", "Source supplied warning", "Q1", ""});

    const auto diagnostics = domain::collect_market_snapshot_diagnostics(snapshot);
    ASSERT_FALSE(diagnostics.empty());
    EXPECT_EQ(diagnostics.front().severity, "ERROR");
    EXPECT_NE(std::find_if(diagnostics.begin(),
                           diagnostics.end(),
                           [](const auto& diagnostic) { return diagnostic.code == "SOURCE_WARNING"; }),
              diagnostics.end());

    const auto source_warning_count = std::count_if(diagnostics.begin(), diagnostics.end(), [](const auto& diagnostic) {
        return diagnostic.code == "SOURCE_WARNING";
    });
    EXPECT_EQ(source_warning_count, 1);
    EXPECT_THROW(domain::throw_if_market_snapshot_not_ready(snapshot), std::runtime_error);
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
    EXPECT_THROW({ qrp::market::MarketSnapshot runtime_snapshot(snapshot); }, std::runtime_error);
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
    EXPECT_THROW({ qrp::market::MarketSnapshot runtime_snapshot(snapshot); }, std::runtime_error);
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

TEST(MarketTest, RatesMarketBuilderReportsSkippedAndFailedCurves) {
    domain::MarketSnapshot skipped_snapshot;
    skipped_snapshot.valuation_date = "2026-03-24";
    skipped_snapshot.default_stale_after_days = -1;

    domain::CurveSpec unsupported_curve;
    unsupported_curve.id = {domain::Currency::USD, "FX_FORWARD"};
    unsupported_curve.purpose = domain::CurvePurpose::FXForward;
    skipped_snapshot.curves.push_back(unsupported_curve);

    const auto skipped_result = RatesMarketBuilder::build(skipped_snapshot);
    ASSERT_EQ(skipped_result.curve_results.size(), 1U);
    EXPECT_FALSE(skipped_result.curve_results[0].built);
    EXPECT_EQ(skipped_result.curve_results[0].status_message,
              "Skipped: curve purpose is not supported by the rates builder");

    domain::MarketSnapshot no_helper_snapshot;
    no_helper_snapshot.valuation_date = "2026-03-24";
    no_helper_snapshot.default_stale_after_days = -1;

    domain::MarketQuote fx_quote;
    fx_quote.id = "EURUSD";
    fx_quote.instrument_type = domain::QuoteInstrumentType::FXSpot;
    fx_quote.currency = domain::Currency::USD;
    fx_quote.tenor = "SPOT";
    fx_quote.value = 1.10;
    fx_quote.source_name = "unit-test";
    fx_quote.source_ts = "2026-03-24T17:00:00Z";
    no_helper_snapshot.quotes.push_back(fx_quote);

    domain::CurveSpec rates_curve;
    rates_curve.id = {domain::Currency::USD, "OIS"};
    rates_curve.purpose = domain::CurvePurpose::OISDiscount;
    rates_curve.day_count = domain::DayCount::ACT360;
    rates_curve.quote_ids = {"EURUSD"};
    no_helper_snapshot.curves.push_back(rates_curve);

    EXPECT_THROW({ RatesMarketBuilder::build(no_helper_snapshot); }, std::runtime_error);

    domain::MarketSnapshot bad_tenor_snapshot = no_helper_snapshot;
    bad_tenor_snapshot.quotes[0].instrument_type = domain::QuoteInstrumentType::OIS;

    EXPECT_THROW({ RatesMarketBuilder::build(bad_tenor_snapshot); }, std::runtime_error);
}

TEST(MarketTest, BuildsInterestRateFutureCurveHelpersFromPriceAndRateQuotes) {
    domain::CurveSpec spec;
    spec.id = {domain::Currency::USD, "IBOR_3M"};
    spec.purpose = domain::CurvePurpose::Forward3M;
    spec.day_count = domain::DayCount::ACT360;
    spec.quote_ids = {"USD_FUT_PRICE", "USD_FUT_RATE"};

    std::map<std::string, domain::MarketQuote> quotes;

    domain::MarketQuote price_quote;
    price_quote.id = "USD_FUT_PRICE";
    price_quote.instrument_type = domain::QuoteInstrumentType::InterestRateFuture;
    price_quote.currency = domain::Currency::USD;
    price_quote.tenor = "3M";
    price_quote.expiry = "2026-06-17";
    price_quote.value = 94.50;
    quotes[price_quote.id] = price_quote;

    domain::MarketQuote rate_quote = price_quote;
    rate_quote.id = "USD_FUT_RATE";
    rate_quote.expiry = "";
    rate_quote.tenor = "6M";
    rate_quote.value = 0.055;
    quotes[rate_quote.id] = rate_quote;

    const auto valuation_date = CurveBuilder::parse_date("2026-03-24");
    auto curve = CurveBuilder::build_rate_curve(spec, quotes, valuation_date, nullptr);
    ASSERT_NE(curve, nullptr);
    EXPECT_GT(curve->discount(valuation_date + QuantLib::Period(6, QuantLib::Months)), 0.0);
}

TEST(MarketTest, ParsesInterestRateFutureQuoteTaxonomy) {
    EXPECT_EQ(domain::parse_quote_instrument_type("InterestRateFuture"),
              domain::QuoteInstrumentType::InterestRateFuture);
    EXPECT_EQ(domain::parse_quote_type("InterestRateFuture"), domain::QuoteType::InterestRateFuture);
    EXPECT_TRUE(CurveBuilder::supports_rates_curve_quote(domain::QuoteInstrumentType::InterestRateFuture));
}

TEST(MarketTest, ParsesScalarEnumsAndOptionalQuoteFields) {
    EXPECT_EQ(nlohmann::json("USD").get<domain::Currency>(), domain::Currency::USD);
    EXPECT_EQ(nlohmann::json("CreditSpread").get<domain::CurvePurpose>(), domain::CurvePurpose::CreditSpread);
    EXPECT_EQ(nlohmann::json("RecoveryRate").get<domain::QuoteInstrumentType>(),
              domain::QuoteInstrumentType::RecoveryRate);
    EXPECT_EQ(nlohmann::json("RecoveryRate").get<domain::QuoteType>(), domain::QuoteType::RecoveryRate);

    const auto quote_json = nlohmann::json::parse(R"json({
        "id": "ACME_CDS_5Y",
        "instrument_type": "CDS",
        "quote_type": "CreditSpread",
        "currency": "USD",
        "tenor": "5Y",
        "value": 0.0125,
        "risk_factor_id": "RF:CREDIT:ACME:SPREAD:5Y",
        "underlier": "ACME",
        "expiry": "1Y",
        "strike": "ATM",
        "instrument_family": "CDS",
        "index_family": "CDX",
        "day_count": "ACT360",
        "calendar": "US",
        "bdc": "ModifiedFollowing",
        "settlement_days": 1,
        "market_ts": "2026-03-24",
        "recorded_ts": "2026-03-24T18:00:00Z",
        "source_name": "Composite",
        "source_ts": "2026-03-24T17:00:00Z",
        "stale_after_days": 2
    })json");

    const auto quote = quote_json.get<domain::MarketQuote>();
    EXPECT_EQ(quote.id, "ACME_CDS_5Y");
    EXPECT_EQ(quote.instrument_type, domain::QuoteInstrumentType::CDS);
    EXPECT_EQ(quote.quote_type, domain::QuoteType::CreditSpread);
    EXPECT_EQ(quote.risk_factor_id, "RF:CREDIT:ACME:SPREAD:5Y");
    EXPECT_EQ(quote.underlier, "ACME");
    EXPECT_EQ(quote.expiry, "1Y");
    EXPECT_EQ(quote.strike, "ATM");
    EXPECT_EQ(quote.instrument_family, "CDS");
    EXPECT_EQ(quote.index_family, "CDX");
    EXPECT_EQ(quote.day_count, domain::DayCount::ACT360);
    EXPECT_EQ(quote.calendar, domain::BusinessCalendar::US);
    EXPECT_EQ(quote.bdc, domain::BusinessDayConvention::ModifiedFollowing);
    EXPECT_EQ(quote.settlement_days, 1);
    EXPECT_EQ(quote.market_ts, "2026-03-24");
    EXPECT_EQ(quote.recorded_ts, "2026-03-24T18:00:00Z");
    EXPECT_EQ(quote.source_name, "Composite");
    EXPECT_EQ(quote.source_ts, "2026-03-24T17:00:00Z");
    EXPECT_EQ(quote.stale_after_days, 2);

    const auto curve_id =
        nlohmann::json::parse(R"json({"currency": "EUR", "family": "OIS"})json").get<domain::CurveId>();
    EXPECT_EQ(curve_id.currency, domain::Currency::EUR);
    EXPECT_EQ(curve_id.family, "OIS");
}

TEST(MarketTest, MarketDateParsingSeverityAndCurveOrderingCoverEdges) {
    EXPECT_TRUE(domain::market_data_detail::parse_iso_day("2026-03-24").has_value());
    EXPECT_FALSE(domain::market_data_detail::parse_iso_day("2026-02-31").has_value());
    EXPECT_FALSE(domain::market_data_detail::parse_iso_day("2026-0A-24").has_value());

    EXPECT_EQ(domain::market_data_detail::diagnostic_severity_rank("ERROR"), 0);
    EXPECT_EQ(domain::market_data_detail::diagnostic_severity_rank("WARNING"), 1);
    EXPECT_EQ(domain::market_data_detail::diagnostic_severity_rank("INFO"), 2);

    const domain::CurveId eur_ois{domain::Currency::EUR, "OIS"};
    const domain::CurveId usd_ois{domain::Currency::USD, "OIS"};
    const domain::CurveId usd_sofr{domain::Currency::USD, "SOFR"};
    EXPECT_TRUE(eur_ois < usd_ois);
    EXPECT_TRUE(usd_ois < usd_sofr);
}

TEST(MarketTest, PricingContextResolvesCurveIdsAndMarketStateStoresCurves) {
    const QuantLib::Date valuation_date(24, QuantLib::March, 2024);
    MarketState state(valuation_date);
    const domain::CurveId curve_id{domain::Currency::USD, "OIS"};
    auto curve = QuantLib::ext::make_shared<QuantLib::FlatForward>(valuation_date, 0.05, QuantLib::Actual365Fixed());

    state.add_curve(curve_id, curve);

    EXPECT_EQ(state.valuation_date(), valuation_date);
    EXPECT_EQ(state.get_curve(curve_id), curve);

    auto shared_state = std::make_shared<MarketState>(valuation_date);
    qrp::analytics::PricingContext context(shared_state);
    EXPECT_EQ(context.market_state_ptr(), shared_state);
    EXPECT_EQ(context.get_discount_curve_id(domain::Currency::USD).currency, domain::Currency::USD);
    EXPECT_EQ(context.get_discount_curve_id(domain::Currency::USD).family, "OIS");
    EXPECT_EQ(context.get_forecast_curve_id(domain::Currency::USD, "IBOR_3M").family, "IBOR_3M");
}
