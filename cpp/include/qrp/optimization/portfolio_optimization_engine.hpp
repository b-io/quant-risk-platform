#pragma once

#include <qrp/optimization/optimization_types.hpp>
#include <qrp/optimization/portfolio_optimization.hpp>
#include <qrp/optimization/models/risk_model.hpp>
#include <map>
#include <string>
#include <vector>
#include <memory>

namespace qrp::optimization {

/**
 * @brief High-level engine for portfolio optimization.
 * Translates portfolio concepts into OptimizationProblem and calls the solver.
 */
class PortfolioOptimizationEngine {
public:
    PortfolioOptimizationEngine(std::shared_ptr<OptimizationSolver> solver);

    /**
     * @brief Optimizes a portfolio based on returns, risk model, and constraints.
     */
    OptimizationResult optimize(
        const std::map<std::string, double>& current_weights,
        const std::map<std::string, double>& expected_returns,
        const std::shared_ptr<RiskModel>& risk_model,
        const std::vector<std::shared_ptr<OptimizationConstraint>>& constraints,
        const std::shared_ptr<OptimizationObjective>& objective,
        const SolverConfig& config = SolverConfig());

private:
    std::shared_ptr<OptimizationSolver> solver_;
};

} // namespace qrp::optimization
