// Implements deterministic sensitivity-style risk measures over the valuation service.

#include <qrp/analytics/risk_service.hpp>
#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/instruments/instrument_factory.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <ql/instrument.hpp>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
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

namespace {

struct RiskInstrument {
    const domain::Trade* trade = nullptr;
    QuantLib::ext::shared_ptr<QuantLib::Instrument> instrument;
};

} // namespace

/**
 * @brief Computes risk metrics for a given portfolio and market state.
 * @param portfolio The portfolio of trades to evaluate.
 * @param base_market_dto The base market data used as the reference state.
 * @return A vector of RiskResult containing PV01, CS01, and bucketed risk per trade.
 */
std::vector<RiskResult> RiskService::compute_risk(
    const domain::Portfolio& portfolio,
    const domain::MarketSnapshot& base_market_dto,
    const std::vector<domain::FactorDefinition>& factors,
    const std::vector<domain::FactorBinding>& bindings) {

    // 1bp shift is the industry standard for PV01/DV01 calculations.
    const double bump_size = 0.0001; 
    std::vector<RiskResult> results;

    qrp::market::MarketSnapshot base_market(base_market_dto);
    auto state = base_market.built_state();
    PricingContext context(state);

    std::vector<RiskInstrument> ql_instruments;
    for (const auto& trade_ptr : portfolio.trades) {
        const auto& trade = *trade_ptr;
        auto inst = instruments::InstrumentFactory::create_instrument(trade, context);
        if (inst) ql_instruments.push_back({&trade, inst});
    }

    std::map<std::string, double> base_map;
    for (const auto& priced : ql_instruments) {
        base_map[priced.trade->id] = ValuationService::price_instrument(*priced.trade, *priced.instrument);
    }

    if (factors.empty() || bindings.empty()) {
        throw std::runtime_error("RiskService: Factors and bindings are required for risk computation");
    }

    for (const auto& priced : ql_instruments) {
        const auto& trade = *priced.trade;
        const auto& id = trade.id;
        const auto base_npv = base_map.at(id);
        RiskResult res;
        res.trade_id = id;
        res.pv01 = 0.0;
        res.cs01 = 0.0;

        std::string cc = trade.currency;

        // --- Factor-based Risk ---
        
        // 1. PV01: Shock all rate factors for the trade currency
        market::ScenarioDefinition pv01_scenario;
        pv01_scenario.name = "PV01";
        for (const auto& f : factors) {
            if (domain::to_string(f.currency) == cc && 
                (f.factor_type == domain::FactorType::RateZero || f.factor_type == domain::FactorType::RateForward)) {
                pv01_scenario.factor_shocks[f.factor_id] = bump_size;
            }
        }
        if (!pv01_scenario.factor_shocks.empty()) {
            market::ScenarioEngine::apply_scenario_to_state(*state, base_market_dto, pv01_scenario, factors, bindings);
            double bumped_npv = ValuationService::price_instrument(trade, *priced.instrument);
            res.pv01 = bumped_npv - base_npv;
            state->reset_to_snapshot(base_market_dto);
        }

        // 2. CS01: Shock all credit factors
        market::ScenarioDefinition cs01_scenario;
        cs01_scenario.name = "CS01";
        for (const auto& f : factors) {
            if (f.factor_type == domain::FactorType::CreditSpread || f.factor_type == domain::FactorType::HazardRate) {
                cs01_scenario.factor_shocks[f.factor_id] = bump_size;
            }
        }
        if (!cs01_scenario.factor_shocks.empty()) {
            market::ScenarioEngine::apply_scenario_to_state(*state, base_market_dto, cs01_scenario, factors, bindings);
            double bumped_npv = ValuationService::price_instrument(trade, *priced.instrument);
            res.cs01 = bumped_npv - base_npv;
            state->reset_to_snapshot(base_market_dto);
        }

        // 3. Bucketed Risk: Shock each factor individually
        for (const auto& f : factors) {
            if (domain::to_string(f.currency) == cc && 
                (f.factor_type == domain::FactorType::RateZero || f.factor_type == domain::FactorType::RateForward)) {
                
                market::ScenarioDefinition bucket_scenario;
                bucket_scenario.name = "Bucket_" + f.factor_id;
                bucket_scenario.factor_shocks[f.factor_id] = bump_size;

                market::ScenarioEngine::apply_scenario_to_state(*state, base_market_dto, bucket_scenario, factors, bindings);
                double bumped_npv = ValuationService::price_instrument(trade, *priced.instrument);
                std::string label = f.tenor.empty() ? f.factor_id : f.tenor;
                res.bucketed_risk[label] += bumped_npv - base_npv;
                state->reset_to_snapshot(base_market_dto);
            }
        }
        results.push_back(res);
    }
    return results;
}

} // namespace qrp::analytics
