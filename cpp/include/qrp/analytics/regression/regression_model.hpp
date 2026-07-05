#pragma once

// Declares regression models used to fit continuation values in simulation algorithms.

#include <Eigen/Dense>

#include <vector>

namespace qrp::analytics::regression {

/**
 * @brief Fitted regression coefficients and goodness-of-fit diagnostics.
 */
struct RegressionResult {
    Eigen::VectorXd coefficients;     // Fitted regression coefficients.
    double r_squared;                 // Coefficient of determination.
    double residual_sum_of_squares;   // Sum of squared residuals.
};

/**
 * @brief Interface for continuation-value regression models.
 */
class RegressionModel {
public:
    /**
     * @brief Allows deletion through the regression-model base type.
     */
    virtual ~RegressionModel() = default;

    /**
     * @brief Fits a regression model to design matrix X and target y.
     */
    virtual RegressionResult fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y) const = 0;

    /**
     * @brief Predicts target values from X and fitted coefficients.
     */
    virtual Eigen::VectorXd predict(const Eigen::MatrixXd& X, const Eigen::VectorXd& coefficients) const = 0;
};

/**
 * @brief Ordinary least-squares model solved with Eigen's QR decomposition.
 */
class OrdinaryLeastSquares : public RegressionModel {
public:
    /**
     * @brief Fits ordinary least squares coefficients and diagnostics.
     */
    RegressionResult fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y) const override {
        // Solve (X^T * X) * beta = X^T * y
        // Using ColPivHouseholderQR for stability
        Eigen::VectorXd beta = X.colPivHouseholderQr().solve(y);

        RegressionResult result;
        result.coefficients = beta;

        Eigen::VectorXd y_pred = X * beta;
        Eigen::VectorXd residuals = y - y_pred;
        result.residual_sum_of_squares = residuals.squaredNorm();

        double y_mean = y.mean();
        double total_sum_of_squares = (y.array() - y_mean).square().sum();

        if (total_sum_of_squares > 1e-14) {
            result.r_squared = 1.0 - (result.residual_sum_of_squares / total_sum_of_squares);
        } else {
            result.r_squared = 1.0;
        }

        return result;
    }

    /**
     * @brief Predicts values using a linear model.
     */
    Eigen::VectorXd predict(const Eigen::MatrixXd& X, const Eigen::VectorXd& coefficients) const override {
        return X * coefficients;
    }
};

} // namespace qrp::analytics::regression
