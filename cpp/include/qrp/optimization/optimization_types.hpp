#pragma once

// Defines solver-neutral optimization problem, result, capability, and adapter interfaces.

#include <nlohmann/json.hpp>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace qrp::optimization {

/**
 * @brief Forward declaration for risk-aware optimization models.
 */
class RiskModel;

/**
 * @brief Status of the optimization solver.
 */
enum class SolverStatus {
    Error,
    Infeasible,
    LimitReached, // Iteration or time limit
    Solved,
    Unbounded,
    Unknown
};

/**
 * @brief Result of an optimization run.
 */
struct OptimizationResult {
    /** @brief Solver termination status. */
    SolverStatus status = SolverStatus::Unknown;
    /** @brief Human-readable solver status or error message. */
    std::string message;
    /** @brief Final objective value reported by the solver. */
    double objective_value = 0.0;
    /** @brief Optimal decision variable values keyed by variable id. */
    std::map<std::string, double> optimal_values;

    /** @brief Solver wall-clock time in milliseconds. */
    double solve_time_ms = 0.0;
    /** @brief Dual values keyed by constraint id when provided by the solver. */
    std::map<std::string, double> dual_values;
    /** @brief Solver-specific diagnostic key-value pairs. */
    std::map<std::string, std::string> solver_specific_info;
};

/**
 * @brief Configuration for the solver.
 */
struct SolverConfig {
    /** @brief Requested solver backend name. */
    std::string solver_name;
    /** @brief Numerical feasibility and optimality tolerance. */
    double tolerance = 1e-6;
    /** @brief Maximum solver iterations. */
    int max_iterations = 1000;
    /** @brief Optional wall-clock time limit in seconds. */
    std::optional<double> time_limit_sec;
    /** @brief Whether to emit solver-native verbose logs. */
    bool verbose = false;
    /** @brief Additional backend-specific solver options. */
    std::map<std::string, std::string> custom_params;
};

/**
 * @brief Capabilities of an optimization solver.
 */
struct SolverCapabilities {
    /** @brief Whether quadratic objective terms are accepted. */
    bool supports_quadratic_objectives = false;
    /** @brief Whether linear equality/inequality constraints are accepted. */
    bool supports_linear_constraints = false;
    /** @brief Whether second-order cone constraints are accepted. */
    bool supports_second_order_cone_constraints = false;
    /** @brief Whether integer or binary variables are accepted. */
    bool supports_integer_variables = false;
    /** @brief Whether cardinality constraints are accepted directly. */
    bool supports_cardinality_constraints = false;
    /** @brief Whether expected-shortfall objectives or constraints are accepted. */
    bool supports_expected_shortfall = false;
    /** @brief Whether an initial solution can be supplied to the solver. */
    bool supports_warm_start = false;
    /** @brief Whether sparse matrix payloads are accepted efficiently. */
    bool supports_sparse_matrices = false;
};

/**
 * @brief Base class for an optimization variable.
 */
struct OptimizationVariable {
    /** @brief Stable variable identifier used in objectives, constraints, and results. */
    std::string id;
    /** @brief Lower variable bound, with -1e20 representing practical negative infinity. */
    double lower_bound = -1e20;
    /** @brief Upper variable bound, with 1e20 representing practical positive infinity. */
    double upper_bound = 1e20;
    /** @brief Whether the variable is constrained to integer values. */
    bool is_integer = false;
};

/**
 * @brief Base class for an optimization objective.
 */
class OptimizationObjective {
public:
    /**
     * @brief Allows deletion through the objective base type.
     */
    virtual ~OptimizationObjective() = default;

    /**
     * @brief Returns the stable objective type name used by solver adapters.
     */
    virtual std::string type() const = 0;
};

/**
 * @brief Base class for an optimization constraint.
 */
class OptimizationConstraint {
public:
    /**
     * @brief Allows deletion through the constraint base type.
     */
    virtual ~OptimizationConstraint() = default;

    /**
     * @brief Returns the stable constraint type name used by solver adapters.
     */
    virtual std::string type() const = 0;
};

/**
 * @brief Encapsulates a solver-independent optimization problem.
 */
struct OptimizationProblem {
    /** @brief Decision variables controlled by the optimizer. */
    std::vector<OptimizationVariable> variables;
    /** @brief Objective functions to minimize or maximize. */
    std::vector<std::shared_ptr<OptimizationObjective>> objectives;
    /** @brief Hard or soft constraints imposed on the solution. */
    std::vector<std::shared_ptr<OptimizationConstraint>> constraints;
    /** @brief Optional risk model used by risk-aware objectives and constraints. */
    std::shared_ptr<RiskModel> risk_model;
    /** @brief Scenario payloads consumed by scenario-aware optimization constraints. */
    std::vector<nlohmann::json> scenarios;
    /** @brief Human-readable optimization problem name. */
    std::string name;
};

/**
 * @brief Interface for an optimization solver.
 */
class OptimizationSolver {
public:
    /**
     * @brief Allows deletion through the solver base type.
     */
    virtual ~OptimizationSolver() = default;

    /**
     * @brief Solves a solver-independent optimization problem.
     */
    virtual OptimizationResult solve(const OptimizationProblem& problem, const SolverConfig& config) = 0;

    /**
     * @brief Returns declared solver capabilities.
     */
    virtual SolverCapabilities get_capabilities() const = 0;

    /**
     * @brief Returns whether the solver can handle a specific problem.
     */
    virtual bool supports(const OptimizationProblem& problem) const = 0;

    /**
     * @brief Returns the stable solver name used in diagnostics.
     */
    virtual std::string name() const = 0;
};

} // namespace qrp::optimization
