// Verifies binding-aware scenario application from canonical factors to mutable quote handles.

#include <qrp/domain/factors.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <qrp/market/scenario_engine.hpp>

#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>
#include <string>

using namespace qrp::market;
using namespace qrp;

namespace {

domain::FactorBinding make_factor_binding(const std::string& factor_id,
                                          const std::string& quote_id,
                                          domain::ShockMeasure shock_measure,
                                          double weight = 1.0) {
    domain::FactorBinding binding;
    binding.factor_id = factor_id;
    binding.quote_id = quote_id;
    binding.shock_measure = shock_measure;
    binding.weight = weight;
    return binding;
}

} // namespace

TEST(ScenarioEngineTest, TestFactorBindingApplication) {
    domain::MarketSnapshot base;
    base.valuation_date = "2024-01-02";
    domain::MarketQuote q1;
    q1.id = "USD_OIS_5Y";
    q1.value = 0.03;
    q1.currency = domain::Currency::USD;
    base.quotes.push_back(q1);

    domain::FactorDefinition f1;
    f1.factor_id = domain::make_rates_factor_id(domain::Currency::USD, "OIS", "5Y");
    f1.factor_type = domain::FactorType::RateZero;
    f1.shock_measure = domain::ShockMeasure::Absolute;

    domain::FactorBinding b1;
    b1.factor_id = f1.factor_id;
    b1.quote_id = "USD_OIS_5Y";
    b1.weight = 1.0;
    b1.shock_measure = domain::ShockMeasure::Absolute;

    ScenarioDefinition scenario;
    scenario.factor_shocks[f1.factor_id] = 0.0001;

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
    scenario.factor_shocks[domain::make_rates_factor_id(domain::Currency::USD, "OIS", "5Y")] = 0.0001;

    MarketSnapshot snapshot(base);
    auto state = snapshot.built_state();

    EXPECT_THROW(ScenarioEngine::apply_scenario_to_state(*state, base, scenario, {}, {}), std::invalid_argument);
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
    f1.factor_id = domain::make_rates_factor_id(domain::Currency::USD, "OIS", "5Y");
    f1.factor_type = domain::FactorType::RateZero;

    domain::FactorBinding b1;
    b1.factor_id = f1.factor_id;
    b1.quote_id = "USD_OIS_10Y";

    ScenarioDefinition scenario;
    scenario.factor_shocks[f1.factor_id] = 0.0001;

    MarketSnapshot snapshot(base);
    auto state = snapshot.built_state();

    EXPECT_THROW(ScenarioEngine::apply_scenario_to_state(*state, base, scenario, {f1}, {b1}), std::invalid_argument);
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
    bindings.push_back(make_factor_binding("EQ", "EQUITY", domain::ShockMeasure::Relative));
    bindings.push_back(make_factor_binding("FX", "FX", domain::ShockMeasure::LogReturn));
    bindings.push_back(make_factor_binding("RATE", "RATE", domain::ShockMeasure::BasisPoints));
    bindings.push_back(make_factor_binding("VOL", "VOL", domain::ShockMeasure::VolPoints));

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
    bindings.push_back(make_factor_binding("PARALLEL_USD", "Q1", domain::ShockMeasure::Absolute));
    bindings.push_back(make_factor_binding("PARALLEL_USD", "Q2", domain::ShockMeasure::Absolute, 0.5));

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

    EXPECT_THROW(ScenarioEngine::apply_scenario_to_state(*state, base, scenario, {factor}, {}), std::invalid_argument);
}
