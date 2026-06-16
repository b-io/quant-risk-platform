#include <qrp/optimization/adapters/cvxpy_adapter.hpp>
#include <qrp/optimization/portfolio_optimization.hpp>
#include <qrp/optimization/models/risk_model.hpp>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <array>

namespace qrp::optimization {

CvxpyAdapter::CvxpyAdapter() {}

SolverCapabilities CvxpyAdapter::get_capabilities() const {
    SolverCapabilities caps;
    caps.supports_quadratic_objectives = true;
    caps.supports_linear_constraints = true;
    caps.supports_sparse_matrices = true;
    caps.supports_warm_start = true;
    return caps;
}

bool CvxpyAdapter::supports(const OptimizationProblem& problem) const {
    // For now, support most convex problems.
    // In a real implementation, we would check if objectives/constraints are supported.
    return true;
}

OptimizationResult CvxpyAdapter::solve(const OptimizationProblem& problem, const SolverConfig& config) {
    nlohmann::json request = serialize_problem(problem, config);
    
    // Write request to temporary file
    std::string request_file = "opt_request.json";
    std::string response_file = "opt_response.json";
    
    {
        std::ofstream ofs(request_file);
        ofs << request.dump(4);
    }
    
    // Call Python worker
    // Using an absolute path to the Python worker script to ensure it can be found regardless of current directory.
    // In a production environment, this path would be configurable or relative to the executable.
    std::string command = "python D:/IT/DEV/C++/quant-risk-platform/python/qrp/optimization/cvxpy_worker.py " + request_file + " " + response_file;
    if (config.verbose) {
        std::cout << "Running command: " << command << std::endl;
    }
    
    int ret = std::system(command.c_str());
    if (ret != 0) {
        OptimizationResult error_res;
        error_res.status = SolverStatus::Error;
        error_res.message = "Failed to execute Python worker";
        return error_res;
    }
    
    // Read response
    std::ifstream ifs(response_file);
    if (!ifs.is_open()) {
        OptimizationResult error_res;
        error_res.status = SolverStatus::Error;
        error_res.message = "Failed to open response file from Python worker";
        return error_res;
    }
    
    nlohmann::json response;
    try {
        ifs >> response;
    } catch (const std::exception& e) {
        OptimizationResult error_res;
        error_res.status = SolverStatus::Error;
        error_res.message = "Failed to parse JSON response: " + std::string(e.what());
        return error_res;
    }
    
    return parse_result(response);
}

nlohmann::json CvxpyAdapter::serialize_problem(const OptimizationProblem& problem, const SolverConfig& config) {
    nlohmann::json j;
    j["name"] = problem.name;
    
    // Variables
    nlohmann::json vars = nlohmann::json::array();
    for (const auto& var : problem.variables) {
        nlohmann::json v;
        v["id"] = var.id;
        v["lb"] = var.lower_bound;
        v["ub"] = var.upper_bound;
        v["integer"] = var.is_integer;
        vars.push_back(v);
    }
    j["variables"] = vars;
    
    // Objectives
    nlohmann::json objs = nlohmann::json::array();
    for (const auto& obj : problem.objectives) {
        nlohmann::json o;
        o["type"] = obj->type();
        if (auto mv = std::dynamic_pointer_cast<MeanVarianceObjective>(obj)) {
            o["expected_returns"] = mv->expected_returns;
            o["risk_aversion"] = mv->risk_aversion;
        } else if (auto te = std::dynamic_pointer_cast<TrackingErrorObjective>(obj)) {
            o["benchmark_weights"] = te->benchmark_weights;
        } else if (auto mr = std::dynamic_pointer_cast<MaximizeReturnObjective>(obj)) {
            o["expected_returns"] = mr->expected_returns;
        }
        objs.push_back(o);
    }
    j["objectives"] = objs;
    
    // Constraints
    nlohmann::json cons = nlohmann::json::array();
    for (const auto& con : problem.constraints) {
        nlohmann::json c;
        c["type"] = con->type();
        if (auto le = std::dynamic_pointer_cast<LinearEqualityConstraint>(con)) {
            c["coefficients"] = le->coefficients;
            c["target"] = le->target_value;
        } else if (auto li = std::dynamic_pointer_cast<LinearInequalityConstraint>(con)) {
            c["coefficients"] = li->coefficients;
            if (li->lower_bound) c["lb"] = *li->lower_bound;
            if (li->upper_bound) c["ub"] = *li->upper_bound;
        } else if (auto to = std::dynamic_pointer_cast<TurnoverConstraint>(con)) {
            c["current_weights"] = to->current_weights;
            c["max_turnover"] = to->max_turnover;
        }
        cons.push_back(c);
    }
    j["constraints"] = cons;
    
    // Risk Model
    if (problem.risk_model) {
        nlohmann::json rm;
        rm["type"] = problem.risk_model->type();
        if (auto full = std::dynamic_pointer_cast<FullCovarianceModel>(problem.risk_model)) {
            rm["asset_ids"] = full->asset_ids;
            rm["covariance"] = full->covariance_matrix;
        } else if (auto factor = std::dynamic_pointer_cast<FactorRiskModel>(problem.risk_model)) {
            rm["asset_ids"] = factor->asset_ids;
            rm["factor_ids"] = factor->factor_ids;
            rm["exposures"] = factor->exposures;
            rm["factor_covariance"] = factor->factor_covariance;
            rm["specific_risk"] = factor->specific_risk;
        }
        j["risk_model"] = rm;
    }
    
    // Solver Config
    j["config"]["tolerance"] = config.tolerance;
    j["config"]["max_iterations"] = config.max_iterations;
    if (config.time_limit_sec) j["config"]["time_limit"] = *config.time_limit_sec;
    j["config"]["verbose"] = config.verbose;
    j["config"]["solver"] = "OSQP"; // Hardcoded for this backend
    
    return j;
}

OptimizationResult CvxpyAdapter::parse_result(const nlohmann::json& j) {
    OptimizationResult res;
    std::string status = j.at("status").get<std::string>();
    if (status == "optimal") res.status = SolverStatus::Solved;
    else if (status == "infeasible") res.status = SolverStatus::Infeasible;
    else if (status == "unbounded") res.status = SolverStatus::Unbounded;
    else res.status = SolverStatus::Error;
    
    if (j.contains("message")) res.message = j.at("message").get<std::string>();
    if (j.contains("objective_value")) res.objective_value = j.at("objective_value").get<double>();
    if (j.contains("optimal_values")) res.optimal_values = j.at("optimal_values").get<std::map<std::string, double>>();
    
    if (j.contains("solve_time_ms")) res.solve_time_ms = j.at("solve_time_ms").get<double>();
    
    return res;
}

} // namespace qrp::optimization
