#pragma once

// Declares deterministic stress revaluation APIs and result DTOs.

#include <qrp/market/scenario_engine.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/portfolio.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <vector>
#include <string>

namespace qrp::analytics {

/**
 * @brief Scenario-level stress output with portfolio and trade PnL contributions.
 */
struct StressResult {
    std::string scenario_name;
    double total_pnl;
    std::map<std::string, double> trade_pnls;
};

/**
 * @brief Revalues portfolios under deterministic market shock scenarios.
 */
class StressEngine {
public:
    /**
     * @brief Applies each historical scenario to the base market and returns stressed PnL.
     */
    static std::vector<StressResult> run_historical_stress(
        const domain::Portfolio& portfolio,
        const domain::MarketSnapshot& base_market_dto,
        const std::vector<market::ScenarioDefinition>& historical_scenarios,
        const std::vector<domain::FactorDefinition>& factors = {},
        const std::vector<domain::FactorBinding>& bindings = {});
};

} // namespace qrp::analytics
