#pragma once

// Declares the high-level portfolio optimization engine and solver injection point.

#include <qrp/optimization/models/risk_model.hpp>
#include <qrp/optimization/optimization_types.hpp>
#include <qrp/optimization/portfolio_optimization.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace qrp::optimization {

/**
 * @brief High-level engine for portfolio optimization.
 * Translates portfolio concepts into OptimizationProblem and calls the solver.
 */
class PortfolioOptimizationEngine {
public:
    /**
     * @brief Creates an engine around an injected optimization solver.
     */
    PortfolioOptimizationEngine(std::shared_ptr<OptimizationSolver> solver);

    /**
     * @brief Optimizes a portfolio based on returns, risk model, and constraints.
     */
    OptimizationResult optimize(const std::map<std::string, double>& current_weights,
                                const std::map<std::string, double>& expected_returns,
                                const std::shared_ptr<RiskModel>& risk_model,
                                const std::vector<std::shared_ptr<OptimizationConstraint>>& constraints,
                                const std::shared_ptr<OptimizationObjective>& objective,
                                const SolverConfig& config = SolverConfig());

private:
    std::shared_ptr<OptimizationSolver> solver_; // Solver adapter used for all optimization requests.
};

} // namespace qrp::optimization
