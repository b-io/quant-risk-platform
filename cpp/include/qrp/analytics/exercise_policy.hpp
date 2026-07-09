#pragma once

// Declares reusable exercise-policy adapters for LSMC decision problems.

#include <qrp/analytics/dynamic_programming/decision_problem.hpp>

#include <memory>
#include <string>
#include <vector>

namespace qrp::analytics::exercise {

inline constexpr int kContinueActionId = 0;
inline constexpr int kExerciseActionId = 1;

/**
 * @brief Product-level exercise policy used by the reusable LSMC decision adapter.
 */
class ExercisePolicy {
public:
    /**
     * @brief Allows derived policies to be deleted through the base type.
     */
    virtual ~ExercisePolicy() = default;

    /**
     * @brief Returns whether exercise is allowed at the supplied state and time index.
     */
    virtual bool canExercise(const dynamic_programming::State& state, std::size_t time_index) const = 0;

    /**
     * @brief Returns immediate exercise value at the supplied state and time index.
     */
    virtual double exerciseValue(const dynamic_programming::State& state, std::size_t time_index) const = 0;

    /**
     * @brief Returns continuation-regression features at the supplied state and time index.
     */
    virtual std::vector<double> regressionFeatures(const dynamic_programming::State& state,
                                                   std::size_t time_index) const = 0;

    /**
     * @brief Returns stable names for continuation-regression features.
     */
    virtual std::vector<std::string> regressionFeatureNames(std::size_t time_index) const = 0;

    /**
     * @brief Returns terminal value at the final time index.
     */
    virtual double terminalValue(const dynamic_programming::State& state, std::size_t time_index) const;
};

/**
 * @brief Adapts an exercise policy to the generic dynamic-programming contract.
 */
class ExercisePolicyDecisionProblem final : public dynamic_programming::DecisionProblem {
public:
    /**
     * @brief Creates a decision problem from a product exercise policy.
     */
    ExercisePolicyDecisionProblem(std::shared_ptr<const ExercisePolicy> policy, std::size_t maturity_time_index);

    std::vector<dynamic_programming::Action> feasibleActions(const dynamic_programming::State& state,
                                                             std::size_t time_index) const override;

    double immediateCashflow(const dynamic_programming::State& state,
                             const dynamic_programming::Action& action,
                             std::size_t time_index) const override;

    dynamic_programming::State nextState(const dynamic_programming::State& state,
                                         const dynamic_programming::Action& action,
                                         const std::vector<double>& market_variables_next,
                                         std::size_t time_index) const override;

    bool isTerminalAction(const dynamic_programming::State& state,
                          const dynamic_programming::Action& action,
                          std::size_t time_index) const override;

    std::vector<double> regressionFeatures(const dynamic_programming::State& state,
                                           std::size_t time_index) const override;

    std::vector<std::string> regressionFeatureNames(std::size_t time_index) const override;

    double terminalValue(const dynamic_programming::State& state) const override;

private:
    std::shared_ptr<const ExercisePolicy> policy_;
    std::size_t maturity_time_index_;
};

/**
 * @brief Vanilla American/Bermudan option exercise policy over a one-dimensional spot state.
 */
class VanillaOptionExercisePolicy final : public ExercisePolicy {
public:
    /**
     * @brief Creates a vanilla option policy with polynomial continuation features.
     */
    VanillaOptionExercisePolicy(double strike, bool is_put, int basis_degree = 2);

    bool canExercise(const dynamic_programming::State& state, std::size_t time_index) const override;

    double exerciseValue(const dynamic_programming::State& state, std::size_t time_index) const override;

    std::vector<double> regressionFeatures(const dynamic_programming::State& state,
                                           std::size_t time_index) const override;

    std::vector<std::string> regressionFeatureNames(std::size_t time_index) const override;

private:
    double strike_;
    bool is_put_;
    int basis_degree_;
};

} // namespace qrp::analytics::exercise
