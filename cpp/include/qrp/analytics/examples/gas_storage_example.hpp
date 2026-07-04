#pragma once

// Defines a gas-storage control example for dynamic-programming workflows.

#include <qrp/analytics/dynamic_programming/decision_problem.hpp>

#include <algorithm>
#include <utility>

namespace qrp::analytics::examples {

/**
 * @brief Simplified gas-storage exercise problem for dynamic-programming tests.
 */
class GasStorageProblem : public dynamic_programming::DecisionProblem {
public:
    /**
     * @brief Operational constraints and exercise costs for storage.
     */
    struct Params {
        double min_inventory = 0.0;
        double max_inventory = 100.0;
        double max_injection_rate = 10.0;
        double max_withdrawal_rate = 10.0;
        double injection_cost = 0.5;
        double withdrawal_cost = 0.5;
    };

    /**
     * @brief Creates a gas-storage problem from fixed operational parameters.
     */
    GasStorageProblem(Params params) : params_(std::move(params)) {}

    /**
     * @brief Returns idle, inject, and withdraw actions.
     */
    std::vector<dynamic_programming::Action> feasibleActions(
        const dynamic_programming::State& state,
        std::size_t timeIndex
    ) const override {
        // The example uses a compact action set; production storage models can use finer volume grids.
        return {
            {0, "Idle", {}},
            {1, "Inject", {}},
            {2, "Withdraw", {}}
        };
    }

    /**
     * @brief Returns the cashflow from injection or withdrawal at the current price.
     */
    double immediateCashflow(
        const dynamic_programming::State& state,
        const dynamic_programming::Action& action,
        std::size_t timeIndex
    ) const override {
        double price = state.market_variables[0];
        double inventory = state.operational_variables[0];

        if (action.id == 1) { // Inject
            double volume = std::min(params_.max_injection_rate, params_.max_inventory - inventory);
            return -(price + params_.injection_cost) * volume;
        } else if (action.id == 2) { // Withdraw
            double volume = std::min(params_.max_withdrawal_rate, inventory - params_.min_inventory);
            return (price - params_.withdrawal_cost) * volume;
        }
        return 0.0;
    }

    /**
     * @brief Updates inventory after the selected storage action.
     */
    dynamic_programming::State nextState(
        const dynamic_programming::State& state,
        const dynamic_programming::Action& action,
        const std::vector<double>& market_variables_next,
        std::size_t timeIndex
    ) const override {
        double inventory = state.operational_variables[0];
        double next_inventory = inventory;

        if (action.id == 1) { // Inject
            next_inventory += std::min(params_.max_injection_rate, params_.max_inventory - inventory);
        } else if (action.id == 2) { // Withdraw
            next_inventory -= std::min(params_.max_withdrawal_rate, inventory - params_.min_inventory);
        }

        return {market_variables_next, {next_inventory}};
    }

    /**
     * @brief Returns polynomial features of price and inventory.
     */
    std::vector<double> regressionFeatures(
        const dynamic_programming::State& state,
        std::size_t timeIndex
    ) const override {
        double p = state.market_variables[0];
        double v = state.operational_variables[0];
        return {1.0, p, v, p * p, v * v, p * v};
    }

private:
    Params params_;
};

} // namespace qrp::analytics::examples
