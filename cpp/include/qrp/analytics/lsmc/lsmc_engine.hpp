#pragma once
#include <qrp/analytics/simulation/stochastic_process.hpp>
#include <qrp/analytics/dynamic_programming/decision_problem.hpp>
#include <qrp/analytics/regression/regression_model.hpp>
#include <memory>

namespace qrp::analytics::lsmc {

struct LsmcConfig {
    std::size_t num_paths = 10000;
    std::uint64_t seed = 42;
    double discount_rate = 0.05;
};

struct LsmcResult {
    double value;
    double standard_error;
    std::vector<double> path_values;
    double var_95;
    double expected_shortfall_95;
};

class LsmcEngine {
public:
    explicit LsmcEngine(LsmcConfig config) : config_(std::move(config)) {}

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
