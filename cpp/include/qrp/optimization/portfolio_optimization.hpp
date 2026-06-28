#pragma once

// Defines solver-independent portfolio optimization objectives and constraints.

#include <qrp/optimization/optimization_types.hpp>
#include <map>
#include <vector>
#include <string>

namespace qrp::optimization {

/**
 * @brief Mean-Variance objective: maximize(w^T * mu - 0.5 * gamma * w^T * Sigma * w)
 */
class MeanVarianceObjective : public OptimizationObjective {
public:
    std::string type() const override { return "MeanVariance"; }
    
    std::map<std::string, double> expected_returns; // Asset ID -> Expected Return
    double risk_aversion = 1.0;                      // gamma
};

/**
 * @brief Minimum Variance objective: minimize(w^T * Sigma * w)
 */
class MinimumVarianceObjective : public OptimizationObjective {
public:
    std::string type() const override { return "MinimumVariance"; }
};

/**
 * @brief Maximize Return objective: maximize(w^T * mu)
 */
class MaximizeReturnObjective : public OptimizationObjective {
public:
    std::string type() const override { return "MaximizeReturn"; }
    
    std::map<std::string, double> expected_returns;
};

/**
 * @brief Tracking Error objective: minimize((w - w_bench)^T * Sigma * (w - w_bench))
 */
class TrackingErrorObjective : public OptimizationObjective {
public:
    std::string type() const override { return "TrackingError"; }
    
    std::map<std::string, double> benchmark_weights;
};

/**
 * @brief Sum of weights = total_weight (e.g. 1.0 for fully invested)
 */
class LinearEqualityConstraint : public OptimizationConstraint {
public:
    std::string type() const override { return "LinearEquality"; }
    
    std::map<std::string, double> coefficients;
    double target_value = 0.0;
};

/**
 * @brief Linear Inequality: lower <= sum(coeffs * weights) <= upper
 */
class LinearInequalityConstraint : public OptimizationConstraint {
public:
    std::string type() const override { return "LinearInequality"; }
    
    std::map<std::string, double> coefficients;
    std::optional<double> lower_bound;
    std::optional<double> upper_bound;
};

/**
 * @brief Turnover constraint: sum(|w_i - w_current_i|) <= max_turnover
 */
class TurnoverConstraint : public OptimizationConstraint {
public:
    std::string type() const override { return "Turnover"; }
    
    std::map<std::string, double> current_weights;
    double max_turnover = 1.0;
};

} // namespace qrp::optimization
