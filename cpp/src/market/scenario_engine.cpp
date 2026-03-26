#include <qrp/market/scenario_engine.hpp>
#include <qrp/market/market_snapshot.hpp>

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

/**
 * @brief Applies a scenario to a MarketSnapshot DTO and returns a shocked copy.
 * @param base_market The original market data.
 * @param scenario The shocks to apply.
 * @return A new shocked MarketSnapshot.
 */
domain::MarketSnapshot ScenarioEngine::apply_scenario(
    const domain::MarketSnapshot& base_market,
    const ScenarioDefinition& scenario) {

    domain::MarketSnapshot shocked = base_market;

    for (auto& quote : shocked.quotes) {
        std::string cc_str = domain::to_string(quote.currency);

        // 1. Parallel shock: Applied to all quotes of a specific currency.
        if (scenario.parallel_shocks.contains(cc_str)) {
            quote.value += scenario.parallel_shocks.at(cc_str);
        }

        // 2. Node shocks: Applied to specific quotes identified by ID.
        if (scenario.node_shocks.contains(cc_str)) {
            const auto& nodes_to_shock = scenario.node_shocks.at(cc_str);
            if (nodes_to_shock.contains(quote.id)) {
                quote.value += nodes_to_shock.at(quote.id);
            }
        }

        // 3. Twist shock: A slope-based shift based on the quote's tenor.
        // shift = slope * (T - T_pivot)
        if (scenario.twist_shocks.contains(cc_str)) {
            const auto& twist = scenario.twist_shocks.at(cc_str);
            double t = CurveBuilder::tenor_to_years(quote.tenor);
            quote.value += twist.slope * (t - twist.pivot_tenor);
        }

        // 4. Credit shock: Specifically for CDS instruments.
        if (quote.instrument_type == domain::QuoteInstrumentType::CDS) {
            if (scenario.credit_shocks.contains(quote.id)) {
                quote.value += scenario.credit_shocks.at(quote.id);
            }
        }
    }
    return shocked;
}

/**
 * @brief Applies a scenario directly to a built MarketState (reactive handles).
 * @param state The active market state to mutate.
 * @param base_dto The base values to shock from (ensures shocks are relative to base, not current state).
 * @param scenario The shocks to apply.
 */
void ScenarioEngine::apply_scenario_to_state(
    MarketState& state,
    const domain::MarketSnapshot& base_dto,
    const ScenarioDefinition& scenario) {
    
    for (const auto& quote_dto : base_dto.quotes) {
        std::string cc_str = domain::to_string(quote_dto.currency);
        double shocked_value = quote_dto.value;

        // 1. Parallel shock
        if (scenario.parallel_shocks.contains(cc_str)) {
            shocked_value += scenario.parallel_shocks.at(cc_str);
        }

        // 2. Node shocks: Using quote_id for precision.
        if (scenario.node_shocks.contains(cc_str)) {
            const auto& nodes_to_shock = scenario.node_shocks.at(cc_str);
            if (nodes_to_shock.contains(quote_dto.id)) {
                shocked_value += nodes_to_shock.at(quote_dto.id);
            }
        }

        // 3. Twist shock
        if (scenario.twist_shocks.contains(cc_str)) {
            const auto& twist = scenario.twist_shocks.at(cc_str);
            double t = CurveBuilder::tenor_to_years(quote_dto.tenor);
            shocked_value += twist.slope * (t - twist.pivot_tenor);
        }

        // 4. Credit shock
        if (quote_dto.instrument_type == domain::QuoteInstrumentType::CDS) {
            if (scenario.credit_shocks.contains(quote_dto.id)) {
                shocked_value += scenario.credit_shocks.at(quote_dto.id);
            }
        }

        // This update triggers QuantLib observers (curves/instruments) to invalidate their caches.
        state.add_quote(quote_dto.id, shocked_value);
    }
}

} // namespace qrp::market
