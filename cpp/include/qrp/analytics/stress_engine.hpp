#pragma once
#include <qrp/market/scenario_engine.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/portfolio.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <vector>
#include <string>

namespace qrp::analytics {

struct StressResult {
    std::string scenario_name;
    double total_pnl;
    std::map<std::string, double> trade_pnls;
};

class StressEngine {
public:
    static std::vector<StressResult> run_historical_stress(
        const domain::Portfolio& portfolio,
        const domain::MarketSnapshot& base_market_dto,
        const std::vector<market::ScenarioDefinition>& historical_scenarios);
};

} // namespace qrp::analytics
