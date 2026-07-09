#pragma once

// Defines the inventory-control decision problem used by gas-storage valuation.

#include <qrp/analytics/dynamic_programming/decision_problem.hpp>

#include <set>

namespace qrp::analytics::dynamic_programming {

struct GasStoragePolicyParams {
    double min_inventory = 0.0;
    double max_inventory = 0.0;
    double max_injection_quantity = 0.0;
    double max_withdrawal_quantity = 0.0;
    double injection_cost = 0.0;
    double withdrawal_cost = 0.0;
    double terminal_inventory_target = 0.0;
    double terminal_inventory_penalty = 0.0;
    std::vector<double> discount_factors;
};

class GasStorageDecisionProblem final : public DecisionProblem {
public:
    explicit GasStorageDecisionProblem(GasStoragePolicyParams params);

    std::vector<Action> feasibleActions(const State& state, std::size_t time_index) const override;

    double immediateCashflow(const State& state, const Action& action, std::size_t time_index) const override;

    State nextState(const State& state,
                    const Action& action,
                    const std::vector<double>& market_variables_next,
                    std::size_t time_index) const override;

    std::vector<double> regressionFeatures(const State& state, std::size_t time_index) const override;

    std::vector<std::string> regressionFeatureNames(std::size_t time_index) const override;

    double terminalValue(const State& state) const override;

private:
    static double current_inventory(const State& state);

    static double action_delta(const Action& action);

    static void add_delta(std::set<double>& deltas, double delta);

    GasStoragePolicyParams params_;
};

} // namespace qrp::analytics::dynamic_programming
