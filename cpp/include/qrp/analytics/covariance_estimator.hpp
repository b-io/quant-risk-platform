#pragma once

// Declares covariance estimation utilities for historical factor moves and simulation inputs.

#include <qrp/domain/factors.hpp>

#include <ql/math/matrix.hpp>

#include <string>
#include <vector>

namespace qrp::analytics {

/**
 * @brief Configuration for the covariance estimation process.
 */
struct CovarianceEstimationConfig {
    std::string start_date;                         // Inclusive first observation date.
    std::string end_date;                           // Inclusive last observation date.
    bool use_ewma = false;                          // Use exponentially weighted covariance when true.
    double ewma_lambda = 0.97;                       // EWMA decay factor.
    bool demean = true;                             // Remove sample mean before covariance estimation.
    double observation_interval_years = 1.0 / 252.0; // Time represented by one historical move.
    double target_horizon_years = 1.0 / 252.0;       // Horizon used when scaling the covariance.
    bool return_horizon_scaled_covariance = true;    // Return covariance scaled to target_horizon_years.
    double eigenvalue_floor = 1e-10;                 // Minimum eigenvalue used during PSD repair.
};

/**
 * @brief Service to estimate factor covariance from historical moves.
 *
 * Includes robust estimation techniques like EWMA and PSD repair via
 * eigenvalue clipping.
 */
class CovarianceEstimator {
public:
    /**
     * @brief Estimates the covariance matrix Sigma for a set of factors.
     *
     * Formula: Sigma_base = 1/(M-1) * sum((x_k - mu)(x_k - mu)^T)
     *
     * If the input moves are daily and the requested horizon is H days,
     * the returned matrix is horizon-scaled:
     * Sigma_H = H * Sigma_base
     *
     * @param factors The factor definitions defining the units/types.
     * @param history The historical observations of those factors.
     * @param config Estimation parameters.
     * @return A symmetric positive definite matrix safe for Cholesky.
     */
    static QuantLib::Matrix estimate_covariance(
        const std::vector<domain::FactorDefinition>& factors,
        const std::vector<domain::FactorObservation>& history,
        const CovarianceEstimationConfig& config);

    /**
     * @brief Repairs a symmetric matrix to ensure it is Positive Semi-Definite (PSD).
     *
     * Uses eigenvalue decomposition: Sigma_repaired = Q * Lambda+ * Q^T
     * where Lambda+ = max(lambda, floor).
     */
    static QuantLib::Matrix repair_psd(const QuantLib::Matrix& raw, double floor);
};

} // namespace qrp::analytics
