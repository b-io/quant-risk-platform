#pragma once

// Declares covariance and factor risk-model interfaces for optimization workflows.

#include <string>
#include <vector>
#include <map>

namespace qrp::optimization {

/**
 * @brief Abstract base class for risk models.
 */
class RiskModel {
public:
    virtual ~RiskModel() = default;
    virtual std::string type() const = 0;
};

/**
 * @brief Full Covariance Risk Model.
 * Sigma is an N x N dense matrix.
 */
class FullCovarianceModel : public RiskModel {
public:
    std::string type() const override { return "FullCovariance"; }
    
    std::vector<std::string> asset_ids;
    std::vector<std::vector<double>> covariance_matrix; // Asset-by-asset
};

/**
 * @brief Factor Risk Model: Sigma = B * F * B^T + D
 */
class FactorRiskModel : public RiskModel {
public:
    std::string type() const override { return "FactorRisk"; }
    
    std::vector<std::string> asset_ids;
    std::vector<std::string> factor_ids;
    
    // B: Asset-by-Factor exposure matrix (N assets x K factors)
    std::vector<std::vector<double>> exposures; 
    
    // F: Factor covariance matrix (K factors x K factors)
    std::vector<std::vector<double>> factor_covariance;
    
    // D: Specific/Idiosyncratic risk (N assets)
    std::map<std::string, double> specific_risk; 
};

} // namespace qrp::optimization
