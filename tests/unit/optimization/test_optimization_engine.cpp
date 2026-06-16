#include <gtest/gtest.h>
#include <qrp/optimization/portfolio_optimization_engine.hpp>
#include <qrp/optimization/adapters/cvxpy_adapter.hpp>
#include <qrp/optimization/portfolio_optimization.hpp>
#include <qrp/optimization/models/risk_model.hpp>

using namespace qrp::optimization;

TEST(PortfolioOptimizationTest, LongOnlyMeanVariance) {
    auto solver = std::make_shared<CvxpyAdapter>();
    PortfolioOptimizationEngine engine(solver);

    std::map<std::string, double> returns = {
        {"AAPL", 0.15},
        {"MSFT", 0.12},
        {"GOOG", 0.10}
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

    std::vector<std::shared_ptr<OptimizationConstraint>> constraints;
    
    // Sum of weights = 1
    auto budget = std::make_shared<LinearEqualityConstraint>();
    budget->coefficients = {{"AAPL", 1.0}, {"MSFT", 1.0}, {"GOOG", 1.0}};
    budget->target_value = 1.0;
    constraints.push_back(budget);

    // Long only 
    auto long_only = std::make_shared<LinearInequalityConstraint>();
    long_only->coefficients = {{"AAPL", 1.0}};
    long_only->lower_bound = 0.0;
    constraints.push_back(long_only); 

    auto result = engine.optimize({}, returns, risk_model, constraints, objective);

    EXPECT_EQ(result.status, SolverStatus::Solved);
    EXPECT_GT(result.optimal_values.size(), 0);
    
    double sum_w = 0;
    for (auto const& [id, val] : result.optimal_values) {
        sum_w += val;
        EXPECT_GE(val, -1e-7); // Close to 0
    }
    EXPECT_NEAR(sum_w, 1.0, 1e-6);
}

TEST(PortfolioOptimizationTest, FactorRiskModel) {
    auto solver = std::make_shared<CvxpyAdapter>();
    PortfolioOptimizationEngine engine(solver);

    std::map<std::string, double> returns = {
        {"AAPL", 0.1},
        {"TSLA", 0.2}
    };

    auto risk_model = std::make_shared<FactorRiskModel>();
    risk_model->asset_ids = {"AAPL", "TSLA"};
    risk_model->factor_ids = {"Market"};
    risk_model->exposures = {
        {1.2}, // AAPL beta
        {2.0}  // TSLA beta
    };
    risk_model->factor_covariance = {{0.04}};
    risk_model->specific_risk = {{"AAPL", 0.01}, {"TSLA", 0.05}};

    auto objective = std::make_shared<MeanVarianceObjective>();
    objective->risk_aversion = 1.0;

    std::vector<std::shared_ptr<OptimizationConstraint>> constraints;
    auto budget = std::make_shared<LinearEqualityConstraint>();
    budget->coefficients = {{"AAPL", 1.0}, {"TSLA", 1.0}};
    budget->target_value = 1.0;
    constraints.push_back(budget);

    auto result = engine.optimize({}, returns, risk_model, constraints, objective);

    EXPECT_EQ(result.status, SolverStatus::Solved);
    EXPECT_NEAR(result.optimal_values["AAPL"] + result.optimal_values["TSLA"], 1.0, 1e-6);
}
