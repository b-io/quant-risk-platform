#pragma once
#include <qrp/domain/market_data.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <map>

namespace qrp::market {

struct ScenarioDefinition {
    std::string name;
    std::map<std::string, double> parallel_shocks; // currency -> bump
    std::map<std::string, std::map<std::string, double>> node_shocks; // currency -> {tenor -> bump}
};

class ScenarioEngine {
public:
    static domain::MarketSnapshot apply_scenario(
        const domain::MarketSnapshot& base_market,
        const ScenarioDefinition& scenario) {

        domain::MarketSnapshot shocked = base_market;

        for (auto& [asset_class, market_objs] : shocked.markets) {
            for (auto& [name, curve_def] : market_objs) {
                // Apply parallel shock if currency matches
                if (scenario.parallel_shocks.contains(curve_def.currency)) {
                    double bump = scenario.parallel_shocks.at(curve_def.currency);
                    for (auto& node : curve_def.nodes) {
                        node.value += bump;
                    }
                }

                // Apply node shocks
                if (scenario.node_shocks.contains(curve_def.currency)) {
                    const auto& nodes_to_shock = scenario.node_shocks.at(curve_def.currency);
                    for (auto& node : curve_def.nodes) {
                        if (nodes_to_shock.contains(node.tenor)) {
                            node.value += nodes_to_shock.at(node.tenor);
                        }
                    }
                }
            }
        }
        return shocked;
    }
};

} // namespace qrp::market
