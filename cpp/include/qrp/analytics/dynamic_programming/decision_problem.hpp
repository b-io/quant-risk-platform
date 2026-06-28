#pragma once

// Declares generic dynamic-programming contracts for state, action, transition, and reward models.

#include <map>
#include <string>
#include <vector>

namespace qrp::analytics::dynamic_programming {

/**
 * @brief Minimal state vector for dynamic-programming examples and LSMC exercise policies.
 */
struct State {
    std::vector<double> market_variables;
    std::vector<double> operational_variables;
};

/**
 * @brief Discrete action with optional numeric parameters.
 */
struct Action {
    int id;
    std::string name;
    std::map<std::string, double> parameters;
};

/**
 * @brief Contract for products that can be solved by dynamic programming or LSMC.
 */
class DecisionProblem {
public:
    /**
     * @brief Allows derived decision problems to be deleted through the base type.
     */
    virtual ~DecisionProblem() = default;

    /**
     * @brief Returns the admissible actions at a state and time index.
     */
    virtual std::vector<Action> feasibleActions(
        const State& state,
        std::size_t timeIndex
    ) const = 0;

    /**
     * @brief Returns the immediate cashflow from applying an action.
     */
    virtual double immediateCashflow(
        const State& state,
        const Action& action,
        std::size_t timeIndex
    ) const = 0;

    /**
     * @brief Evolves operational state after an action and next market state.
     */
    virtual State nextState(
        const State& state,
        const Action& action,
        const std::vector<double>& market_variables_next,
        std::size_t timeIndex
    ) const = 0;

    /**
     * @brief Returns whether an action ends the decision process.
     */
    virtual bool isTerminalAction(
        const State&,
        const Action&,
        std::size_t
    ) const {
        return false;
    }

    /**
     * @brief Returns state features used for continuation-value regression.
     */
    virtual std::vector<double> regressionFeatures(
        const State& state,
        std::size_t timeIndex
    ) const = 0;

    /**
     * @brief Returns terminal payoff or penalty at maturity.
     */
    virtual double terminalValue(const State&) const { return 0.0; }
};

} // namespace qrp::analytics::dynamic_programming
