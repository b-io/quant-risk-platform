#include <gtest/gtest.h>
#include <qrp/market/scenario_engine.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/factors.hpp>
#include <qrp/market/market_snapshot.hpp>

using namespace qrp::market;
using namespace qrp;

TEST(ScenarioEngineTest, TestFactorBindingApplication) {
    domain::MarketSnapshot base;
    domain::MarketQuote q1;
    q1.id = "USD_OIS_5Y";
    q1.value = 0.03;
    q1.currency = domain::Currency::USD;
    base.quotes.push_back(q1);

    domain::FactorDefinition f1;
    f1.factor_id = "RF:RATE:USD:5Y";
    f1.factor_type = domain::FactorType::RateZero;
    f1.shock_measure = domain::ShockMeasure::Absolute;

    domain::FactorBinding b1;
    b1.factor_id = "RF:RATE:USD:5Y";
    b1.quote_id = "USD_OIS_5Y";
    b1.weight = 1.0;
    b1.shock_measure = domain::ShockMeasure::Absolute;

    ScenarioDefinition scenario;
    scenario.factor_shocks["RF:RATE:USD:5Y"] = 0.0001;

    MarketSnapshot snapshot(base);
    auto state = snapshot.built_state();

    ScenarioEngine::apply_scenario_to_state(*state, base, scenario, {f1}, {b1});
}
