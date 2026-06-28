#pragma once

// Declares regression models used to fit continuation values in simulation algorithms.

#include <Eigen/Dense>
#include <vector>

namespace qrp::analytics::regression {

struct RegressionResult {
    Eigen::VectorXd coefficients;
    double r_squared;
    double residual_sum_of_squares;
};

class RegressionModel {
public:
    virtual ~RegressionModel() = default;
    virtual RegressionResult fit(const Eigen::MatrixXd& X, const Eigen::VectorXd& y) const = 0;
    virtual Eigen::VectorXd predict(const Eigen::MatrixXd& X, const Eigen::VectorXd& coefficients) const = 0;
};

class OrdinaryLeastSquares : public RegressionModel {
public:
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

    Eigen::VectorXd predict(const Eigen::MatrixXd& X, const Eigen::VectorXd& coefficients) const override {
        return X * coefficients;
    }
};

} // namespace qrp::analytics::regression
