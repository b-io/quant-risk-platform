// Verifies covariance estimation, horizon scaling, PSD repair, and historical-data validation.

#include <qrp/analytics/covariance_estimator.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <ql/math/matrix.hpp>
#include <ql/math/matrixutilities/symmetricschurdecomposition.hpp>
#include <stdexcept>
#include <vector>

using namespace qrp::analytics;
using namespace qrp::domain;

TEST(CovarianceTest, TestSimpleSampleCovariance) {
    std::vector<FactorDefinition> factors;
    FactorDefinition f1;
    f1.factor_id = "F1";
    FactorDefinition f2;
    f2.factor_id = "F2";
    factors.push_back(f1);
    factors.push_back(f2);

    std::vector<FactorObservation> history;
    // Date 1: F1=1%, F2=2%
    history.push_back({"F1", "2024-01-01", 1.0, 0.01, "absolute"});
    history.push_back({"F2", "2024-01-01", 1.0, 0.02, "absolute"});
    // Date 2: F1=2%, F2=4% (Perfect correlation)
    history.push_back({"F1", "2024-01-02", 1.0, 0.02, "absolute"});
    history.push_back({"F2", "2024-01-02", 1.0, 0.04, "absolute"});

    CovarianceEstimationConfig config;
    config.demean = true;
    config.use_ewma = false;

    QuantLib::Matrix cov = CovarianceEstimator::estimate_covariance(factors, history, config);

    ASSERT_EQ(cov.rows(), 2);
    ASSERT_EQ(cov.columns(), 2);

    // Mean F1 = 0.015, Mean F2 = 0.03
    // Var F1 = ( (0.01-0.015)^2 + (0.02-0.015)^2 ) / 1 = 0.000025 + 0.000025 = 0.00005
    // Var F2 = ( (0.02-0.03)^2 + (0.04-0.03)^2 ) / 1 = 0.0001 + 0.0001 = 0.0002
    // Cov(F1,F2) = ( (0.01-0.015)(0.02-0.03) + (0.02-0.015)(0.04-0.03) ) / 1
    //            = ( -0.005 * -0.01 + 0.005 * 0.01 ) = 0.00005 + 0.00005 = 0.0001

    EXPECT_NEAR(cov[0][0], 0.00005, 1e-7);
    EXPECT_NEAR(cov[1][1], 0.0002, 1e-7);
    EXPECT_NEAR(cov[0][1], 0.0001, 1e-7);
}

TEST(CovarianceTest, TestHorizonScaling) {
    std::vector<FactorDefinition> factors;
    FactorDefinition f1;
    f1.factor_id = "F1";
    factors.push_back(f1);

    std::vector<FactorObservation> history;
    history.push_back({"F1", "2024-01-01", 1.0, 0.01, "absolute"});
    history.push_back({"F1", "2024-01-02", 1.0, 0.02, "absolute"});

    CovarianceEstimationConfig config;
    config.observation_interval_years = 1.0 / 252.0;
    config.target_horizon_years = 10.0 / 252.0;
    config.return_horizon_scaled_covariance = true;

    QuantLib::Matrix cov = CovarianceEstimator::estimate_covariance(factors, history, config);

    // Base variance = 0.00005
    // Scaled variance = (10/1) * 0.00005 = 0.0005
    EXPECT_NEAR(cov[0][0], 0.0005, 1e-7);
}

TEST(CovarianceTest, TestPSDRepair) {
    double floor = 0.1;
    QuantLib::Matrix empty(0, 0);
    QuantLib::Matrix empty_repaired = CovarianceEstimator::repair_psd(empty, floor);
    EXPECT_EQ(empty_repaired.rows(), 0U);
    EXPECT_EQ(empty_repaired.columns(), 0U);

    // Create a non-PSD matrix
    // [ 1  2 ]
    // [ 2  1 ]
    // Eigenvalues: det(1-L, 2; 2, 1-L) = (1-L)^2 - 4 = L^2 - 2L - 3 = (L-3)(L+1)
    // Eigenvalues are 3 and -1.
    QuantLib::Matrix non_psd(2, 2);
    non_psd[0][0] = 1.0;
    non_psd[0][1] = 2.0;
    non_psd[1][0] = 2.0;
    non_psd[1][1] = 1.0;

    QuantLib::Matrix repaired = CovarianceEstimator::repair_psd(non_psd, floor);

    // Repaired eigenvalues should be 3 and 0.1
    QuantLib::SymmetricSchurDecomposition schur(repaired);
    QuantLib::Array ev = schur.eigenvalues();

    EXPECT_NEAR(std::max(ev[0], ev[1]), 3.0, 1e-7);
    EXPECT_NEAR(std::min(ev[0], ev[1]), 0.1, 1e-7);
}

TEST(CovarianceTest, EmptyFactorsReturnsEmptyMatrix) {
    CovarianceEstimationConfig config;
    auto cov = CovarianceEstimator::estimate_covariance({}, {}, config);

    EXPECT_EQ(cov.rows(), 0U);
    EXPECT_EQ(cov.columns(), 0U);
}

TEST(CovarianceTest, InsufficientHistoryReturnsZeroMatrix) {
    FactorDefinition factor;
    factor.factor_id = "F1";

    std::vector<FactorObservation> history = {{"F1", "2024-01-01", 1.0, 0.01, "absolute"}};

    CovarianceEstimationConfig config;
    auto cov = CovarianceEstimator::estimate_covariance({factor}, history, config);

    ASSERT_EQ(cov.rows(), 1U);
    EXPECT_DOUBLE_EQ(cov[0][0], 0.0);
}

TEST(CovarianceTest, MissingSynchronizedObservationThrows) {
    FactorDefinition f1;
    f1.factor_id = "F1";
    FactorDefinition f2;
    f2.factor_id = "F2";
    FactorDefinition f3;
    f3.factor_id = "F3";

    std::vector<FactorObservation> history = {{"F1", "2024-01-01", 1.0, 0.01, "absolute"},
                                              {"F2", "2024-01-01", 1.0, 0.02, "absolute"},
                                              {"F1", "2024-01-02", 1.0, 0.03, "absolute"}};

    CovarianceEstimationConfig config;

    EXPECT_THROW(CovarianceEstimator::estimate_covariance({f1, f2}, history, config), std::runtime_error);
    EXPECT_THROW(CovarianceEstimator::estimate_covariance({f1, f3}, history, config), std::runtime_error);
}

TEST(CovarianceTest, EwmaCovarianceUsesConfiguredLambda) {
    FactorDefinition factor;
    factor.factor_id = "F1";
    FactorDefinition second_factor;
    second_factor.factor_id = "F2";

    std::vector<FactorObservation> history = {{"F1", "2024-01-01", 1.0, 0.01, "absolute"},
                                              {"F2", "2024-01-01", 1.0, 0.02, "absolute"},
                                              {"F1", "2024-01-02", 1.0, 0.03, "absolute"},
                                              {"F2", "2024-01-02", 1.0, 0.04, "absolute"}};

    CovarianceEstimationConfig config;
    config.demean = false;
    config.ewma_lambda = 0.5;
    config.return_horizon_scaled_covariance = false;
    config.use_ewma = true;

    auto cov = CovarianceEstimator::estimate_covariance({factor, second_factor}, history, config);

    // Recursive EWMA from zero: 0.5 * 0 + 0.5 * 0.01^2, then 0.5 * prev + 0.5 * 0.03^2.
    EXPECT_NEAR(cov[0][0], 0.000475, 1e-12);
    EXPECT_NEAR(cov[0][1], 0.00065, 1e-12);
    EXPECT_NEAR(cov[1][0], cov[0][1], 1e-12);
}
