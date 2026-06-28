#pragma once

// Declares least-squares Monte Carlo interfaces for exercise policies and continuation fitting.

#include <qrp/analytics/dynamic_programming/decision_problem.hpp>
#include <qrp/analytics/regression/regression_model.hpp>
#include <qrp/analytics/simulation/stochastic_process.hpp>
#include <cstdint>
#include <memory>
#include <utility>

namespace qrp::analytics::lsmc {

/**
 * @brief Runtime configuration for least-squares Monte Carlo valuation.
 */
struct LsmcConfig {
    std::size_t num_paths = 10000;
    std::uint64_t seed = 42;
    double discount_rate = 0.05;
};

/**
 * @brief Valuation output and path diagnostics from an LSMC run.
 */
struct LsmcResult {
    double value;
    double standard_error;
    std::vector<double> path_values;
    double var_95;
    double expected_shortfall_95;
};

/**
 * @brief Least-squares Monte Carlo engine for exercise-policy valuation.
 */
class LsmcEngine {
public:
    /**
     * @brief Creates an LSMC engine from fixed runtime configuration.
     */
    explicit LsmcEngine(LsmcConfig config) : config_(std::move(config)) {}

    /**
     * @brief Runs backward induction over simulated paths and decision problem states.
     */
    LsmcResult run(
        const simulation::TimeGrid& time_grid,
        const simulation::StochasticProcess& process,
        const dynamic_programming::DecisionProblem& problem,
        const dynamic_programming::State& initial_state
    ) const;

private:
    LsmcConfig config_;
};

} // namespace qrp::analytics::lsmc
