#include <qrp/analytics/risk_service.hpp>
#include <set>

/*
Design note (see docs/design/ANALYTICS_SERVICES.md):
- RiskService performs PV01 and key-rate DV01 by applying shocks directly to SimpleQuote handles via
  ScenarioEngine::apply_scenario_to_state, avoiding full market rebuilds per bump.
- Instruments are translated once and cached; QuantLib's lazy evaluation updates NPV after handle bumps.
- This structure enables linear scaling across buckets and scenarios and is friendly to parallelization later.
*/
namespace qrp::analytics {

std::vector<RiskResult> RiskService::compute_risk(
    const domain::Portfolio& portfolio,
    const domain::MarketSnapshot& base_market_dto) {

    double bump_size = 0.0001; // 1bp
    std::vector<RiskResult> results;

    // Base Market setup
    market::MarketSnapshot base_market(base_market_dto);
    auto state = base_market.built_state();
    PricingContext context(state);

    // Instrument cache to avoid repeated translation
    std::vector<std::pair<std::string, std::shared_ptr<QuantLib::Instrument>>> instruments;
    for (const auto& trade : portfolio.trades) {
        auto inst = instruments::InstrumentFactory::create_instrument(trade, context);
        if (inst) instruments.push_back({trade.id, inst});
    }

    // Map base NPVs
    std::map<std::string, double> base_map;
    for (auto& [id, inst] : instruments) {
        base_map[id] = inst->NPV();
    }

    // Determine currencies in portfolio to shock
    std::set<std::string> currencies;
    for (const auto& t : portfolio.trades) currencies.insert(t.currency);

    for (const auto& [id, base_npv] : base_map) {
        RiskResult res;
        res.trade_id = id;
        res.pv01 = 0.0;
        res.cs01 = 0.0;

        // Reset state to base
        for (const auto& q : base_market_dto.quotes) {
            state->add_quote(q.id, q.value);
        }

        for (const auto& cc : currencies) {
            // 1. Parallel PV01
            market::ScenarioDefinition parallel_up;
            parallel_up.parallel_shocks[cc] = bump_size;
            
            market::ScenarioEngine::apply_scenario_to_state(*state, base_market_dto, parallel_up);
            for (auto& [inst_id, inst] : instruments) {
                if (inst_id == id) {
                    res.pv01 += (inst->NPV() - base_npv);
                    break;
                }
            }
            // Reset after PV01
            for (const auto& q : base_market_dto.quotes) state->add_quote(q.id, q.value);

            // 2. CS01
            market::ScenarioDefinition credit_up;
            for (const auto& q : base_market_dto.quotes) {
                if (q.type == domain::QuoteType::CreditSpread) {
                    credit_up.credit_shocks[q.id] = bump_size;
                }
            }
            if (!credit_up.credit_shocks.empty()) {
                market::ScenarioEngine::apply_scenario_to_state(*state, base_market_dto, credit_up);
                for (auto& [inst_id, inst] : instruments) {
                    if (inst_id == id) {
                        res.cs01 += (inst->NPV() - base_npv);
                        break;
                    }
                }
                for (const auto& q : base_market_dto.quotes) state->add_quote(q.id, q.value);
            }

            // 3. Bucketed Key-Rate Risk
            for (const auto& q : base_market_dto.quotes) {
                if (domain::to_string(q.currency) == cc) {
                    market::ScenarioDefinition node_shock;
                    node_shock.node_shocks[cc][q.tenor] = bump_size;

                    market::ScenarioEngine::apply_scenario_to_state(*state, base_market_dto, node_shock);
                    for (auto& [inst_id, inst] : instruments) {
                        if (inst_id == id) {
                            res.bucketed_risk[q.tenor] += (inst->NPV() - base_npv);
                            break;
                        }
                    }
                    state->add_quote(q.id, q.value); // Reset single quote
                }
            }
        }
        results.push_back(res);
    }
    return results;
}

} // namespace qrp::analytics
