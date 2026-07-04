// Implements scenario application by resolving factor shocks to concrete quote updates.

#include <qrp/market/scenario_engine.hpp>

#include <qrp/domain/factors.hpp>
#include <qrp/market/factor_shock_resolver.hpp>
#include <qrp/market/market_snapshot.hpp>

#include <cmath>

/**
 * @brief ScenarioEngine applies market shocks (scenarios) to market data.
 *
 * It supports two modes of operation:
 * 1. DTO-to-DTO: Returns a new MarketSnapshot with shocked values. Useful for audit and external systems.
 * 2. In-place State update: Directly modifies MarketState's SimpleQuote handles.
 *    Essential for performance in risk and simulation engines as it triggers QuantLib's
 *    observer mechanism instead of rebuilding curves from scratch.
 */
namespace qrp::market {

void ScenarioEngine::apply_scenario_to_state(
    MarketState& state,
    const domain::MarketSnapshot& reference_snapshot,
    const ScenarioDefinition& scenario,
    const std::vector<domain::FactorDefinition>& factors,
    const std::vector<domain::FactorBinding>& bindings) {

    // 1. Resolve factor shocks to quote absolute values using the reference snapshot
    auto shocked_quotes = FactorShockResolver::resolve_quote_values(scenario, factors, bindings, reference_snapshot);

    // 2. Apply resolved quote values to the state
    for (const auto& [qid, value] : shocked_quotes) {
        state.add_quote(qid, value);
    }
}

} // namespace qrp::market
