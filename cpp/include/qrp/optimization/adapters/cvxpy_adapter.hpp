#pragma once

// Declares the CVXPY-backed solver adapter and worker-process configuration.

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
    /**
     * @brief Creates an adapter that resolves the Python worker at solve time.
     */
    CvxpyAdapter();

    /**
     * @brief Serializes, submits, and parses a CVXPY/OSQP solve request.
     */
    OptimizationResult solve(const OptimizationProblem& problem, const SolverConfig& config) override;

    /**
     * @brief Returns the optimization features supported by the CVXPY worker.
     */
    SolverCapabilities get_capabilities() const override;

    /**
     * @brief Returns whether the adapter can solve the requested problem shape.
     */
    bool supports(const OptimizationProblem& problem) const override;

    /**
     * @brief Returns the stable adapter name.
     */
    std::string name() const override { return "CVXPY+OSQP"; }

private:
    /**
     * @brief Converts a solver-neutral problem into the worker JSON protocol.
     */
    nlohmann::json serialize_problem(const OptimizationProblem& problem, const SolverConfig& config);

    /**
     * @brief Converts the worker JSON response into a solver-neutral result.
     */
    OptimizationResult parse_result(const nlohmann::json& result_json);
};

} // namespace qrp::optimization
