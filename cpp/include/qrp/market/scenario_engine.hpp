#pragma once

// Declares scenario application APIs that map factor shocks onto market-state quote handles.

#include <qrp/domain/market_data.hpp>
#include <qrp/domain/factors.hpp>
#include <qrp/market/market_state.hpp>
#include <map>
#include <string>

namespace qrp::market {

class MarketState; // Forward declaration

struct ScenarioDefinition {
    std::string name;
    // Generic Factor Shocks
    std::map<std::string, double> factor_shocks; // factor_id -> shock value
};

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
