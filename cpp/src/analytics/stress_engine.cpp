#include <qrp/analytics/stress_engine.hpp>
#include <qrp/analytics/pricing_context.hpp>
#include <qrp/instruments/instrument_factory.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <ql/instrument.hpp>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace qrp::analytics {

namespace {

struct StressInstrument {
    const domain::Trade* trade = nullptr;
    QuantLib::ext::shared_ptr<QuantLib::Instrument> instrument;
};

} // namespace

std::vector<StressResult> StressEngine::run_historical_stress(
    const domain::Portfolio& portfolio,
    const domain::MarketSnapshot& base_market_dto,
    const std::vector<market::ScenarioDefinition>& historical_scenarios,
    const std::vector<domain::FactorDefinition>& factors,
    const std::vector<domain::FactorBinding>& bindings) {

    std::vector<StressResult> results;
    
    // 1. Setup Base Market and Instrument Cache
    qrp::market::MarketSnapshot base_market(base_market_dto);
    auto state = base_market.built_state();
    PricingContext context(state);

    std::vector<StressInstrument> instruments;
    std::map<std::string, double> base_map;
    double base_total = 0.0;
    for (const auto& trade_ptr : portfolio.trades) {
        const auto& trade = *trade_ptr;
        if (auto inst = instruments::InstrumentFactory::create_instrument(trade, context)) {
            instruments.push_back({&trade, inst});
            double npv = ValuationService::price_instrument(trade, *inst);
            base_map[trade.id] = npv;
            base_total += npv;
        }
    }

    if (factors.empty() || bindings.empty()) {
        throw std::runtime_error("StressEngine: Factors and bindings are required for historical stress");
    }

    // 2. Iterate over scenarios using direct handle bumping
    for (const auto& scenario : historical_scenarios) {
        StressResult res;
        res.scenario_name = scenario.name;
        
        market::ScenarioEngine::apply_scenario_to_state(*state, base_market_dto, scenario, factors, bindings);
        
        double shocked_total = 0.0;
        for (const auto& priced : instruments) {
            const auto& trade = *priced.trade;
            double npv = ValuationService::price_instrument(trade, *priced.instrument);
            res.trade_pnls[trade.id] = npv - base_map[trade.id];
            shocked_total += npv;
        }
        res.total_pnl = shocked_total - base_total;
        results.push_back(res);

        // Reset state for next scenario
        state->reset_to_snapshot(base_market_dto);
    }
    
    return results;
}

} // namespace qrp::analytics
