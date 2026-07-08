#pragma once

// Declares least-squares Monte Carlo interfaces for exercise policies and continuation fitting.

#include <qrp/analytics/dynamic_programming/decision_problem.hpp>
#include <qrp/analytics/exercise_policy.hpp>
#include <qrp/analytics/regression/regression_model.hpp>
#include <qrp/analytics/simulation/stochastic_process.hpp>

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace qrp::analytics::lsmc {

/**
 * @brief Runtime configuration for least-squares Monte Carlo valuation.
 */
struct LsmcConfig {
    std::size_t num_paths = 10000; // Number of simulated paths.
    std::uint64_t seed = 42;       // Random seed for reproducible paths.
    double discount_rate = 0.05;   // Flat annual discount rate used by the example engine.
};

/**
 * @brief Continuation-regression diagnostics captured at one backward-induction step.
 */
struct LsmcRegressionDiagnostic {
    std::size_t time_index = 0;              // Backward-induction time index.
    double time = 0.0;                       // Time-grid value in years.
    std::size_t sample_count = 0;            // Number of paths used in the regression.
    std::size_t basis_function_count = 0;    // Number of continuation-regression features.
    double r_squared = 0.0;                  // Regression coefficient of determination.
    double residual_sum_of_squares = 0.0;    // Regression residual sum of squares.
    std::size_t exercise_count = 0;          // Paths choosing a non-continue action.
    std::size_t terminal_exercise_count = 0; // Paths choosing a terminal exercise action.
    std::size_t continuation_count = 0;      // Paths carrying continuation value.
};

/**
 * @brief Valuation output and path diagnostics from an LSMC run.
 */
struct LsmcResult {
    double value = 0.0;                                           // Estimated option or contract value.
    double standard_error = 0.0;                                  // Monte Carlo standard error of the estimate.
    std::vector<double> path_values;                              // Discounted value by simulated path.
    std::vector<double> sorted_path_values;                       // Path values sorted ascending for tail diagnostics.
    double var_95 = 0.0;                                          // 95% value-at-risk of path values.
    double expected_shortfall_95 = 0.0;                           // 95% expected shortfall of path values.
    std::vector<std::string> basis_function_names;                // Serialized basis-function labels.
    std::vector<LsmcRegressionDiagnostic> regression_diagnostics; // Step-level regression diagnostics.
    std::map<std::string, std::string> config_tags;               // Serialized run configuration for audit/reporting.
};

/**
 * @brief Python-friendly request for C++-owned American option LSMC valuation.
 */
struct AmericanOptionLsmcRequest {
    double spot = 100.0;             // Initial spot level.
    double strike = 100.0;           // Strike price.
    double risk_free_rate = 0.05;    // Flat risk-free rate.
    double dividend_yield = 0.0;     // Continuous dividend yield.
    double borrow_rate = 0.0;        // Continuous borrow or stock-loan rate.
    double volatility = 0.20;        // Lognormal volatility.
    double maturity = 1.0;           // Option maturity in years.
    std::size_t exercise_steps = 50; // Number of exercise intervals.
    bool is_put = true;              // Put when true, call when false.
    int basis_degree = 2;            // Polynomial continuation basis degree.
    LsmcConfig config;               // Simulation and discounting configuration.
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
    LsmcResult run(const simulation::TimeGrid& time_grid,
                   const simulation::StochasticProcess& process,
                   const dynamic_programming::DecisionProblem& problem,
                   const dynamic_programming::State& initial_state) const;

private:
    LsmcConfig config_; // Immutable runtime configuration for each run.
};

/**
 * @brief Prices a vanilla American option using C++-owned GBM paths and exercise policy.
 */
LsmcResult price_american_option(const AmericanOptionLsmcRequest& request);

} // namespace qrp::analytics::lsmc
