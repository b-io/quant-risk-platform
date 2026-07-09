#include <qrp/analytics/dynamic_programming/gas_storage_policy.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

namespace qrp::analytics::dynamic_programming {

GasStorageDecisionProblem::GasStorageDecisionProblem(GasStoragePolicyParams params) : params_(std::move(params)) {
    params_.min_inventory = std::max(params_.min_inventory, 0.0);
    params_.max_inventory = std::max(params_.max_inventory, params_.min_inventory);
    params_.max_injection_quantity = std::max(params_.max_injection_quantity, 0.0);
    params_.max_withdrawal_quantity = std::max(params_.max_withdrawal_quantity, 0.0);
    params_.terminal_inventory_target =
        std::clamp(params_.terminal_inventory_target, params_.min_inventory, params_.max_inventory);
    params_.terminal_inventory_penalty = std::max(params_.terminal_inventory_penalty, 0.0);
}

std::vector<Action> GasStorageDecisionProblem::feasibleActions(const State& state, std::size_t) const {
    const double inventory = current_inventory(state);
    std::set<double> deltas;
    add_delta(deltas, 0.0);
    add_delta(deltas, std::min(params_.max_injection_quantity, params_.max_inventory - inventory));
    add_delta(deltas, -std::min(params_.max_withdrawal_quantity, inventory - params_.min_inventory));
    add_delta(deltas,
              std::clamp(params_.terminal_inventory_target - inventory,
                         -params_.max_withdrawal_quantity,
                         params_.max_injection_quantity));

    std::vector<Action> actions;
    int action_id = 0;
    for (double delta : deltas) {
        const double next_inventory = inventory + delta;
        if (next_inventory < params_.min_inventory - 1.0e-10 || next_inventory > params_.max_inventory + 1.0e-10) {
            continue;
        }
        const std::string name = delta > 1.0e-10 ? "Inject" : (delta < -1.0e-10 ? "Withdraw" : "Idle");
        actions.push_back({action_id++, name, {{"inventory_delta", delta}}});
    }
    return actions;
}

double
GasStorageDecisionProblem::immediateCashflow(const State& state, const Action& action, std::size_t time_index) const {
    const double price = state.market_variables.empty() ? 0.0 : state.market_variables.front();
    const double delta = action_delta(action);
    const double discount = time_index < params_.discount_factors.size() ? params_.discount_factors[time_index] : 1.0;
    if (delta > 0.0) {
        return -(price + params_.injection_cost) * delta * discount;
    }
    if (delta < 0.0) {
        return (price - params_.withdrawal_cost) * -delta * discount;
    }
    return 0.0;
}

State GasStorageDecisionProblem::nextState(const State& state,
                                           const Action& action,
                                           const std::vector<double>& market_variables_next,
                                           std::size_t) const {
    const double next_inventory =
        std::clamp(current_inventory(state) + action_delta(action), params_.min_inventory, params_.max_inventory);
    return {market_variables_next, {next_inventory}};
}

std::vector<double> GasStorageDecisionProblem::regressionFeatures(const State& state, std::size_t) const {
    const double price = state.market_variables.empty() ? 0.0 : state.market_variables.front();
    const double inventory = current_inventory(state);
    return {1.0, price, inventory, price * price, inventory * inventory, price * inventory};
}

std::vector<std::string> GasStorageDecisionProblem::regressionFeatureNames(std::size_t) const {
    return {"1", "price", "inventory", "price^2", "inventory^2", "price*inventory"};
}

double GasStorageDecisionProblem::terminalValue(const State& state) const {
    const double deviation = std::abs(current_inventory(state) - params_.terminal_inventory_target);
    const double discount = params_.discount_factors.empty() ? 1.0 : params_.discount_factors.back();
    return -params_.terminal_inventory_penalty * deviation * discount;
}

double GasStorageDecisionProblem::current_inventory(const State& state) {
    return state.operational_variables.empty() ? 0.0 : state.operational_variables.front();
}

double GasStorageDecisionProblem::action_delta(const Action& action) {
    const auto it = action.parameters.find("inventory_delta");
    return it == action.parameters.end() ? 0.0 : it->second;
}

void GasStorageDecisionProblem::add_delta(std::set<double>& deltas, double delta) {
    if (std::isfinite(delta)) {
        deltas.insert(std::round(delta * 1.0e8) / 1.0e8);
    }
}

} // namespace qrp::analytics::dynamic_programming
