#include <gtest/gtest.h>
#include <qrp/market/scenario_engine.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/factors.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <cmath>
#include <stdexcept>

using namespace qrp::market;
using namespace qrp;

TEST(ScenarioEngineTest, TestFactorBindingApplication) {
    domain::MarketSnapshot base;
    base.valuation_date = "2024-01-02";
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
    EXPECT_NEAR(state->get_quote("USD_OIS_5Y"), 0.0301, 1e-12);
}

TEST(ScenarioEngineTest, ThrowsWhenScenarioFactorIsUndefined) {
    domain::MarketSnapshot base;
    base.valuation_date = "2024-01-02";
    domain::MarketQuote q1;
    q1.id = "USD_OIS_5Y";
    q1.value = 0.03;
    q1.currency = domain::Currency::USD;
    base.quotes.push_back(q1);

    ScenarioDefinition scenario;
    scenario.factor_shocks["RF:RATE:USD:5Y"] = 0.0001;

    MarketSnapshot snapshot(base);
    auto state = snapshot.built_state();

    EXPECT_THROW(
        ScenarioEngine::apply_scenario_to_state(*state, base, scenario, {}, {}),
        std::invalid_argument);
}

TEST(ScenarioEngineTest, ThrowsWhenBindingTargetsMissingQuote) {
    domain::MarketSnapshot base;
    base.valuation_date = "2024-01-02";
    domain::MarketQuote q1;
    q1.id = "USD_OIS_5Y";
    q1.value = 0.03;
    q1.currency = domain::Currency::USD;
    base.quotes.push_back(q1);

    domain::FactorDefinition f1;
    f1.factor_id = "RF:RATE:USD:5Y";
    f1.factor_type = domain::FactorType::RateZero;

    domain::FactorBinding b1;
    b1.factor_id = "RF:RATE:USD:5Y";
    b1.quote_id = "USD_OIS_10Y";

    ScenarioDefinition scenario;
    scenario.factor_shocks["RF:RATE:USD:5Y"] = 0.0001;

    MarketSnapshot snapshot(base);
    auto state = snapshot.built_state();

    EXPECT_THROW(
        ScenarioEngine::apply_scenario_to_state(*state, base, scenario, {f1}, {b1}),
        std::invalid_argument);
}

TEST(ScenarioEngineTest, AppliesRelativeLogReturnBasisPointAndVolPointShocks) {
    domain::MarketSnapshot base;
    base.valuation_date = "2024-01-02";
    base.quotes.push_back({"EQUITY", domain::QuoteInstrumentType::Future, domain::Currency::USD, "SPOT", 100.0});
    base.quotes.push_back({"FX", domain::QuoteInstrumentType::Future, domain::Currency::USD, "SPOT", 1.20});
    base.quotes.push_back({"RATE", domain::QuoteInstrumentType::OIS, domain::Currency::USD, "5Y", 0.03});
    base.quotes.push_back({"VOL", domain::QuoteInstrumentType::CapFloorVol, domain::Currency::USD, "1Y", 0.20});

    std::vector<domain::FactorDefinition> factors;
    for (const auto& factor_id : {"EQ", "FX", "RATE", "VOL"}) {
        domain::FactorDefinition factor;
        factor.factor_id = factor_id;
        factors.push_back(factor);
    }

    std::vector<domain::FactorBinding> bindings;
    bindings.push_back({"EQ", "EQUITY", domain::ShockMeasure::Relative, 1.0});
    bindings.push_back({"FX", "FX", domain::ShockMeasure::LogReturn, 1.0});
    bindings.push_back({"RATE", "RATE", domain::ShockMeasure::BasisPoints, 1.0});
    bindings.push_back({"VOL", "VOL", domain::ShockMeasure::VolPoints, 1.0});

    ScenarioDefinition scenario;
    scenario.factor_shocks["EQ"] = 0.10;
    scenario.factor_shocks["FX"] = std::log(1.05);
    scenario.factor_shocks["RATE"] = 25.0;
    scenario.factor_shocks["VOL"] = 2.5;

    MarketSnapshot snapshot(base);
    auto state = snapshot.built_state();

    ScenarioEngine::apply_scenario_to_state(*state, base, scenario, factors, bindings);

    EXPECT_NEAR(state->get_quote("EQUITY"), 110.0, 1e-12);
    EXPECT_NEAR(state->get_quote("FX"), 1.26, 1e-12);
    EXPECT_NEAR(state->get_quote("RATE"), 0.0325, 1e-12);
    EXPECT_NEAR(state->get_quote("VOL"), 0.225, 1e-12);
}

TEST(ScenarioEngineTest, AppliesWeightedFactorToMultipleQuotes) {
    domain::MarketSnapshot base;
    base.valuation_date = "2024-01-02";
    base.quotes.push_back({"Q1", domain::QuoteInstrumentType::OIS, domain::Currency::USD, "2Y", 0.02});
    base.quotes.push_back({"Q2", domain::QuoteInstrumentType::OIS, domain::Currency::USD, "5Y", 0.03});

    domain::FactorDefinition factor;
    factor.factor_id = "PARALLEL_USD";

    std::vector<domain::FactorBinding> bindings;
    bindings.push_back({"PARALLEL_USD", "Q1", domain::ShockMeasure::Absolute, 1.0});
    bindings.push_back({"PARALLEL_USD", "Q2", domain::ShockMeasure::Absolute, 0.5});

    ScenarioDefinition scenario;
    scenario.factor_shocks["PARALLEL_USD"] = 0.001;

    MarketSnapshot snapshot(base);
    auto state = snapshot.built_state();

    ScenarioEngine::apply_scenario_to_state(*state, base, scenario, {factor}, bindings);

    EXPECT_NEAR(state->get_quote("Q1"), 0.021, 1e-12);
    EXPECT_NEAR(state->get_quote("Q2"), 0.0305, 1e-12);
}

TEST(ScenarioEngineTest, ThrowsWhenDefinedFactorHasNoBinding) {
    domain::MarketSnapshot base;
    base.valuation_date = "2024-01-02";
    base.quotes.push_back({"Q1", domain::QuoteInstrumentType::OIS, domain::Currency::USD, "2Y", 0.02});

    domain::FactorDefinition factor;
    factor.factor_id = "UNBOUND";

    ScenarioDefinition scenario;
    scenario.factor_shocks["UNBOUND"] = 0.001;

    MarketSnapshot snapshot(base);
    auto state = snapshot.built_state();

    EXPECT_THROW(
        ScenarioEngine::apply_scenario_to_state(*state, base, scenario, {factor}, {}),
        std::invalid_argument);
}
