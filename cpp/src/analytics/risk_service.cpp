#include <qrp/analytics/risk_service.hpp>
#include <qrp/instruments/instrument_factory.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <ql/instrument.hpp>
#include <map>
#include <memory>
#include <set>
#include <vector>

/*
Design note (see docs/design/ANALYTICS_SERVICES.md):
- RiskService performs PV01 and key-rate DV01 by applying shocks directly to SimpleQuote handles via
  ScenarioEngine::apply_scenario_to_state, avoiding full market rebuilds per bump.
- Instruments are translated once and cached; QuantLib's lazy evaluation updates NPV after handle bumps.
- This structure enables linear scaling across buckets and scenarios and is friendly to parallelization later.
*/
/*
 * @brief RiskService provides deterministic risk analytics (PV01, CS01, Bucketed Risk).
 *
 * Design choices (see docs/design/ANALYTICS_SERVICES.md and docs/design/ARCHITECTURE.md):
 * 1. Brute-force revaluation via shocks: We apply small (1bp) parallel and node shocks to market inputs.
 * 2. Handle-based revaluation: Instead of rebuilding curves/instruments, we update QuantLib::SimpleQuote handles.
 *    This triggers QuantLib's internal observer mechanism, performing lazy re-evaluation of affected NPVs.
 * 3. Granularity: Risk is calculated per-trade and per-currency to avoid double-counting in multi-currency portfolios.
 * 4. Parallelization ready: The service is structured to eventually support parallel evaluation across trades or buckets.
 */
namespace qrp::analytics {

/**
 * @brief Computes risk metrics for a given portfolio and market state.
 * @param portfolio The portfolio of trades to evaluate.
 * @param base_market_dto The base market data used as the reference state.
 * @return A vector of RiskResult containing PV01, CS01, and bucketed risk per trade.
 */
std::vector<RiskResult> RiskService::compute_risk(
    const domain::Portfolio& portfolio,
    const domain::MarketSnapshot& base_market_dto) {

    // 1bp shift is the industry standard for PV01/DV01 calculations.
    // Tradeoff: Larger bumps reduce numerical noise but increase non-linearity errors (Gamma leakage).
    const double bump_size = 0.0001; 
    std::vector<RiskResult> results;

    // Build the initial market state and curves from the DTO.
    // We use the 'built_state' which contains the QuantLib handles and curves.
    qrp::market::MarketSnapshot base_market(base_market_dto);
    auto state = base_market.built_state();
    PricingContext context(state);

    // Instrument cache: We translate domain trades to QuantLib instruments once.
    // This is more efficient than re-translating for every risk bucket.
    std::vector<std::pair<std::string, QuantLib::ext::shared_ptr<QuantLib::Instrument>>> ql_instruments;
    for (const auto& trade : portfolio.trades) {
        auto inst = instruments::InstrumentFactory::create_instrument(trade, context);
        if (inst) ql_instruments.push_back({trade.id, inst});
    }

    // Capture base NPVs before applying any shocks.
    std::map<std::string, double> base_map;
    for (auto& [id, inst] : ql_instruments) {
        base_map[id] = inst->NPV();
    }

    // Iterate through each trade and compute its specific risk sensitivities.
    for (const auto& [id, base_npv] : base_map) {
        RiskResult res;
        res.trade_id = id;
        res.pv01 = 0.0;
        res.cs01 = 0.0;

        // Locate the trade to identify its base currency.
        // We only shock the relevant currency for each trade to ensure independent risks.
        const auto& trade = *std::find_if(portfolio.trades.begin(), portfolio.trades.end(),
                                          [&](const auto& t) { return t.id == id; });
        std::string cc = trade.currency;

        // Ensure the market state is at its base levels before starting shocks for this trade.
        for (const auto& q : base_market_dto.quotes) {
            state->add_quote(q.id, q.value);
        }

        // --- 1. Parallel PV01 (Rate Risk) ---
        // We apply a parallel shift to all yield curve nodes for the trade's currency.
        market::ScenarioDefinition parallel_up;
        parallel_up.parallel_shocks[cc] = bump_size;
        
        market::ScenarioEngine::apply_scenario_to_state(*state, base_market_dto, parallel_up);
        for (auto& [inst_id, inst] : ql_instruments) {
            if (inst_id == id) {
                res.pv01 = (inst->NPV() - base_npv);
                break;
            }
        }
        // Reset state back to base after parallel shock.
        for (const auto& q : base_market_dto.quotes) state->add_quote(q.id, q.value);

        // --- 2. CS01 (Credit Spread Risk) ---
        // We shock all CDS quotes in the market. 
        // Note: In a production system, we would map specific issuers/entities to the trade.
        market::ScenarioDefinition credit_up;
        for (const auto& q : base_market_dto.quotes) {
            if (q.instrument_type == domain::QuoteInstrumentType::CDS) {
                credit_up.credit_shocks[q.id] = bump_size;
            }
        }
        if (!credit_up.credit_shocks.empty()) {
            market::ScenarioEngine::apply_scenario_to_state(*state, base_market_dto, credit_up);
            for (auto& [inst_id, inst] : ql_instruments) {
                if (inst_id == id) {
                    res.cs01 = (inst->NPV() - base_npv);
                    break;
                }
            }
            // Reset state after credit shock.
            for (const auto& q : base_market_dto.quotes) state->add_quote(q.id, q.value);
        }

        // --- 3. Bucketed Key-Rate Risk (Delta/DV01 Buckets) ---
        // We shock each market quote (node) individually to build the risk profile across the curve.
        // This identifies the trade's sensitivity to specific tenors (e.g., 2Y vs 10Y).
        
        // Collect all unique quotes that contribute to this currency's risk.
        std::set<std::string> quote_ids_for_cc;
        for (const auto& q : base_market_dto.quotes) {
            if (domain::to_string(q.currency) == cc) {
                quote_ids_for_cc.insert(q.id);
            }
        }

        for (const auto& q_id : quote_ids_for_cc) {
            const auto& q = *std::find_if(base_market_dto.quotes.begin(), base_market_dto.quotes.end(),
                                          [&](const auto& item) { return item.id == q_id; });
            
            market::ScenarioDefinition node_shock;
            node_shock.node_shocks[cc][q_id] = bump_size;

            // Apply granular shock to a single node.
            market::ScenarioEngine::apply_scenario_to_state(*state, base_market_dto, node_shock);
            for (auto& [inst_id, inst] : ql_instruments) {
                if (inst_id == id) {
                    // Accumulate risk by tenor. Multiple quotes might share a tenor (less common but possible).
                    res.bucketed_risk[q.tenor] += (inst->NPV() - base_npv);
                    break;
                }
            }
            // Reset state to base after each node shock to ensure buckets are independent (non-cumulative).
            for (const auto& quote_to_reset : base_market_dto.quotes) {
                state->add_quote(quote_to_reset.id, quote_to_reset.value);
            }
        }
        results.push_back(res);
    }
    return results;
}

} // namespace qrp::analytics
