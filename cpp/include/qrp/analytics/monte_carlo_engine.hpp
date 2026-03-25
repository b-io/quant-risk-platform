#pragma once
#include <qrp/market/scenario_engine.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/portfolio.hpp>
#include <vector>
#include <string>

namespace qrp::analytics {

struct SimulationResult {
    std::vector<double> portfolio_values;
    double var_95;
    double expected_shortfall_95;
};

class MonteCarloEngine {
public:
    static SimulationResult run_simulation(
        const domain::Portfolio& portfolio,
        const domain::MarketSnapshot& base_market_dto,
        int num_paths);
};

} // namespace qrp::analytics
