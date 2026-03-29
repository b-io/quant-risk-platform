#include <gtest/gtest.h>
#include <qrp/market/market_snapshot.hpp>
#include <ql/termstructures/yield/piecewiseyieldcurve.hpp>
#include <ql/termstructures/yield/oisratehelper.hpp>
#include <ql/indexes/ibor/sofr.hpp>
#include <qrp/domain/market_data.hpp>
#include <iostream>

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
