#pragma once
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/market/scenario_engine.hpp>
#include <vector>

namespace qrp::analytics {

struct RiskResult {
    std::string trade_id;
    double pv01; // Parallel shock
    std::map<std::string, double> bucketed_risk; // Tenor -> shock
};

class RiskService {
public:
    static std::vector<RiskResult> compute_risk(
        const domain::Portfolio& portfolio,
        const domain::MarketSnapshot& base_market_dto) {

        double bump_size = 0.0001; // 1bp
        std::vector<RiskResult> results;

        // Base NPV
        market::MarketSnapshot base_market(base_market_dto);
        auto base_npvs = ValuationService::price_portfolio(portfolio, base_market);

        // Parallel Shock
        market::ScenarioDefinition parallel_up;
        parallel_up.name = "parallel_up";
        parallel_up.parallel_shocks["USD"] = bump_size;

        auto shocked_dto = market::ScenarioEngine::apply_scenario(base_market_dto, parallel_up);
        market::MarketSnapshot shocked_market(shocked_dto);
        auto shocked_npvs = ValuationService::price_portfolio(portfolio, shocked_market);

        // Map NPVs for easy comparison
        std::map<std::string, double> base_map, shocked_map;
        for (const auto& r : base_npvs) base_map[r.trade_id] = r.npv;
        for (const auto& r : shocked_npvs) shocked_map[r.trade_id] = r.npv;

        for (const auto& [id, npv] : base_map) {
            RiskResult res;
            res.trade_id = id;
            res.pv01 = (shocked_map[id] - npv);

            // Compute bucketed risk (key-rate)
            std::vector<std::string> tenors = {"1M", "3M", "6M", "1Y", "2Y", "5Y", "10Y", "30Y"};
            for (const auto& t : tenors) {
                market::ScenarioDefinition node_shock;
                node_shock.name = "node_shock_" + t;
                node_shock.node_shocks["USD"][t] = bump_size;
                auto n_shocked_dto = market::ScenarioEngine::apply_scenario(base_market_dto, node_shock);
                market::MarketSnapshot n_shocked_market(n_shocked_dto);
                auto n_shocked_npvs = ValuationService::price_portfolio(portfolio, n_shocked_market);

                for (const auto& nr : n_shocked_npvs) {
                    if (nr.trade_id == id) {
                        res.bucketed_risk[t] = (nr.npv - npv);
                        break;
                    }
                }
            }
            results.push_back(res);
        }
        return results;
    }
};

} // namespace qrp::analytics
