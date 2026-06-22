#include <gtest/gtest.h>
#include <qrp/analytics/regression/regression_model.hpp>
#include <Eigen/Dense>

using namespace qrp::analytics::regression;

TEST(RegressionTest, OrdinaryLeastSquaresFitsLinearModel) {
    Eigen::MatrixXd x(4, 2);
    x << 1.0, 0.0,
         1.0, 1.0,
         1.0, 2.0,
         1.0, 3.0;

    Eigen::VectorXd y(4);
    y << 1.0, 3.0, 5.0, 7.0;

    OrdinaryLeastSquares ols;
    auto result = ols.fit(x, y);

    ASSERT_EQ(result.coefficients.size(), 2);
    EXPECT_NEAR(result.coefficients[0], 1.0, 1e-12);
    EXPECT_NEAR(result.coefficients[1], 2.0, 1e-12);
    EXPECT_NEAR(result.r_squared, 1.0, 1e-12);
    EXPECT_NEAR(result.residual_sum_of_squares, 0.0, 1e-12);
}

TEST(RegressionTest, PredictUsesProvidedCoefficients) {
    Eigen::MatrixXd x(2, 3);
    x << 1.0, 2.0, 3.0,
         1.0, 4.0, 5.0;

    Eigen::VectorXd beta(3);
    beta << 10.0, 2.0, -1.0;

    OrdinaryLeastSquares ols;
    Eigen::VectorXd prediction = ols.predict(x, beta);

    ASSERT_EQ(prediction.size(), 2);
    EXPECT_DOUBLE_EQ(prediction[0], 11.0);
    EXPECT_DOUBLE_EQ(prediction[1], 13.0);
}

TEST(RegressionTest, ConstantTargetReportsPerfectRSquared) {
    Eigen::MatrixXd x(3, 1);
    x << 1.0, 1.0, 1.0;

    Eigen::VectorXd y(3);
    y << 5.0, 5.0, 5.0;

    OrdinaryLeastSquares ols;
    auto result = ols.fit(x, y);

    EXPECT_NEAR(result.coefficients[0], 5.0, 1e-12);
    EXPECT_DOUBLE_EQ(result.r_squared, 1.0);
    EXPECT_NEAR(result.residual_sum_of_squares, 0.0, 1e-12);
}
