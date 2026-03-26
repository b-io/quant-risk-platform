#include <qrp/analytics/stress_engine.hpp>
#include <qrp/instruments/instrument_factory.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <ql/instrument.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace qrp::analytics {

std::vector<StressResult> StressEngine::run_historical_stress(
    const domain::Portfolio& portfolio,
    const domain::MarketSnapshot& base_market_dto,
    const std::vector<market::ScenarioDefinition>& historical_scenarios) {

    std::vector<StressResult> results;
    
    // 1. Setup Base Market and Instrument Cache
    qrp::market::MarketSnapshot base_market(base_market_dto);
    auto state = base_market.built_state();
    PricingContext context(state);

    std::vector<std::pair<std::string, QuantLib::ext::shared_ptr<QuantLib::Instrument>>> instruments;
    std::map<std::string, double> base_map;
    double base_total = 0.0;
    for (const auto& trade : portfolio.trades) {
        auto inst = instruments::InstrumentFactory::create_instrument(trade, context);
        if (inst) {
            instruments.push_back({trade.id, inst});
            double npv = inst->NPV();
            base_map[trade.id] = npv;
            base_total += npv;
        }
    }

    // 2. Iterate over scenarios using direct handle bumping
    for (const auto& scenario : historical_scenarios) {
        StressResult res;
        res.scenario_name = scenario.name;
        
        market::ScenarioEngine::apply_scenario_to_state(*state, base_market_dto, scenario);
        
        double shocked_total = 0.0;
        for (auto& [id, inst] : instruments) {
            double npv = inst->NPV();
            res.trade_pnls[id] = npv - base_map[id];
            shocked_total += npv;
        }
        res.total_pnl = shocked_total - base_total;
        results.push_back(res);

        // Reset state for next scenario
        for (const auto& q : base_market_dto.quotes) state->add_quote(q.id, q.value);
    }
    
    return results;
}

} // namespace qrp::analytics
