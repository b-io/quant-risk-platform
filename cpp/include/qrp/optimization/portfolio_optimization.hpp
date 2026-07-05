#pragma once

// Defines solver-independent portfolio optimization objectives and constraints.

#include <qrp/optimization/optimization_types.hpp>

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace qrp::optimization {

/**
 * @brief Mean-Variance objective: maximize(w^T * mu - 0.5 * gamma * w^T * Sigma * w)
 */
class MeanVarianceObjective : public OptimizationObjective {
public:
    /**
     * @brief Returns the solver-facing objective type identifier.
     */
    std::string type() const override { return "MeanVariance"; }

    std::map<std::string, double> expected_returns; // Asset id to expected return.
    double risk_aversion = 1.0;                     // Mean-variance risk-aversion gamma.
};

/**
 * @brief Minimum Variance objective: minimize(w^T * Sigma * w)
 */
class MinimumVarianceObjective : public OptimizationObjective {
public:
    /**
     * @brief Returns the solver-facing objective type identifier.
     */
    std::string type() const override { return "MinimumVariance"; }
};

/**
 * @brief Maximize Return objective: maximize(w^T * mu)
 */
class MaximizeReturnObjective : public OptimizationObjective {
public:
    /**
     * @brief Returns the solver-facing objective type identifier.
     */
    std::string type() const override { return "MaximizeReturn"; }

    std::map<std::string, double> expected_returns; // Asset id to expected return.
};

/**
 * @brief Tracking Error objective: minimize((w - w_bench)^T * Sigma * (w - w_bench))
 */
class TrackingErrorObjective : public OptimizationObjective {
public:
    /**
     * @brief Returns the solver-facing objective type identifier.
     */
    std::string type() const override { return "TrackingError"; }

    std::map<std::string, double> benchmark_weights; // Asset id to benchmark weight.
};

/**
 * @brief Sum of weights = total_weight (e.g. 1.0 for fully invested)
 */
class LinearEqualityConstraint : public OptimizationConstraint {
public:
    /**
     * @brief Returns the solver-facing constraint type identifier.
     */
    std::string type() const override { return "LinearEquality"; }

    std::map<std::string, double> coefficients; // Asset id to linear coefficient.
    double target_value = 0.0;                  // Required linear target value.
};

/**
 * @brief Linear Inequality: lower <= sum(coeffs * weights) <= upper
 */
class LinearInequalityConstraint : public OptimizationConstraint {
public:
    /**
     * @brief Returns the solver-facing constraint type identifier.
     */
    std::string type() const override { return "LinearInequality"; }

    std::map<std::string, double> coefficients; // Asset id to linear coefficient.
    std::optional<double> lower_bound;          // Optional lower bound on the linear expression.
    std::optional<double> upper_bound;          // Optional upper bound on the linear expression.
};

/**
 * @brief Turnover constraint: sum(|w_i - w_current_i|) <= max_turnover
 */
class TurnoverConstraint : public OptimizationConstraint {
public:
    /**
     * @brief Returns the solver-facing constraint type identifier.
     */
    std::string type() const override { return "Turnover"; }

    std::map<std::string, double> current_weights; // Asset id to current portfolio weight.
    double max_turnover = 1.0;                     // Maximum absolute weight turnover.
};

} // namespace qrp::optimization
