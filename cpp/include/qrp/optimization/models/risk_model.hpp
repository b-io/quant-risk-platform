#pragma once

// Declares covariance and factor risk-model interfaces for optimization workflows.

#include <map>
#include <string>
#include <vector>

namespace qrp::optimization {

/**
 * @brief Abstract base class for risk models.
 */
class RiskModel {
public:
    /**
     * @brief Allows deletion through the risk-model base type.
     */
    virtual ~RiskModel() = default;

    /**
     * @brief Returns the stable risk-model type name used by solver adapters.
     */
    virtual std::string type() const = 0;
};

/**
 * @brief Full Covariance Risk Model.
 * Sigma is an N x N dense matrix.
 */
class FullCovarianceModel : public RiskModel {
public:
    /**
     * @brief Returns the solver-facing risk-model type identifier.
     */
    std::string type() const override {
        return "FullCovariance";
    }

    std::vector<std::string> asset_ids;                 // Asset order used by the covariance matrix.
    std::vector<std::vector<double>> covariance_matrix; // Asset-by-asset covariance matrix.
};

/**
 * @brief Factor Risk Model: Sigma = B * F * B^T + D
 */
class FactorRiskModel : public RiskModel {
public:
    /**
     * @brief Returns the solver-facing risk-model type identifier.
     */
    std::string type() const override {
        return "FactorRisk";
    }

    std::vector<std::string> asset_ids;  // Asset order used by exposure and specific-risk inputs.
    std::vector<std::string> factor_ids; // Factor order used by exposure and covariance matrices.

    // B: Asset-by-Factor exposure matrix (N assets x K factors)
    std::vector<std::vector<double>> exposures;

    // F: Factor covariance matrix (K factors x K factors)
    std::vector<std::vector<double>> factor_covariance;

    // D: Specific/Idiosyncratic risk (N assets)
    std::map<std::string, double> specific_risk;
};

} // namespace qrp::optimization
