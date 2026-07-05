#pragma once

// Defines a swing-contract exercise example for dynamic-programming workflows.

#include <qrp/analytics/dynamic_programming/decision_problem.hpp>

#include <algorithm>
#include <utility>

namespace qrp::analytics::examples {

/**
 * @brief Simplified swing-contract exercise problem for dynamic-programming tests.
 */
class SwingContractProblem : public dynamic_programming::DecisionProblem {
public:
    /**
     * @brief Volume constraints and fixed strike for the swing contract.
     */
    struct Params {
        double min_daily_volume = 0.0;   // Minimum exercised volume per step.
        double max_daily_volume = 10.0;  // Maximum exercised volume per step.
        double min_total_volume = 100.0; // Minimum cumulative volume over the contract.
        double max_total_volume = 500.0; // Maximum cumulative volume over the contract.
        double strike = 30.0;            // Fixed delivery strike.
    };

    /**
     * @brief Creates a swing-contract problem from fixed exercise parameters.
     */
    SwingContractProblem(Params params) : params_(std::move(params)) {}

    /**
     * @brief Returns minimum-volume and maximum-volume actions.
     */
    std::vector<dynamic_programming::Action> feasibleActions(
        const dynamic_programming::State& state,
        std::size_t timeIndex
    ) const override {
        return {
            {0, "Min", {}},
            {1, "Max", {}}
        };
    }

    /**
     * @brief Returns intrinsic exercise cashflow for the selected volume.
     */
    double immediateCashflow(
        const dynamic_programming::State& state,
        const dynamic_programming::Action& action,
        std::size_t timeIndex
    ) const override {
        double price = state.market_variables[0];
        double volume = (action.id == 0) ? params_.min_daily_volume : params_.max_daily_volume;
        return (price - params_.strike) * volume;
    }

    /**
     * @brief Accumulates exercised volume after the selected action.
     */
    dynamic_programming::State nextState(
        const dynamic_programming::State& state,
        const dynamic_programming::Action& action,
        const std::vector<double>& market_variables_next,
        std::size_t timeIndex
    ) const override {
        double current_total = state.operational_variables[0];
        double volume = (action.id == 0) ? params_.min_daily_volume : params_.max_daily_volume;
        return {market_variables_next, {current_total + volume}};
    }

    /**
     * @brief Returns polynomial features of price and cumulative volume.
     */
    std::vector<double> regressionFeatures(
        const dynamic_programming::State& state,
        std::size_t timeIndex
    ) const override {
        double p = state.market_variables[0];
        double v = state.operational_variables[0];
        return {1.0, p, v, p * p, v * v, p * v};
    }

    /**
     * @brief Applies a terminal penalty when minimum total volume is unmet.
     */
    double terminalValue(const dynamic_programming::State& state) const override {
        double total_v = state.operational_variables[0];
        if (total_v < params_.min_total_volume) {
            return -1000.0 * (params_.min_total_volume - total_v);
        }
        return 0.0;
    }

private:
    Params params_; // Fixed swing contract parameters.
};

} // namespace qrp::analytics::examples
