#pragma once

// Declares scenario application APIs that map factor shocks onto market-state quote handles.

#include <qrp/domain/factors.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/market/market_state.hpp>

#include <map>
#include <string>
#include <vector>

namespace qrp::market {

class MarketState; // Forward declaration

/**
 * @brief Named scenario containing canonical factor shocks.
 */
struct ScenarioDefinition {
    std::string name;

    // Generic factor shocks
    std::map<std::string, double> factor_shocks; // factor_id -> shock value
};

/**
 * @brief Applies factor-shock scenarios to mutable market states.
 */
class ScenarioEngine {
public:
    /**
     * @brief Binding-aware scenario application.
     * Resolves factor shocks into absolute quote values using the provided bindings.
     */
    static void apply_scenario_to_state(
        MarketState& state,
        const domain::MarketSnapshot& reference_snapshot,
        const ScenarioDefinition& scenario,
        const std::vector<domain::FactorDefinition>& factors,
        const std::vector<domain::FactorBinding>& bindings);
};

} // namespace qrp::market
