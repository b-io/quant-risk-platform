#pragma once
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/market/scenario_engine.hpp>
#include <vector>

namespace qrp::analytics {

struct RiskResult {
    std::string trade_id;
    double pv01; // Parallel rate shock
    double cs01; // Parallel credit shock
    std::map<std::string, double> bucketed_risk; // Tenor -> shock
};

class RiskService {
public:
    static std::vector<RiskResult> compute_risk(
        const domain::Portfolio& portfolio,
        const domain::MarketSnapshot& base_market_dto);
};

} // namespace qrp::analytics
