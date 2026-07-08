// Verifies least-squares Monte Carlo valuation against an American put smoke benchmark.

#include <qrp/analytics/dynamic_programming/decision_problem.hpp>
#include <qrp/analytics/exercise_policy.hpp>
#include <qrp/analytics/lsmc/lsmc_engine.hpp>
#include <qrp/analytics/simulation/gbm.hpp>

#include <gtest/gtest.h>

#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

using namespace qrp::analytics;

class AmericanPutProblem : public dynamic_programming::DecisionProblem {
public:
    AmericanPutProblem(double strike) : strike_(strike) {}

    std::vector<dynamic_programming::Action> feasibleActions(const dynamic_programming::State&,
                                                             std::size_t) const override {
        return {{0, "Continue", {}}, {1, "Exercise", {}}};
    }

    double immediateCashflow(const dynamic_programming::State& state,
                             const dynamic_programming::Action& action,
                             std::size_t) const override {
        if (action.id == 1) { // Exercise
            double spot = state.market_variables[0];
            return std::max(strike_ - spot, 0.0);
        }
        return 0.0;
    }

    dynamic_programming::State nextState(const dynamic_programming::State&,
                                         const dynamic_programming::Action&,
                                         const std::vector<double>& market_variables_next,
                                         std::size_t) const override {
        return {market_variables_next, {}};
    }

    bool isTerminalAction(const dynamic_programming::State&,
                          const dynamic_programming::Action& action,
                          std::size_t) const override {
        return action.id == 1;
    }

    std::vector<double> regressionFeatures(const dynamic_programming::State& state, std::size_t) const override {
        double s = state.market_variables[0];
        return {1.0, s, s * s};
    }

private:
    double strike_;
};

TEST(LsmcTest, AmericanPutValuation) {
    double s0 = 100.0;
    double strike = 100.0;
    double r = 0.05;
    double vol = 0.2;
    double T = 1.0;
    int steps = 50;

    std::vector<double> times;
    for (int i = 0; i <= steps; ++i) {
        times.push_back(i * T / steps);
    }
    simulation::TimeGrid grid(times);
    simulation::GeometricBrownianMotion gbm(s0, r, vol);
    AmericanPutProblem problem(strike);

    lsmc::LsmcConfig config;
    config.num_paths = 10000;
    config.seed = 42;
    config.discount_rate = r;

    lsmc::LsmcEngine engine(config);
    dynamic_programming::State initial_state{{s0}, {}};
    auto result = engine.run(grid, gbm, problem, initial_state);

    std::cout << "American Put Value: " << result.value << " +/- " << result.standard_error << std::endl;

    // Black-Scholes European Put is approx 6.09
    // American Put should be slightly higher, around 6.5 - 7.0
    EXPECT_GT(result.value, 6.0);
    EXPECT_LT(result.value, 8.0);
    EXPECT_EQ(result.path_values.size(), config.num_paths);
    EXPECT_EQ(result.sorted_path_values.size(), config.num_paths);
    EXPECT_FALSE(result.regression_diagnostics.empty());
    EXPECT_EQ(result.basis_function_names, std::vector<std::string>({"basis_0", "basis_1", "basis_2"}));
    EXPECT_EQ(result.config_tags.at("seed"), "42");
}

class DefaultDecisionProblem : public dynamic_programming::DecisionProblem {
public:
    std::vector<dynamic_programming::Action> feasibleActions(const dynamic_programming::State&,
                                                             std::size_t) const override {
        return {{0, "Continue", {}}};
    }

    double immediateCashflow(const dynamic_programming::State&,
                             const dynamic_programming::Action&,
                             std::size_t) const override {
        return 0.0;
    }

    dynamic_programming::State nextState(const dynamic_programming::State&,
                                         const dynamic_programming::Action&,
                                         const std::vector<double>& market_variables_next,
                                         std::size_t) const override {
        return {market_variables_next, {}};
    }

    std::vector<double> regressionFeatures(const dynamic_programming::State& state, std::size_t) const override {
        return state.market_variables;
    }
};

class NoActionDecisionProblem : public dynamic_programming::DecisionProblem {
public:
    std::vector<dynamic_programming::Action> feasibleActions(const dynamic_programming::State&,
                                                             std::size_t) const override {
        return {};
    }

    double immediateCashflow(const dynamic_programming::State&,
                             const dynamic_programming::Action&,
                             std::size_t) const override {
        return 0.0;
    }

    dynamic_programming::State nextState(const dynamic_programming::State&,
                                         const dynamic_programming::Action&,
                                         const std::vector<double>& market_variables_next,
                                         std::size_t) const override {
        return {market_variables_next, {}};
    }

    std::vector<double> regressionFeatures(const dynamic_programming::State& state, std::size_t) const override {
        return {1.0, state.market_variables[0]};
    }

    double terminalValue(const dynamic_programming::State& state) const override {
        return state.market_variables[0];
    }
};

class EmptyFeatureDecisionProblem : public dynamic_programming::DecisionProblem {
public:
    std::vector<dynamic_programming::Action> feasibleActions(const dynamic_programming::State&,
                                                             std::size_t) const override {
        return {{0, "Continue", {}}};
    }

    double immediateCashflow(const dynamic_programming::State&,
                             const dynamic_programming::Action&,
                             std::size_t) const override {
        return 0.0;
    }

    dynamic_programming::State nextState(const dynamic_programming::State&,
                                         const dynamic_programming::Action&,
                                         const std::vector<double>& market_variables_next,
                                         std::size_t) const override {
        return {market_variables_next, {}};
    }

    std::vector<double> regressionFeatures(const dynamic_programming::State&, std::size_t) const override {
        return {};
    }

    double terminalValue(const dynamic_programming::State& state) const override {
        return state.market_variables[0];
    }
};

TEST(LsmcTest, DecisionProblemDefaultsRemainUsableThroughBaseInterface) {
    std::unique_ptr<dynamic_programming::DecisionProblem> problem = std::make_unique<DefaultDecisionProblem>();
    const dynamic_programming::State state{{100.0}, {}};
    const dynamic_programming::Action action{0, "Continue", {}};

    EXPECT_FALSE(problem->isTerminalAction(state, action, 0));
    EXPECT_DOUBLE_EQ(problem->terminalValue(state), 0.0);

    problem.reset();
}

TEST(LsmcTest, EmptyActionSetCarriesContinuationValue) {
    simulation::TimeGrid grid({0.0, 0.5, 1.0});
    simulation::GeometricBrownianMotion gbm(100.0, 0.02, 0.1);
    NoActionDecisionProblem problem;

    lsmc::LsmcConfig config;
    config.num_paths = 64;
    config.seed = 7;
    config.discount_rate = 0.0;

    lsmc::LsmcEngine engine(config);
    const auto result = engine.run(grid, gbm, problem, {{100.0}, {}});

    ASSERT_EQ(result.path_values.size(), config.num_paths);
    EXPECT_GT(result.value, 80.0);
    EXPECT_LT(result.value, 125.0);
    EXPECT_GE(result.standard_error, 0.0);
    EXPECT_LE(result.expected_shortfall_95, result.var_95);
}

TEST(LsmcTest, ExercisePolicyAdapterPricesAmericanPut) {
    simulation::TimeGrid grid({0.0, 0.25, 0.5, 0.75, 1.0});
    simulation::GeometricBrownianMotion gbm(100.0, 0.05, 0.20);
    auto policy = std::make_shared<exercise::VanillaOptionExercisePolicy>(100.0, true, 3);
    exercise::ExercisePolicyDecisionProblem problem(policy, grid.size() - 1U);

    lsmc::LsmcConfig config;
    config.num_paths = 2048;
    config.seed = 11;
    config.discount_rate = 0.05;

    lsmc::LsmcEngine engine(config);
    const auto result = engine.run(grid, gbm, problem, {{100.0}, {}});

    EXPECT_GT(result.value, 4.0);
    EXPECT_LT(result.value, 10.0);
    EXPECT_EQ(result.basis_function_names, std::vector<std::string>({"1", "spot", "spot^2", "spot^3"}));
    ASSERT_FALSE(result.regression_diagnostics.empty());
    EXPECT_EQ(result.regression_diagnostics.front().basis_function_count, 4U);
    EXPECT_GT(result.regression_diagnostics.front().sample_count, 0U);
    EXPECT_GE(result.regression_diagnostics.front().continuation_count +
                  result.regression_diagnostics.front().exercise_count,
              result.regression_diagnostics.front().sample_count);
}

TEST(LsmcTest, AmericanOptionHelperIsSeedReproducibleAndSerializesDiagnostics) {
    lsmc::AmericanOptionLsmcRequest request;
    request.spot = 100.0;
    request.strike = 100.0;
    request.risk_free_rate = 0.05;
    request.volatility = 0.20;
    request.maturity = 1.0;
    request.exercise_steps = 12;
    request.basis_degree = 2;
    request.config.num_paths = 1024;
    request.config.seed = 99;
    request.config.discount_rate = request.risk_free_rate;

    const auto first = lsmc::price_american_option(request);
    const auto second = lsmc::price_american_option(request);

    EXPECT_DOUBLE_EQ(first.value, second.value);
    EXPECT_EQ(first.path_values, second.path_values);
    EXPECT_EQ(first.path_values.size(), request.config.num_paths);
    EXPECT_EQ(first.sorted_path_values.size(), request.config.num_paths);
    EXPECT_EQ(first.basis_function_names, std::vector<std::string>({"1", "spot", "spot^2"}));
    EXPECT_EQ(first.config_tags.at("seed"), "99");
    EXPECT_EQ(first.config_tags.at("num_paths"), "1024");
    EXPECT_EQ(first.config_tags.at("option_type"), "put");
    ASSERT_FALSE(first.regression_diagnostics.empty());
    EXPECT_EQ(first.regression_diagnostics.front().sample_count, request.config.num_paths);
    EXPECT_GE(first.standard_error, 0.0);
    EXPECT_LE(first.expected_shortfall_95, first.var_95 + 1.0e-12);
}

TEST(LsmcTest, AmericanOptionHelperReturnsIntrinsicFallbackForDegenerateInput) {
    lsmc::AmericanOptionLsmcRequest request;
    request.spot = 80.0;
    request.strike = 100.0;
    request.maturity = 0.0;
    request.config.num_paths = 0;

    const auto result = lsmc::price_american_option(request);

    EXPECT_DOUBLE_EQ(result.value, 20.0);
    EXPECT_DOUBLE_EQ(result.standard_error, 0.0);
    EXPECT_EQ(result.path_values, std::vector<double>({20.0}));
    EXPECT_EQ(result.config_tags.at("status"), "expired_or_zero_maturity");
}

TEST(LsmcTest, EngineRejectsInvalidRuntimeInputs) {
    simulation::TimeGrid grid({0.0, 1.0});
    simulation::GeometricBrownianMotion gbm(100.0, 0.02, 0.1);
    NoActionDecisionProblem problem;

    lsmc::LsmcConfig config;
    config.num_paths = 0;
    lsmc::LsmcEngine zero_path_engine(config);
    EXPECT_THROW(zero_path_engine.run(grid, gbm, problem, {{100.0}, {}}), std::invalid_argument);

    config.num_paths = 16;
    lsmc::LsmcEngine engine(config);
    EXPECT_THROW(engine.run(simulation::TimeGrid({0.0}), gbm, problem, {{100.0}, {}}), std::invalid_argument);

    EmptyFeatureDecisionProblem empty_feature_problem;
    EXPECT_THROW(engine.run(grid, gbm, empty_feature_problem, {{100.0}, {}}), std::invalid_argument);
}
