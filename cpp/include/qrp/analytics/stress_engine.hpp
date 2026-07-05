#pragma once

// Declares deterministic stress revaluation APIs and result DTOs.

#include <qrp/analytics/valuation_service.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/portfolio.hpp>
#include <qrp/market/scenario_engine.hpp>

#include <map>
#include <string>
#include <vector>

namespace qrp::analytics {

/**
 * @brief Scenario-level stress output with portfolio and trade PnL contributions.
 */
struct StressResult {
    std::string scenario_name;                 // Applied stress scenario name.
    double total_pnl;                          // Portfolio PnL under the scenario.
    std::map<std::string, double> trade_pnls;  // Trade id to stressed PnL contribution.
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
