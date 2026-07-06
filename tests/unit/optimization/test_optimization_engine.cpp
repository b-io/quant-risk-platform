// Verifies solver-independent portfolio-optimization model construction and CVXPY adapter guards.

#include <qrp/optimization/adapters/cvxpy_adapter.hpp>
#include <qrp/optimization/models/risk_model.hpp>
#include <qrp/optimization/portfolio_optimization.hpp>
#include <qrp/optimization/portfolio_optimization_engine.hpp>
#include <qrp/util/logger.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

using namespace qrp::optimization;

class RecordingSolver : public OptimizationSolver {
public:
    OptimizationProblem last_problem;
    SolverConfig last_config;
    bool called = false;
    bool supports_problem = true;

    OptimizationResult solve(const OptimizationProblem& problem, const SolverConfig& config) override {
        called = true;
        last_problem = problem;
        last_config = config;

        OptimizationResult result;
        result.status = SolverStatus::Solved;
        const double equal_weight =
            problem.variables.empty() ? 0.0 : 1.0 / static_cast<double>(problem.variables.size());
        for (const auto& variable : problem.variables) {
            result.optimal_values[variable.id] = equal_weight;
        }
        return result;
    }

    SolverCapabilities get_capabilities() const override {
        SolverCapabilities caps;
        caps.supports_linear_constraints = true;
        caps.supports_quadratic_objectives = true;
        return caps;
    }

    bool supports(const OptimizationProblem&) const override {
        return supports_problem;
    }
    std::string name() const override {
        return "RecordingSolver";
    }
};

std::shared_ptr<FullCovarianceModel> make_two_asset_risk_model() {
    auto risk_model = std::make_shared<FullCovarianceModel>();
    risk_model->asset_ids = {"AAPL", "MSFT"};
    risk_model->covariance_matrix = {{0.04, 0.01}, {0.01, 0.09}};
    return risk_model;
}

std::shared_ptr<FactorRiskModel> make_two_asset_factor_risk_model() {
    auto risk_model = std::make_shared<FactorRiskModel>();
    risk_model->asset_ids = {"AAPL", "MSFT"};
    risk_model->factor_ids = {"EQUITY"};
    risk_model->exposures = {{1.0}, {0.8}};
    risk_model->factor_covariance = {{0.04}};
    risk_model->specific_risk = {{"AAPL", 0.01}, {"MSFT", 0.02}};
    return risk_model;
}

OptimizationProblem make_supported_mean_variance_problem() {
    OptimizationProblem problem;
    problem.name = "Supported mean variance";
    problem.variables = {{"AAPL"}, {"MSFT"}};
    problem.risk_model = make_two_asset_risk_model();

    auto objective = std::make_shared<MeanVarianceObjective>();
    objective->expected_returns = {{"AAPL", 0.10}, {"MSFT", 0.08}};
    objective->risk_aversion = 1.0;
    problem.objectives.push_back(objective);

    auto budget = std::make_shared<LinearEqualityConstraint>();
    budget->coefficients = {{"AAPL", 1.0}, {"MSFT", 1.0}};
    budget->target_value = 1.0;
    problem.constraints.push_back(budget);

    return problem;
}

OptimizationProblem make_supported_factor_problem() {
    OptimizationProblem problem;
    problem.name = "Supported factor problem";
    problem.variables = {{"AAPL", 0.0, 1.0}, {"MSFT", 0.0, 1.0}};
    problem.risk_model = make_two_asset_factor_risk_model();

    auto tracking_error = std::make_shared<TrackingErrorObjective>();
    tracking_error->benchmark_weights = {{"AAPL", 0.55}, {"MSFT", 0.45}};
    problem.objectives.push_back(tracking_error);

    auto maximize_return = std::make_shared<MaximizeReturnObjective>();
    maximize_return->expected_returns = {{"AAPL", 0.10}, {"MSFT", 0.08}};
    problem.objectives.push_back(maximize_return);

    auto exposure_cap = std::make_shared<LinearInequalityConstraint>();
    exposure_cap->coefficients = {{"AAPL", 1.0}, {"MSFT", 1.0}};
    exposure_cap->lower_bound = 0.80;
    exposure_cap->upper_bound = 1.00;
    problem.constraints.push_back(exposure_cap);

    auto turnover = std::make_shared<TurnoverConstraint>();
    turnover->current_weights = {{"AAPL", 0.50}, {"MSFT", 0.50}};
    turnover->max_turnover = 0.20;
    problem.constraints.push_back(turnover);

    return problem;
}

std::optional<std::string> test_python_executable() {
#ifdef QRP_TEST_PYTHON_EXECUTABLE
    return std::string(QRP_TEST_PYTHON_EXECUTABLE);
#else
    const char* python = std::getenv("QRP_PYTHON_EXECUTABLE");
    if (python != nullptr && *python != '\0') {
        return std::string(python);
    }
    return std::nullopt;
#endif
}

void set_environment_variable(const char* name, const std::string& value) {
#ifdef _MSC_VER
    _putenv_s(name, value.c_str());
#else
    setenv(name, value.c_str(), 1);
#endif
}

class TemporaryWorkerScript {
public:
    explicit TemporaryWorkerScript(std::string body) {
        const auto suffix = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        path_ = std::filesystem::current_path() / ("qrp_cvxpy_worker_test_" + std::to_string(suffix) + ".py");
        std::ofstream script(path_);
        script << body;
    }

    ~TemporaryWorkerScript() {
        std::error_code ec;
        std::filesystem::remove(path_, ec);
    }

    const std::filesystem::path& path() const {
        return path_;
    }

private:
    std::filesystem::path path_;
};

SolverConfig
make_worker_config(const std::filesystem::path& worker_path, const std::string& python, const std::string& status) {
    SolverConfig config;
    config.custom_params["cvxpy_worker_path"] = worker_path.filename().string();
    config.custom_params["python_executable"] = python;
    config.solver_name = status;
    config.tolerance = 1.0e-8;
    config.max_iterations = 250;
    config.time_limit_sec = 3.5;
    return config;
}

TEST(PortfolioOptimizationTest, BuildsMeanVarianceProblemForSolver) {
    auto solver = std::make_shared<RecordingSolver>();
    PortfolioOptimizationEngine engine(solver);

    std::map<std::string, double> returns = {{"AAPL", 0.15}, {"GOOG", 0.10}, {"MSFT", 0.12}};

    auto risk_model = std::make_shared<FullCovarianceModel>();
    risk_model->asset_ids = {"AAPL", "MSFT", "GOOG"};
    risk_model->covariance_matrix = {{0.04, 0.02, 0.01}, {0.02, 0.03, 0.015}, {0.01, 0.015, 0.025}};

    auto objective = std::make_shared<MeanVarianceObjective>();
    objective->risk_aversion = 2.0;

    auto budget = std::make_shared<LinearEqualityConstraint>();
    budget->coefficients = {{"AAPL", 1.0}, {"GOOG", 1.0}, {"MSFT", 1.0}};
    budget->target_value = 1.0;

    auto result = engine.optimize({}, returns, risk_model, {budget}, objective);

    ASSERT_TRUE(solver->called);
    EXPECT_EQ(result.status, SolverStatus::Solved);
    EXPECT_EQ(solver->last_problem.variables.size(), 3U);
    EXPECT_EQ(solver->last_problem.constraints.size(), 1U);
    EXPECT_EQ(solver->last_problem.objectives.size(), 1U);
    EXPECT_EQ(solver->last_problem.risk_model, risk_model);

    auto mv_objective = std::dynamic_pointer_cast<MeanVarianceObjective>(solver->last_problem.objectives.front());
    ASSERT_NE(mv_objective, nullptr);
    EXPECT_EQ(mv_objective->expected_returns, returns);
    EXPECT_NEAR(result.optimal_values["AAPL"] + result.optimal_values["GOOG"] + result.optimal_values["MSFT"],
                1.0,
                1e-12);
}

TEST(PortfolioOptimizationTest, IncludesCurrentWeightsAndFactorRiskAssets) {
    auto solver = std::make_shared<RecordingSolver>();
    PortfolioOptimizationEngine engine(solver);

    std::map<std::string, double> current_weights = {{"BOND", 0.25}};
    std::map<std::string, double> returns = {{"AAPL", 0.1}};

    auto risk_model = std::make_shared<FactorRiskModel>();
    risk_model->asset_ids = {"TSLA"};
    risk_model->factor_ids = {"Market"};
    risk_model->exposures = {{2.0}};
    risk_model->factor_covariance = {{0.04}};
    risk_model->specific_risk = {{"TSLA", 0.05}};

    auto objective = std::make_shared<MaximizeReturnObjective>();
    auto result = engine.optimize(current_weights, returns, risk_model, {}, objective);

    ASSERT_TRUE(solver->called);
    EXPECT_EQ(result.status, SolverStatus::Solved);
    EXPECT_TRUE(result.optimal_values.contains("AAPL"));
    EXPECT_TRUE(result.optimal_values.contains("BOND"));
    EXPECT_TRUE(result.optimal_values.contains("TSLA"));
    EXPECT_EQ(solver->last_problem.variables.size(), 3U);

    auto return_objective = std::dynamic_pointer_cast<MaximizeReturnObjective>(solver->last_problem.objectives.front());
    ASSERT_NE(return_objective, nullptr);
    EXPECT_EQ(return_objective->expected_returns, returns);
}

TEST(PortfolioOptimizationTest, PreservesExplicitObjectiveReturnsAndPassesSolverConfig) {
    auto solver = std::make_shared<RecordingSolver>();
    PortfolioOptimizationEngine engine(solver);

    std::map<std::string, double> returns = {{"AAPL", 0.15}, {"MSFT", 0.12}, {"TSLA", 0.30}};
    auto objective = std::make_shared<MeanVarianceObjective>();
    objective->expected_returns = {{"AAPL", 0.05}, {"MSFT", 0.04}};
    objective->risk_aversion = 3.0;

    SolverConfig config;
    config.custom_params["warm_start"] = "false";
    config.max_iterations = 77;
    config.solver_name = "unit-solver";
    config.time_limit_sec = 2.5;
    config.tolerance = 1.0e-9;
    config.verbose = true;

    const auto result = engine.optimize({{"AAPL", 0.60}}, returns, make_two_asset_risk_model(), {}, objective, config);

    ASSERT_TRUE(solver->called);
    EXPECT_EQ(result.status, SolverStatus::Solved);
    EXPECT_TRUE(result.optimal_values.contains("AAPL"));
    EXPECT_TRUE(result.optimal_values.contains("MSFT"));
    EXPECT_TRUE(result.optimal_values.contains("TSLA"));

    auto mean_variance = std::dynamic_pointer_cast<MeanVarianceObjective>(solver->last_problem.objectives.front());
    ASSERT_NE(mean_variance, nullptr);
    EXPECT_EQ(mean_variance->expected_returns, objective->expected_returns);
    EXPECT_DOUBLE_EQ(mean_variance->risk_aversion, 3.0);
    EXPECT_EQ(solver->last_config.custom_params.at("warm_start"), "false");
    EXPECT_EQ(solver->last_config.max_iterations, 77);
    EXPECT_EQ(solver->last_config.solver_name, "unit-solver");
    ASSERT_TRUE(solver->last_config.time_limit_sec.has_value());
    EXPECT_DOUBLE_EQ(*solver->last_config.time_limit_sec, 2.5);
    EXPECT_DOUBLE_EQ(solver->last_config.tolerance, 1.0e-9);
    EXPECT_TRUE(solver->last_config.verbose);
}

TEST(PortfolioOptimizationTest, BuildsConstraintOnlyProblemFromCurrentWeights) {
    auto solver = std::make_shared<RecordingSolver>();
    PortfolioOptimizationEngine engine(solver);

    const auto result = engine.optimize({{"CASH", 1.0}}, {}, nullptr, {}, nullptr);

    ASSERT_TRUE(solver->called);
    EXPECT_EQ(result.status, SolverStatus::Solved);
    ASSERT_EQ(solver->last_problem.variables.size(), 1U);
    EXPECT_EQ(solver->last_problem.variables.front().id, "CASH");
    EXPECT_TRUE(solver->last_problem.objectives.empty());
    EXPECT_EQ(solver->last_problem.risk_model, nullptr);
}

TEST(PortfolioOptimizationTest, RejectsNullSolver) {
    EXPECT_THROW(PortfolioOptimizationEngine(nullptr), std::invalid_argument);
}

TEST(PortfolioOptimizationTest, RejectsEmptyProblem) {
    auto solver = std::make_shared<RecordingSolver>();
    PortfolioOptimizationEngine engine(solver);

    EXPECT_THROW(engine.optimize({}, {}, nullptr, {}, nullptr), std::runtime_error);
    EXPECT_FALSE(solver->called);
}

TEST(PortfolioOptimizationTest, RejectsUnsupportedProblemBeforeSolve) {
    auto solver = std::make_shared<RecordingSolver>();
    solver->supports_problem = false;
    PortfolioOptimizationEngine engine(solver);

    std::map<std::string, double> returns = {{"AAPL", 0.10}};
    auto objective = std::make_shared<MaximizeReturnObjective>();
    objective->expected_returns = returns;

    EXPECT_THROW(engine.optimize({}, returns, nullptr, {}, objective), std::runtime_error);
    EXPECT_FALSE(solver->called);
}

TEST(CvxpyAdapterTest, SupportsValidMeanVarianceProblem) {
    CvxpyAdapter solver;

    EXPECT_TRUE(solver.supports(make_supported_mean_variance_problem()));
}

TEST(CvxpyAdapterTest, ReportsCapabilitiesAndName) {
    CvxpyAdapter solver;

    const auto capabilities = solver.get_capabilities();

    EXPECT_EQ(solver.name(), "CVXPY+OSQP");
    EXPECT_TRUE(capabilities.supports_quadratic_objectives);
    EXPECT_TRUE(capabilities.supports_linear_constraints);
    EXPECT_FALSE(capabilities.supports_integer_variables);
}

TEST(CvxpyAdapterTest, SupportsReturnOnlyProblemWithoutRiskModel) {
    CvxpyAdapter solver;
    OptimizationProblem problem;
    problem.variables = {{"AAPL"}, {"MSFT"}};

    auto objective = std::make_shared<MaximizeReturnObjective>();
    objective->expected_returns = {{"AAPL", 0.10}, {"MSFT", 0.08}};
    problem.objectives.push_back(objective);

    EXPECT_TRUE(solver.supports(problem));
}

TEST(CvxpyAdapterTest, RejectsIntegerVariables) {
    CvxpyAdapter solver;
    auto problem = make_supported_mean_variance_problem();
    problem.variables.front().is_integer = true;

    EXPECT_FALSE(solver.supports(problem));
}

TEST(CvxpyAdapterTest, RejectsUnknownConstraintAssets) {
    CvxpyAdapter solver;
    auto problem = make_supported_mean_variance_problem();
    auto constraint = std::dynamic_pointer_cast<LinearEqualityConstraint>(problem.constraints.front());
    ASSERT_NE(constraint, nullptr);
    constraint->coefficients["UNKNOWN"] = 1.0;

    EXPECT_FALSE(solver.supports(problem));
}

TEST(CvxpyAdapterTest, RejectsInvalidFullCovarianceMatrix) {
    CvxpyAdapter solver;
    auto problem = make_supported_mean_variance_problem();
    auto risk_model = std::dynamic_pointer_cast<FullCovarianceModel>(problem.risk_model);
    ASSERT_NE(risk_model, nullptr);
    risk_model->covariance_matrix = {{0.04, 0.03}, {0.01, 0.09}};

    EXPECT_FALSE(solver.supports(problem));
}

TEST(CvxpyAdapterTest, RejectsInvalidProblemShapes) {
    CvxpyAdapter solver;
    auto problem = make_supported_mean_variance_problem();

    auto invalid = problem;
    invalid.variables.clear();
    EXPECT_FALSE(solver.supports(invalid));

    invalid = problem;
    invalid.variables[1].id = invalid.variables[0].id;
    EXPECT_FALSE(solver.supports(invalid));

    invalid = problem;
    invalid.variables[0].lower_bound = 2.0;
    invalid.variables[0].upper_bound = 1.0;
    EXPECT_FALSE(solver.supports(invalid));

    invalid = problem;
    invalid.objectives.clear();
    EXPECT_FALSE(solver.supports(invalid));

    invalid = problem;
    invalid.scenarios.push_back({{"scenario", "not_supported"}});
    EXPECT_FALSE(solver.supports(invalid));

    invalid = problem;
    invalid.objectives.front().reset();
    EXPECT_FALSE(solver.supports(invalid));

    invalid = problem;
    auto mean_variance = std::dynamic_pointer_cast<MeanVarianceObjective>(invalid.objectives.front());
    ASSERT_NE(mean_variance, nullptr);
    mean_variance->risk_aversion = -1.0;
    EXPECT_FALSE(solver.supports(invalid));

    invalid = problem;
    mean_variance = std::dynamic_pointer_cast<MeanVarianceObjective>(invalid.objectives.front());
    ASSERT_NE(mean_variance, nullptr);
    mean_variance->expected_returns.clear();
    EXPECT_FALSE(solver.supports(invalid));

    invalid = make_supported_factor_problem();
    auto tracking_error = std::dynamic_pointer_cast<TrackingErrorObjective>(invalid.objectives.front());
    ASSERT_NE(tracking_error, nullptr);
    tracking_error->benchmark_weights["UNKNOWN"] = 0.1;
    EXPECT_FALSE(solver.supports(invalid));

    invalid = make_supported_factor_problem();
    auto inequality = std::dynamic_pointer_cast<LinearInequalityConstraint>(invalid.constraints.front());
    ASSERT_NE(inequality, nullptr);
    inequality->lower_bound.reset();
    inequality->upper_bound.reset();
    EXPECT_FALSE(solver.supports(invalid));

    invalid = make_supported_factor_problem();
    inequality = std::dynamic_pointer_cast<LinearInequalityConstraint>(invalid.constraints.front());
    ASSERT_NE(inequality, nullptr);
    inequality->lower_bound = 1.1;
    inequality->upper_bound = 1.0;
    EXPECT_FALSE(solver.supports(invalid));

    invalid = make_supported_factor_problem();
    auto turnover = std::dynamic_pointer_cast<TurnoverConstraint>(invalid.constraints.back());
    ASSERT_NE(turnover, nullptr);
    turnover->max_turnover = -0.1;
    EXPECT_FALSE(solver.supports(invalid));

    invalid = make_supported_factor_problem();
    auto factor = std::dynamic_pointer_cast<FactorRiskModel>(invalid.risk_model);
    ASSERT_NE(factor, nullptr);
    factor->exposures = {{1.0}};
    EXPECT_FALSE(solver.supports(invalid));

    invalid = make_supported_factor_problem();
    factor = std::dynamic_pointer_cast<FactorRiskModel>(invalid.risk_model);
    ASSERT_NE(factor, nullptr);
    factor->specific_risk["UNKNOWN"] = 0.01;
    EXPECT_FALSE(solver.supports(invalid));

    invalid = make_supported_factor_problem();
    factor = std::dynamic_pointer_cast<FactorRiskModel>(invalid.risk_model);
    ASSERT_NE(factor, nullptr);
    factor->specific_risk["AAPL"] = -0.01;
    EXPECT_FALSE(solver.supports(invalid));
}

TEST(CvxpyAdapterTest, SupportsTrackingErrorProblem) {
    CvxpyAdapter solver;
    auto problem = make_supported_mean_variance_problem();
    problem.objectives.clear();

    auto objective = std::make_shared<TrackingErrorObjective>();
    objective->benchmark_weights = {{"AAPL", 0.60}, {"MSFT", 0.40}};
    problem.objectives.push_back(objective);

    EXPECT_TRUE(solver.supports(problem));
}

TEST(CvxpyAdapterTest, RejectsMinimumVarianceWithoutRiskModel) {
    CvxpyAdapter solver;
    auto problem = make_supported_mean_variance_problem();
    problem.risk_model.reset();
    problem.objectives.clear();
    problem.objectives.push_back(std::make_shared<MinimumVarianceObjective>());

    EXPECT_FALSE(solver.supports(problem));
}

TEST(CvxpyAdapterTest, SolvesThroughWorkerAndMapsStatuses) {
    const auto python = test_python_executable();
    if (!python) {
        GTEST_SKIP() << "Python executable was not configured for worker protocol tests.";
    }

    TemporaryWorkerScript worker(R"PY(
import json
import sys

with open(sys.argv[1], encoding="utf-8") as fh:
    request = json.load(fh)

status = request["config"]["solver"]
if status == "exit_7":
    sys.exit(7)
if status == "bad_json":
    with open(sys.argv[2], "w", encoding="utf-8") as fh:
        fh.write("{not json")
    sys.exit(0)
if status == "missing_response":
    sys.exit(0)

response = {
    "status": status,
    "message": "worker status: " + status,
    "objective_value": 1.25,
    "optimal_values": {"AAPL": 0.4, "MSFT": 0.6},
    "solve_time_ms": 12.5,
}

with open(sys.argv[2], "w", encoding="utf-8") as fh:
    json.dump(response, fh)
)PY");

    CvxpyAdapter solver;
    const auto problem = make_supported_factor_problem();

    auto config = make_worker_config(worker.path(), *python, "optimal_inaccurate");
    config.verbose = true;
    auto result = solver.solve(problem, config);
    EXPECT_EQ(result.status, SolverStatus::Solved);
    EXPECT_EQ(result.message, "worker status: optimal_inaccurate");
    EXPECT_DOUBLE_EQ(result.objective_value, 1.25);
    EXPECT_DOUBLE_EQ(result.optimal_values.at("AAPL"), 0.4);
    EXPECT_DOUBLE_EQ(result.solve_time_ms, 12.5);

    config = make_worker_config(worker.path(), *python, "infeasible_inaccurate");
    EXPECT_EQ(solver.solve(problem, config).status, SolverStatus::Infeasible);

    config = make_worker_config(worker.path(), *python, "unbounded_inaccurate");
    EXPECT_EQ(solver.solve(problem, config).status, SolverStatus::Unbounded);

    config = make_worker_config(worker.path(), *python, "user_limit");
    EXPECT_EQ(solver.solve(problem, config).status, SolverStatus::LimitReached);

    config = make_worker_config(worker.path(), *python, "unexpected_status");
    EXPECT_EQ(solver.solve(problem, config).status, SolverStatus::Error);
}

TEST(CvxpyAdapterTest, WorkerProtocolFailuresReturnErrors) {
    const auto python = test_python_executable();
    if (!python) {
        GTEST_SKIP() << "Python executable was not configured for worker protocol tests.";
    }

    TemporaryWorkerScript worker(R"PY(
import json
import sys

with open(sys.argv[1], encoding="utf-8") as fh:
    request = json.load(fh)

mode = request["config"]["solver"]
if mode == "exit_7":
    sys.exit(7)
if mode == "bad_json":
    with open(sys.argv[2], "w", encoding="utf-8") as fh:
        fh.write("{not json")
    sys.exit(0)
if mode == "missing_response":
    sys.exit(0)
)PY");

    CvxpyAdapter solver;
    const auto problem = make_supported_mean_variance_problem();

    auto config = make_worker_config(worker.path(), *python, "exit_7");
    auto result = solver.solve(problem, config);
    EXPECT_EQ(result.status, SolverStatus::Error);
    EXPECT_NE(result.message.find("exit code 7"), std::string::npos);

    config = make_worker_config(worker.path(), *python, "bad_json");
    result = solver.solve(problem, config);
    EXPECT_EQ(result.status, SolverStatus::Error);
    EXPECT_NE(result.message.find("Failed to parse JSON response"), std::string::npos);

    config = make_worker_config(worker.path(), *python, "missing_response");
    result = solver.solve(problem, config);
    EXPECT_EQ(result.status, SolverStatus::Error);
    EXPECT_NE(result.message.find("Failed to open response file"), std::string::npos);
}

TEST(CvxpyAdapterTest, SolvesWithWorkerAndPythonFromEnvironment) {
    const auto python = test_python_executable();
    if (!python) {
        GTEST_SKIP() << "Python executable was not configured for worker protocol tests.";
    }

    TemporaryWorkerScript worker(R"PY(
import json
import sys

with open(sys.argv[1], encoding="utf-8") as fh:
    request = json.load(fh)

with open(sys.argv[2], "w", encoding="utf-8") as fh:
    json.dump({
        "status": "optimal",
        "message": request["name"],
        "objective_value": 0.5,
        "optimal_values": {"AAPL": 1.0},
    }, fh)
)PY");

    set_environment_variable("QRP_CVXPY_WORKER", worker.path().string());
    set_environment_variable("QRP_PYTHON_EXECUTABLE", *python);

    OptimizationProblem problem;
    problem.name = "environment worker";
    problem.variables = {{"AAPL"}};
    auto objective = std::make_shared<MaximizeReturnObjective>();
    objective->expected_returns = {{"AAPL", 0.10}};
    problem.objectives.push_back(objective);

    CvxpyAdapter solver;
    const auto result = solver.solve(problem, SolverConfig{});

    EXPECT_EQ(result.status, SolverStatus::Solved);
    EXPECT_EQ(result.message, "environment worker");
    EXPECT_DOUBLE_EQ(result.optimal_values.at("AAPL"), 1.0);
}

TEST(CvxpyAdapterTest, DiscoversWorkerAndReportsProcessStartFailure) {
    set_environment_variable("QRP_CVXPY_WORKER", "");
    set_environment_variable("QRP_PYTHON_EXECUTABLE", "");

    SolverConfig config;
    config.custom_params["python_executable"] = "missing python executable for qrp tests.exe";

    CvxpyAdapter solver;
    const auto result = solver.solve(make_supported_mean_variance_problem(), config);

    EXPECT_EQ(result.status, SolverStatus::Error);
    EXPECT_TRUE(result.message.find("Failed to start CVXPY worker") != std::string::npos ||
                result.message.find("exit code 127") != std::string::npos)
        << result.message;
}

TEST(CvxpyAdapterTest, MissingWorkerPathReturnsErrorWithoutStartingPython) {
    CvxpyAdapter solver;
    auto problem = make_supported_mean_variance_problem();
    SolverConfig config;
    const auto missing_worker = std::filesystem::temp_directory_path() / "qrp_missing_cvxpy_worker_for_test.py";
    ASSERT_FALSE(std::filesystem::exists(missing_worker));
    config.custom_params["cvxpy_worker_path"] = missing_worker.string();

    auto result = solver.solve(problem, config);

    EXPECT_EQ(result.status, SolverStatus::Error);
    EXPECT_NE(result.message.find("CVXPY worker not found"), std::string::npos);
}

TEST(LoggerTest, InitializesSharedLoggerFromEnvironment) {
    set_environment_variable("QRP_LOG_LEVEL", "debug");

    auto logger = qrp::util::Logger::get();

    ASSERT_NE(logger, nullptr);
    EXPECT_EQ(logger->name(), "qrp");
    EXPECT_EQ(logger->level(), spdlog::level::debug);

    qrp::util::Logger::initialize();
    EXPECT_EQ(qrp::util::Logger::get(), logger);
}
