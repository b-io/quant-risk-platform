#pragma once
#include <qrp/analytics/pricing_context.hpp>
#include <qrp/domain/portfolio.hpp>
#include <map>
#include <vector>

namespace qrp::analytics {

struct ValuationResult {
    std::string trade_id;
    double npv;
    std::string currency;
    std::map<std::string, std::string> tags;
};

class ValuationService {
public:
    static std::vector<ValuationResult> price_portfolio(
        const domain::Portfolio& portfolio,
        const PricingContext& context);
};

} // namespace qrp::analytics
