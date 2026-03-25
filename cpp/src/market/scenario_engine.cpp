#include <qrp/market/scenario_engine.hpp>
#include <qrp/market/market_snapshot.hpp>

namespace qrp::market {

domain::MarketSnapshot ScenarioEngine::apply_scenario(
    const domain::MarketSnapshot& base_market,
    const ScenarioDefinition& scenario) {

    domain::MarketSnapshot shocked = base_market;

    for (auto& quote : shocked.quotes) {
        std::string cc_str = domain::to_string(quote.currency);

        // 1. Parallel shock
        if (scenario.parallel_shocks.contains(cc_str)) {
            quote.value += scenario.parallel_shocks.at(cc_str);
        }

        // 2. Node shocks
        if (scenario.node_shocks.contains(cc_str)) {
            const auto& nodes_to_shock = scenario.node_shocks.at(cc_str);
            if (nodes_to_shock.contains(quote.tenor)) {
                quote.value += nodes_to_shock.at(quote.tenor);
            }
        }

        // 3. Twist shock
        if (scenario.twist_shocks.contains(cc_str)) {
            const auto& twist = scenario.twist_shocks.at(cc_str);
            double t = CurveBuilder::tenor_to_years(quote.tenor);
            quote.value += twist.slope * (t - twist.pivot_tenor);
        }

        // 4. Credit shock (applied if quote is a credit spread)
        if (quote.type == domain::QuoteType::CreditSpread) {
            // Simplified: issuer-based shock if we had issuer IDs in quotes
            // For now, check if the quote ID matches or a group matches
            if (scenario.credit_shocks.contains(quote.id)) {
                quote.value += scenario.credit_shocks.at(quote.id);
            }
        }
    }
    return shocked;
}

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

        // 2. Node shocks
        if (scenario.node_shocks.contains(cc_str)) {
            const auto& nodes_to_shock = scenario.node_shocks.at(cc_str);
            if (nodes_to_shock.contains(quote_dto.tenor)) {
                shocked_value += nodes_to_shock.at(quote_dto.tenor);
            }
        }

        // 3. Twist shock
        if (scenario.twist_shocks.contains(cc_str)) {
            const auto& twist = scenario.twist_shocks.at(cc_str);
            double t = CurveBuilder::tenor_to_years(quote_dto.tenor);
            shocked_value += twist.slope * (t - twist.pivot_tenor);
        }

        // 4. Credit shock
        if (quote_dto.type == domain::QuoteType::CreditSpread) {
            if (scenario.credit_shocks.contains(quote_dto.id)) {
                shocked_value += scenario.credit_shocks.at(quote_dto.id);
            }
        }

        state.add_quote(quote_dto.id, shocked_value);
    }
}

} // namespace qrp::market
