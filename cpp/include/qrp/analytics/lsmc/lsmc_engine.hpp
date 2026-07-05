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
    std::size_t num_paths = 10000;   // Number of simulated paths.
    std::uint64_t seed = 42;         // Random seed for reproducible paths.
    double discount_rate = 0.05;     // Flat annual discount rate used by the example engine.
};

/**
 * @brief Valuation output and path diagnostics from an LSMC run.
 */
struct LsmcResult {
    double value;                         // Estimated option or contract value.
    double standard_error;                // Monte Carlo standard error of the estimate.
    std::vector<double> path_values;      // Discounted value by simulated path.
    double var_95;                        // 95% value-at-risk of path values.
    double expected_shortfall_95;         // 95% expected shortfall of path values.
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
    LsmcConfig config_; // Immutable runtime configuration for each run.
};

} // namespace qrp::analytics::lsmc
