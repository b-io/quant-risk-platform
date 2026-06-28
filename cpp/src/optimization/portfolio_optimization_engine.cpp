// Implements solver orchestration for portfolio optimization problems.

#include <qrp/optimization/portfolio_optimization_engine.hpp>
#include <set>
#include <stdexcept>
#include <utility>

namespace qrp::optimization {

PortfolioOptimizationEngine::PortfolioOptimizationEngine(std::shared_ptr<OptimizationSolver> solver)
    : solver_(std::move(solver)) {
    if (!solver_) {
        throw std::invalid_argument("Solver cannot be null");
    }
}

OptimizationResult PortfolioOptimizationEngine::optimize(
    const std::map<std::string, double>& current_weights,
    const std::map<std::string, double>& expected_returns,
    const std::shared_ptr<RiskModel>& risk_model,
    const std::vector<std::shared_ptr<OptimizationConstraint>>& constraints,
    const std::shared_ptr<OptimizationObjective>& objective,
    const SolverConfig& config) {

    OptimizationProblem problem;
    problem.name = "Portfolio Optimization";

    // 1. Identify all assets involved
    std::set<std::string> all_assets;
    for (const auto& entry : expected_returns) all_assets.insert(entry.first);
    for (const auto& entry : current_weights) all_assets.insert(entry.first);
    
    if (auto full_risk = std::dynamic_pointer_cast<FullCovarianceModel>(risk_model)) {
        for (const auto& asset_id : full_risk->asset_ids) all_assets.insert(asset_id);
    } else if (auto factor_risk = std::dynamic_pointer_cast<FactorRiskModel>(risk_model)) {
        for (const auto& asset_id : factor_risk->asset_ids) all_assets.insert(asset_id);
    }

    if (all_assets.empty()) {
        throw std::runtime_error("No assets provided for optimization");
    }

    // 2. Create variables for each asset weight
    for (const auto& asset_id : all_assets) {
        OptimizationVariable var;
        var.id = asset_id;
        problem.variables.push_back(var);
    }

    // 3. Add objectives
    if (objective) {
        if (auto mv = std::dynamic_pointer_cast<MeanVarianceObjective>(objective)) {
            if (mv->expected_returns.empty()) {
                mv->expected_returns = expected_returns;
            }
        } else if (auto mr = std::dynamic_pointer_cast<MaximizeReturnObjective>(objective)) {
            if (mr->expected_returns.empty()) {
                mr->expected_returns = expected_returns;
            }
        }
        problem.objectives.push_back(objective);
    }

    // 4. Add risk model
    problem.risk_model = risk_model;
    
    // 5. Add constraints
    problem.constraints = constraints;

    // 6. Solve
    if (!solver_->supports(problem)) {
        throw std::runtime_error("Solver '" + solver_->name() + "' does not support this optimization problem");
    }

    return solver_->solve(problem, config);
}

} // namespace qrp::optimization
