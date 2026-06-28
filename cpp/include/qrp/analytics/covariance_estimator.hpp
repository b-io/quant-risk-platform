#pragma once

// Declares covariance estimation utilities for historical factor moves and simulation inputs.

#include <qrp/domain/factors.hpp>
#include <ql/math/matrix.hpp>
#include <vector>
#include <string>

namespace qrp::analytics {

/**
 * @brief Configuration for the covariance estimation process.
 */
struct CovarianceEstimationConfig {
    std::string start_date;
    std::string end_date;
    bool use_ewma = false;
    double ewma_lambda = 0.97;
    bool demean = true;
    double observation_interval_years = 1.0 / 252.0;
    double target_horizon_years = 1.0 / 252.0;
    bool return_horizon_scaled_covariance = true;
    double eigenvalue_floor = 1e-10;
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
     * @brief Estimates the covariance matrix Σ for a set of factors.
     * 
     * Formula: Σ_base = 1/(M-1) * sum((x_k - μ)(x_k - μ)^T)
     * 
     * If the input moves are daily and the requested horizon is H days, 
     * the returned matrix is horizon-scaled:
     * Σ_H = H * Σ_base
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
     * Uses eigenvalue decomposition: Σ_repaired = Q * Λ+ * Q^T
     * where Λ+ = max(λ, floor).
     */
    static QuantLib::Matrix repair_psd(const QuantLib::Matrix& raw, double floor);
};

} // namespace qrp::analytics
