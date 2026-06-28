// Verifies least-squares Monte Carlo valuation against an American put smoke benchmark.

#include <gtest/gtest.h>
#include <qrp/analytics/lsmc/lsmc_engine.hpp>
#include <qrp/analytics/simulation/gbm.hpp>
#include <qrp/analytics/dynamic_programming/decision_problem.hpp>
#include <cmath>
#include <iostream>

using namespace qrp::analytics;

class AmericanPutProblem : public dynamic_programming::DecisionProblem {
public:
    AmericanPutProblem(double strike) : strike_(strike) {}

    std::vector<dynamic_programming::Action> feasibleActions(
        const dynamic_programming::State&,
        std::size_t
    ) const override {
        return {
            {0, "Continue", {}},
            {1, "Exercise", {}}
        };
    }

    double immediateCashflow(
        const dynamic_programming::State& state,
        const dynamic_programming::Action& action,
        std::size_t
    ) const override {
        if (action.id == 1) { // Exercise
            double spot = state.market_variables[0];
            return std::max(strike_ - spot, 0.0);
        }
        return 0.0;
    }

    dynamic_programming::State nextState(
        const dynamic_programming::State&,
        const dynamic_programming::Action&,
        const std::vector<double>& market_variables_next,
        std::size_t
    ) const override {
        return {market_variables_next, {}};
    }

    bool isTerminalAction(
        const dynamic_programming::State&,
        const dynamic_programming::Action& action,
        std::size_t
    ) const override {
        return action.id == 1;
    }

    std::vector<double> regressionFeatures(
        const dynamic_programming::State& state,
        std::size_t
    ) const override {
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
}
