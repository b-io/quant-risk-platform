#pragma once

// Declares scenario application APIs that map factor shocks onto market-state quote handles.

#include <qrp/domain/factors.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/market/market_state.hpp>

#include <map>
#include <string>
#include <vector>

namespace qrp::market {

/**
 * @brief Forward declaration of mutable market-state quote and curve handles.
 */
class MarketState;

/**
 * @brief Named scenario containing canonical factor shocks.
 */
struct ScenarioDefinition {
    /** @brief Stable scenario name used in reports and persisted run results. */
    std::string name;

    /** @brief Canonical factor-id to shock-value map. */
    std::map<std::string, double> factor_shocks;
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
