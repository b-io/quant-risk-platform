#pragma once

#include <qrp/optimization/optimization_types.hpp>
#include <memory>

namespace qrp::optimization {

/**
 * @brief Adapter for CVXPY + OSQP backend.
 * This adapter translates the OptimizationProblem into a JSON request,
 * sends it to a Python worker, and parses the result.
 */
class CvxpyAdapter : public OptimizationSolver {
public:
    CvxpyAdapter();
    
    OptimizationResult solve(const OptimizationProblem& problem, const SolverConfig& config) override;
    SolverCapabilities get_capabilities() const override;
    bool supports(const OptimizationProblem& problem) const override;
    std::string name() const override { return "CVXPY+OSQP"; }

private:
    nlohmann::json serialize_problem(const OptimizationProblem& problem, const SolverConfig& config);
    OptimizationResult parse_result(const nlohmann::json& result_json);
};

} // namespace qrp::optimization
