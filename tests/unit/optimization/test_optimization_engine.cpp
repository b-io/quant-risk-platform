// Verifies solver-independent portfolio-optimization model construction and CVXPY adapter guards.

#include <qrp/optimization/adapters/cvxpy_adapter.hpp>
#include <qrp/optimization/models/risk_model.hpp>
#include <qrp/optimization/portfolio_optimization.hpp>
#include <qrp/optimization/portfolio_optimization_engine.hpp>

#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <stdexcept>
#include <string>

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
        const double equal_weight = problem.variables.empty() ? 0.0 : 1.0 / static_cast<double>(problem.variables.size());
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

    bool supports(const OptimizationProblem&) const override { return supports_problem; }
    std::string name() const override { return "RecordingSolver"; }
};

std::shared_ptr<FullCovarianceModel> make_two_asset_risk_model() {
    auto risk_model = std::make_shared<FullCovarianceModel>();
    risk_model->asset_ids = {"AAPL", "MSFT"};
    risk_model->covariance_matrix = {
        {0.04, 0.01},
        {0.01, 0.09}
    };
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

TEST(PortfolioOptimizationTest, BuildsMeanVarianceProblemForSolver) {
    auto solver = std::make_shared<RecordingSolver>();
    PortfolioOptimizationEngine engine(solver);

    std::map<std::string, double> returns = {
        {"AAPL", 0.15},
        {"GOOG", 0.10},
        {"MSFT", 0.12}
    };

    auto risk_model = std::make_shared<FullCovarianceModel>();
    risk_model->asset_ids = {"AAPL", "MSFT", "GOOG"};
    risk_model->covariance_matrix = {
        {0.04, 0.02, 0.01},
        {0.02, 0.03, 0.015},
        {0.01, 0.015, 0.025}
    };

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
    EXPECT_NEAR(result.optimal_values["AAPL"] + result.optimal_values["GOOG"] + result.optimal_values["MSFT"], 1.0, 1e-12);
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
    risk_model->covariance_matrix = {
        {0.04, 0.03},
        {0.01, 0.09}
    };

    EXPECT_FALSE(solver.supports(problem));
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

TEST(CvxpyAdapterTest, MissingWorkerPathReturnsErrorWithoutStartingPython) {
    CvxpyAdapter solver;
    auto problem = make_supported_mean_variance_problem();
    SolverConfig config;
    config.custom_params["cvxpy_worker_path"] = "Z:/definitely/missing/cvxpy_worker.py";

    auto result = solver.solve(problem, config);

    EXPECT_EQ(result.status, SolverStatus::Error);
    EXPECT_NE(result.message.find("CVXPY worker not found"), std::string::npos);
}
