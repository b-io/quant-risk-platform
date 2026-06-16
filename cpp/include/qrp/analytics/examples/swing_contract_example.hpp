#pragma once
#include <qrp/analytics/dynamic_programming/decision_problem.hpp>
#include <cmath>
#include <algorithm>

namespace qrp::analytics::examples {

class SwingContractProblem : public dynamic_programming::DecisionProblem {
public:
    struct Params {
        double min_daily_volume = 0.0;
        double max_daily_volume = 10.0;
        double min_total_volume = 100.0;
        double max_total_volume = 500.0;
        double strike = 30.0;
    };

    SwingContractProblem(Params params) : params_(std::move(params)) {}

    std::vector<dynamic_programming::Action> feasibleActions(
        const dynamic_programming::State& state,
        std::size_t timeIndex
    ) const override {
        // Actions: 0 = Min, 1 = Max
        return {
            {0, "Min", {}},
            {1, "Max", {}}
        };
    }

    double immediateCashflow(
        const dynamic_programming::State& state,
        const dynamic_programming::Action& action,
        std::size_t timeIndex
    ) const override {
        double price = state.market_variables[0];
        double volume = (action.id == 0) ? params_.min_daily_volume : params_.max_daily_volume;
        return (price - params_.strike) * volume;
    }

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

    std::vector<double> regressionFeatures(
        const dynamic_programming::State& state,
        std::size_t timeIndex
    ) const override {
        double p = state.market_variables[0];
        double v = state.operational_variables[0];
        return {1.0, p, v, p * p, v * v, p * v};
    }
    
    double terminalValue(const dynamic_programming::State& state) const override {
        double total_v = state.operational_variables[0];
        if (total_v < params_.min_total_volume) {
            // Penalty for not reaching minimum
            return -1000.0 * (params_.min_total_volume - total_v);
        }
        return 0.0;
    }

private:
    Params params_;
};

} // namespace qrp::analytics::examples
