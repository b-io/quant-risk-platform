#pragma once

// Declares deterministic portfolio risk metrics over scenario-shocked market states.

#include <qrp/analytics/valuation_service.hpp>
#include <qrp/market/scenario_engine.hpp>
#include <vector>

namespace qrp::analytics {

/**
 * @brief Trade-level deterministic risk result.
 */
struct RiskResult {
    std::string trade_id;
    double pv01; // Parallel rate shock
    double cs01; // Parallel credit shock
    std::map<std::string, double> bucketed_risk; // Tenor -> shock
};

/**
 * @brief Computes deterministic first-order risk measures from factor-shock bindings.
 */
class RiskService {
public:
    /**
     * @brief Computes trade-level risk metrics for a portfolio against a base market snapshot.
     */
    static std::vector<RiskResult> compute_risk(
        const domain::Portfolio& portfolio,
        const domain::MarketSnapshot& base_market_dto,
        const std::vector<domain::FactorDefinition>& factors,
        const std::vector<domain::FactorBinding>& bindings);
};

} // namespace qrp::analytics
