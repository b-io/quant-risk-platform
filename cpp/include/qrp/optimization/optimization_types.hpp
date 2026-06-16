#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>

#include <nlohmann/json.hpp>

namespace qrp::optimization {

class RiskModel; // Forward declaration

/**
 * @brief Status of the optimization solver.
 */
enum class SolverStatus {
    Solved,
    Infeasible,
    Unbounded,
    LimitReached, // Iteration or time limit
    Error,
    Unknown
};

/**
 * @brief Result of an optimization run.
 */
struct OptimizationResult {
    SolverStatus status = SolverStatus::Unknown;
    std::string message;
    double objective_value = 0.0;
    std::map<std::string, double> optimal_values; // Variable ID -> Value
    
    // Diagnostics
    double solve_time_ms = 0.0;
    std::map<std::string, double> dual_values;
    std::map<std::string, std::string> solver_specific_info;
};

/**
 * @brief Configuration for the solver.
 */
struct SolverConfig {
    std::string solver_name;
    double tolerance = 1e-6;
    int max_iterations = 1000;
    std::optional<double> time_limit_sec;
    bool verbose = false;
    std::map<std::string, std::string> custom_params;
};

/**
 * @brief Capabilities of an optimization solver.
 */
struct SolverCapabilities {
    bool supports_quadratic_objectives = false;
    bool supports_linear_constraints = false;
    bool supports_second_order_cone_constraints = false;
    bool supports_integer_variables = false;
    bool supports_cardinality_constraints = false;
    bool supports_expected_shortfall = false;
    bool supports_warm_start = false;
    bool supports_sparse_matrices = false;
};

/**
 * @brief Base class for an optimization variable.
 */
struct OptimizationVariable {
    std::string id;
    double lower_bound = -1e20; // Effectively -inf
    double upper_bound = 1e20;  // Effectively inf
    bool is_integer = false;
};

/**
 * @brief Base class for an optimization objective.
 */
class OptimizationObjective {
public:
    virtual ~OptimizationObjective() = default;
    virtual std::string type() const = 0;
};

/**
 * @brief Base class for an optimization constraint.
 */
class OptimizationConstraint {
public:
    virtual ~OptimizationConstraint() = default;
    virtual std::string type() const = 0;
};

/**
 * @brief Encapsulates a solver-independent optimization problem.
 */
struct OptimizationProblem {
    std::vector<OptimizationVariable> variables;
    std::vector<std::shared_ptr<OptimizationObjective>> objectives;
    std::vector<std::shared_ptr<OptimizationConstraint>> constraints;
    std::shared_ptr<RiskModel> risk_model;
    std::vector<nlohmann::json> scenarios; // For future scenario constraints
    std::string name;
};

/**
 * @brief Interface for an optimization solver.
 */
class OptimizationSolver {
public:
    virtual ~OptimizationSolver() = default;
    
    virtual OptimizationResult solve(const OptimizationProblem& problem, const SolverConfig& config) = 0;
    virtual SolverCapabilities get_capabilities() const = 0;
    virtual bool supports(const OptimizationProblem& problem) const = 0;
    virtual std::string name() const = 0;
};

} // namespace qrp::optimization
