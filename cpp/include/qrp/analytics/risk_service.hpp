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
    std::string trade_id;                        // Risked trade id.
    double pv01 = 0.0;                           // Parallel rate shock sensitivity.
    double cs01 = 0.0;                           // Parallel credit spread shock sensitivity.
    double fx_delta = 0.0;                       // FX spot sensitivity from configured FX spot factors.
    double fx_vega = 0.0;                        // FX volatility sensitivity from configured FX vol factors.
    double spread_duration = 0.0;                // Normalized credit spread sensitivity.
    std::map<std::string, double> bucketed_risk; // Factor bucket to shocked PnL contribution.
};

/**
 * @brief Computes deterministic first-order risk measures from factor-shock bindings.
 */
class RiskService {
public:
    /**
     * @brief Computes trade-level risk metrics for a portfolio against a base market snapshot.
     */
    static std::vector<RiskResult> compute_risk(const domain::Portfolio& portfolio,
                                                const domain::MarketSnapshot& base_market_dto,
                                                const std::vector<domain::FactorDefinition>& factors,
                                                const std::vector<domain::FactorBinding>& bindings);
};

} // namespace qrp::analytics
