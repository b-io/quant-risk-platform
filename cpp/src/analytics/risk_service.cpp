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

bool applies_to_trade_currency(const domain::FactorDefinition& factor, const std::string& currency) {
    return factor.currency == domain::Currency::UNKNOWN || domain::to_string(factor.currency) == currency;
}

bool is_rates_factor(domain::FactorType factor_type) {
    return factor_type == domain::FactorType::RateZero || factor_type == domain::FactorType::RateForward;
}

bool is_fx_vol_factor(const domain::FactorDefinition& factor) {
    return factor.factor_type == domain::FactorType::Volatility &&
           factor.factor_id.rfind("RF:FXVOL:", 0) == 0;
}

std::string factor_bucket_label(const domain::FactorDefinition& factor) {
    return factor.tenor.empty() ? factor.factor_id : factor.tenor;
}

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
    const double fx_spot_bump = 0.0001;
    const double fx_vol_bump = 1.0;
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

        std::string cc = trade.currency;

        // --- Factor-based Risk ---

        // 1. PV01: Shock all rate factors for the trade currency
        market::ScenarioDefinition pv01_scenario;
        pv01_scenario.name = "PV01";
        for (const auto& f : factors) {
            if (applies_to_trade_currency(f, cc) && is_rates_factor(f.factor_type)) {
                pv01_scenario.factor_shocks[f.factor_id] = bump_size;
            }
        }
        if (!pv01_scenario.factor_shocks.empty()) {
            market::ScenarioEngine::apply_scenario_to_state(*state, base_market_dto, pv01_scenario, factors, bindings);
            double bumped_npv = ValuationService::price_instrument(trade, *priced.instrument);
            res.pv01 = bumped_npv - base_npv;
            state->reset_to_snapshot(base_market_dto);
        }

        // 2. CS01: Shock all credit factors.
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

        // 3. FX delta: Shock configured FX spot factors.
        market::ScenarioDefinition fx_delta_scenario;
        fx_delta_scenario.name = "FX_DELTA";
        for (const auto& f : factors) {
            if (applies_to_trade_currency(f, cc) && f.factor_type == domain::FactorType::FXSpot) {
                fx_delta_scenario.factor_shocks[f.factor_id] = fx_spot_bump;
            }
        }
        if (!fx_delta_scenario.factor_shocks.empty()) {
            market::ScenarioEngine::apply_scenario_to_state(*state, base_market_dto, fx_delta_scenario, factors, bindings);
            double bumped_npv = ValuationService::price_instrument(trade, *priced.instrument);
            res.fx_delta = bumped_npv - base_npv;
            state->reset_to_snapshot(base_market_dto);
        }

        // 4. FX vega: Shock configured FX volatility surface factors by one vol point.
        market::ScenarioDefinition fx_vega_scenario;
        fx_vega_scenario.name = "FX_VEGA";
        for (const auto& f : factors) {
            if (applies_to_trade_currency(f, cc) && is_fx_vol_factor(f)) {
                fx_vega_scenario.factor_shocks[f.factor_id] = fx_vol_bump;
            }
        }
        if (!fx_vega_scenario.factor_shocks.empty()) {
            market::ScenarioEngine::apply_scenario_to_state(*state, base_market_dto, fx_vega_scenario, factors, bindings);
            double bumped_npv = ValuationService::price_instrument(trade, *priced.instrument);
            res.fx_vega = bumped_npv - base_npv;
            state->reset_to_snapshot(base_market_dto);
        }

        // 5. Bucketed Risk: Shock each supported factor individually.
        for (const auto& f : factors) {
            if (applies_to_trade_currency(f, cc) && (is_rates_factor(f.factor_type) || f.factor_type == domain::FactorType::FXSpot || is_fx_vol_factor(f))) {
                market::ScenarioDefinition bucket_scenario;
                bucket_scenario.name = "Bucket_" + f.factor_id;
                bucket_scenario.factor_shocks[f.factor_id] = is_fx_vol_factor(f) ? fx_vol_bump : bump_size;

                market::ScenarioEngine::apply_scenario_to_state(*state, base_market_dto, bucket_scenario, factors, bindings);
                double bumped_npv = ValuationService::price_instrument(trade, *priced.instrument);
                res.bucketed_risk[factor_bucket_label(f)] += bumped_npv - base_npv;
                state->reset_to_snapshot(base_market_dto);
            }
        }
        results.push_back(res);
    }
    return results;
}

} // namespace qrp::analytics
