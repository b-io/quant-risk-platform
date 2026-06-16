#pragma once
#include <vector>
#include <string>
#include <variant>
#include <map>

namespace qrp::analytics::dynamic_programming {

struct State {
    std::vector<double> market_variables;
    std::vector<double> operational_variables;
};

struct Action {
    int id;
    std::string name;
    std::map<std::string, double> parameters;
};

class DecisionProblem {
public:
    virtual ~DecisionProblem() = default;

    virtual std::vector<Action> feasibleActions(
        const State& state,
        std::size_t timeIndex
    ) const = 0;

    virtual double immediateCashflow(
        const State& state,
        const Action& action,
        std::size_t timeIndex
    ) const = 0;

    virtual State nextState(
        const State& state,
        const Action& action,
        const std::vector<double>& market_variables_next,
        std::size_t timeIndex
    ) const = 0;

    virtual std::vector<double> regressionFeatures(
        const State& state,
        std::size_t timeIndex
    ) const = 0;
    
    virtual double terminalValue(const State& state) const { return 0.0; }
};

} // namespace qrp::analytics::dynamic_programming
