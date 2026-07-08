// Implements reusable exercise-policy adapters for LSMC decision problems.

#include <qrp/analytics/exercise_policy.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace qrp::analytics::exercise {

double ExercisePolicy::terminalValue(const dynamic_programming::State& state, std::size_t time_index) const {
    return canExercise(state, time_index) ? exerciseValue(state, time_index) : 0.0;
}

ExercisePolicyDecisionProblem::ExercisePolicyDecisionProblem(std::shared_ptr<const ExercisePolicy> policy,
                                                             std::size_t maturity_time_index)
    : policy_(std::move(policy)), maturity_time_index_(maturity_time_index) {
    if (!policy_) {
        throw std::invalid_argument("ExercisePolicyDecisionProblem requires a policy");
    }
}

std::vector<dynamic_programming::Action>
ExercisePolicyDecisionProblem::feasibleActions(const dynamic_programming::State& state, std::size_t time_index) const {
    if (!policy_->canExercise(state, time_index)) {
        return {{kContinueActionId, "Continue", {}}};
    }
    return {{kContinueActionId, "Continue", {}}, {kExerciseActionId, "Exercise", {}}};
}

double ExercisePolicyDecisionProblem::immediateCashflow(const dynamic_programming::State& state,
                                                        const dynamic_programming::Action& action,
                                                        std::size_t time_index) const {
    if (action.id != kExerciseActionId) {
        return 0.0;
    }
    return policy_->exerciseValue(state, time_index);
}

dynamic_programming::State ExercisePolicyDecisionProblem::nextState(const dynamic_programming::State& state,
                                                                    const dynamic_programming::Action&,
                                                                    const std::vector<double>& market_variables_next,
                                                                    std::size_t) const {
    return {market_variables_next, state.operational_variables};
}

bool ExercisePolicyDecisionProblem::isTerminalAction(const dynamic_programming::State&,
                                                     const dynamic_programming::Action& action,
                                                     std::size_t) const {
    return action.id == kExerciseActionId;
}

std::vector<double> ExercisePolicyDecisionProblem::regressionFeatures(const dynamic_programming::State& state,
                                                                      std::size_t time_index) const {
    auto features = policy_->regressionFeatures(state, time_index);
    if (features.empty()) {
        features.push_back(1.0);
    }
    return features;
}

std::vector<std::string> ExercisePolicyDecisionProblem::regressionFeatureNames(std::size_t time_index) const {
    return policy_->regressionFeatureNames(time_index);
}

double ExercisePolicyDecisionProblem::terminalValue(const dynamic_programming::State& state) const {
    return policy_->terminalValue(state, maturity_time_index_);
}

VanillaOptionExercisePolicy::VanillaOptionExercisePolicy(double strike, bool is_put, int basis_degree)
    : strike_(strike), is_put_(is_put), basis_degree_(std::max(basis_degree, 0)) {}

bool VanillaOptionExercisePolicy::canExercise(const dynamic_programming::State& state, std::size_t) const {
    return !state.market_variables.empty();
}

double VanillaOptionExercisePolicy::exerciseValue(const dynamic_programming::State& state, std::size_t) const {
    if (state.market_variables.empty()) {
        return 0.0;
    }
    const double spot = state.market_variables.front();
    return is_put_ ? std::max(strike_ - spot, 0.0) : std::max(spot - strike_, 0.0);
}

std::vector<double> VanillaOptionExercisePolicy::regressionFeatures(const dynamic_programming::State& state,
                                                                    std::size_t) const {
    const double spot = state.market_variables.empty() ? 0.0 : state.market_variables.front();
    std::vector<double> features{1.0};
    double power = 1.0;
    for (int degree = 1; degree <= basis_degree_; ++degree) {
        power *= spot;
        features.push_back(power);
    }
    return features;
}

std::vector<std::string> VanillaOptionExercisePolicy::regressionFeatureNames(std::size_t) const {
    std::vector<std::string> names{"1"};
    for (int degree = 1; degree <= basis_degree_; ++degree) {
        if (degree == 1) {
            names.push_back("spot");
        } else {
            names.push_back("spot^" + std::to_string(degree));
        }
    }
    return names;
}

} // namespace qrp::analytics::exercise
