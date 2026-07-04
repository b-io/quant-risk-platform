// Implements historical factor covariance estimation, scaling, and PSD repair utilities.

#include <qrp/analytics/covariance_estimator.hpp>

#include <ql/math/matrix.hpp>
#include <ql/math/matrixutilities/choleskydecomposition.hpp>
#include <ql/math/matrixutilities/symmetricschurdecomposition.hpp>

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>

namespace qrp::analytics {

QuantLib::Matrix CovarianceEstimator::estimate_covariance(
    const std::vector<domain::FactorDefinition>& factors,
    const std::vector<domain::FactorObservation>& history,
    const CovarianceEstimationConfig& config) {

    size_t num_factors = factors.size();
    if (num_factors == 0) return QuantLib::Matrix(0, 0);

    // 1. Organize history into a matrix: factors (rows) x dates (cols)
    std::map<std::string, std::vector<double>> moves_per_factor;
    std::vector<std::string> dates;

    // Find common dates if possible, or use all
    std::map<std::string, std::map<std::string, double>> factor_date_map;
    for (const auto& obs : history) {
        factor_date_map[obs.factor_id][obs.market_date] = obs.move;
        if (std::find(dates.begin(), dates.end(), obs.market_date) == dates.end()) {
            dates.push_back(obs.market_date);
        }
    }
    std::sort(dates.begin(), dates.end());

    size_t num_obs = dates.size();
    if (num_obs < 2) return QuantLib::Matrix(num_factors, num_factors, 0.0);

    // Fill data matrix X (num_factors x num_obs)
    QuantLib::Matrix X(num_factors, num_obs, 0.0);
    for (size_t i = 0; i < num_factors; ++i) {
        auto& factor_history = factor_date_map[factors[i].factor_id];
        if (factor_history.empty()) {
            throw std::runtime_error("CovarianceEstimator: Missing history for factor " + factors[i].factor_id);
        }
        for (size_t k = 0; k < num_obs; ++k) {
            auto it = factor_history.find(dates[k]);
            if (it != factor_history.end()) {
                X[i][k] = it->second;
            } else {
                // Missing observations are rejected rather than silently filled.
                // Covariance estimation requires synchronized factor history unless
                // a caller supplies an explicit missing-data policy upstream.
                throw std::runtime_error("CovarianceEstimator: Missing observation for factor "
                    + factors[i].factor_id + " on date " + dates[k]);
            }
        }
    }

    // 2. Compute Mean mu and Covariance Sigma
    // mu = 1/M * sum(x_k)
    std::vector<double> means(num_factors, 0.0);
    if (config.demean) {
        for (size_t i = 0; i < num_factors; ++i) {
            double sum = 0.0;
            for (size_t k = 0; k < num_obs; ++k) sum += X[i][k];
            means[i] = sum / num_obs;
        }
    }

    QuantLib::Matrix cov(num_factors, num_factors, 0.0);

    if (config.use_ewma) {
        // EWMA: Sigma_k = lambda * Sigma_{k-1} + (1-lambda) * (x_k - mu)(x_k - mu)^T
        double lambda = config.ewma_lambda;
        for (size_t k = 0; k < num_obs; ++k) {
            for (size_t i = 0; i < num_factors; ++i) {
                double diff_i = X[i][k] - means[i];
                for (size_t j = 0; j <= i; ++j) {
                    double diff_j = X[j][k] - means[j];
                    cov[i][j] = lambda * cov[i][j] + (1.0 - lambda) * diff_i * diff_j;
                    if (i != j) cov[j][i] = cov[i][j];
                }
            }
        }
    } else {
        // Standard sample covariance: Sigma = 1/(M-1) * sum((x_k - mu)(x_k - mu)^T)
        for (size_t i = 0; i < num_factors; ++i) {
            for (size_t j = 0; j <= i; ++j) {
                double sum = 0.0;
                for (size_t k = 0; k < num_obs; ++k) {
                    sum += (X[i][k] - means[i]) * (X[j][k] - means[j]);
                }
                cov[i][j] = sum / (num_obs - 1);
                if (i != j) cov[j][i] = cov[i][j];
            }
        }
    }

    // 3. Scale to target horizon
    // Formula: Sigma_target = (h_target / h_obs) * Sigma_obs
    if (config.return_horizon_scaled_covariance) {
        double h_obs = config.observation_interval_years;
        double h_target = config.target_horizon_years;
        if (h_obs > 0 && h_target != h_obs) {
            double scale = h_target / h_obs;
            for (size_t i = 0; i < num_factors; ++i) {
                for (size_t j = 0; j < num_factors; ++j) cov[i][j] *= scale;
            }
        }
    }

    // 4. PSD Repair
    return repair_psd(cov, config.eigenvalue_floor);
}

QuantLib::Matrix CovarianceEstimator::repair_psd(const QuantLib::Matrix& raw, double floor) {
    size_t n = raw.rows();
    if (n == 0) return raw;

    // Eigenvalue Decomposition of the Raw Covariance Matrix
    // Sigma = Q * Lambda * Q^T
    QuantLib::SymmetricSchurDecomposition schur(raw);
    QuantLib::Array eigenvalues = schur.eigenvalues();
    QuantLib::Matrix Q = schur.eigenvectors();

    // Repair Step: Clip eigenvalues to a floor epsilon > 0.
    // Lambda_repaired = max(lambda, epsilon)
    QuantLib::Matrix Lambda_plus(n, n, 0.0);
    for (size_t i = 0; i < n; ++i) {
        Lambda_plus[i][i] = std::max(eigenvalues[i], floor);
    }

    // Reconstruct the repaired matrix: Sigma_repaired = Q * Lambda_plus * Q^T
    // This ensures the matrix is symmetric positive definite and safe for Cholesky.
    QuantLib::Matrix temp = Q * Lambda_plus;
    QuantLib::Matrix repaired = temp * transpose(Q);

    return repaired;
}

} // namespace qrp::analytics
