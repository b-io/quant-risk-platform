#pragma once

// Defines a compact power-plant dispatch example for dynamic-programming workflows.

#include <qrp/analytics/dynamic_programming/decision_problem.hpp>

#include <algorithm>
#include <utility>

namespace qrp::analytics::examples {

/**
 * @brief Simplified thermal power-plant dispatch problem for dynamic programming.
 */
class PowerPlantProblem : public dynamic_programming::DecisionProblem {
public:
    /**
     * @brief Plant economics and operating constraints.
     */
    struct Params {
        double capacity = 100.0; // MW
        double heat_rate = 2.0;  // Fuel units per MWh
        double startup_cost = 500.0;
        double variable_opex = 2.0;
        double co2_intensity = 0.5; // tons/MWh
    };

    /**
     * @brief Creates a dispatch problem from fixed plant parameters.
     */
    PowerPlantProblem(Params params) : params_(std::move(params)) {}

    /**
     * @brief Returns off and on dispatch actions.
     */
    std::vector<dynamic_programming::Action> feasibleActions(
        const dynamic_programming::State& state,
        std::size_t timeIndex
    ) const override {
        return {
            {0, "Off", {}},
            {1, "On", {}}
        };
    }

    /**
     * @brief Returns spark-spread cashflow net of startup and variable costs.
     */
    double immediateCashflow(
        const dynamic_programming::State& state,
        const dynamic_programming::Action& action,
        std::size_t timeIndex
    ) const override {
        if (action.id == 0) return 0.0;

        double power_price = state.market_variables[0];
        double fuel_price = state.market_variables[1];
        double co2_price = state.market_variables[2];
        bool was_on = (state.operational_variables[0] > 0.5);

        double margin = power_price - (fuel_price * params_.heat_rate) - (co2_price * params_.co2_intensity) - params_.variable_opex;
        double flow = margin * params_.capacity;

        if (!was_on) {
            flow -= params_.startup_cost;
        }

        return flow;
    }

    /**
     * @brief Updates the on/off operational state.
     */
    dynamic_programming::State nextState(
        const dynamic_programming::State& state,
        const dynamic_programming::Action& action,
        const std::vector<double>& market_variables_next,
        std::size_t timeIndex
    ) const override {
        double is_on = (action.id == 1) ? 1.0 : 0.0;
        return {market_variables_next, {is_on}};
    }

    /**
     * @brief Returns polynomial features of power, fuel, carbon, and state.
     */
    std::vector<double> regressionFeatures(
        const dynamic_programming::State& state,
        std::size_t timeIndex
    ) const override {
        double p = state.market_variables[0];
        double f = state.market_variables[1];
        double c = state.market_variables[2];
        double on = state.operational_variables[0];
        return {1.0, p, f, c, on, p * p, f * f, c * c, p * f, p * c, f * c};
    }

private:
    Params params_;
};

} // namespace qrp::analytics::examples
