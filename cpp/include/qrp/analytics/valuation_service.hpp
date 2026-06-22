#pragma once
#include <qrp/analytics/pricing_context.hpp>
#include <qrp/domain/portfolio.hpp>
#include <map>
#include <vector>

namespace QuantLib {
class Instrument;
}

namespace qrp::analytics {

struct ValuationResult {
    std::string trade_id;
    double npv;
    std::string currency;
    std::map<std::string, std::string> tags;
};

struct InstrumentPricingProfile {
    double multiplier = 1.0;
    double additive_npv = 0.0;
};

class ValuationService {
public:
    static InstrumentPricingProfile pricing_profile(const domain::Trade& trade);

    static double price_instrument(
        const domain::Trade& trade,
        const QuantLib::Instrument& instrument);

    static std::vector<ValuationResult> price_portfolio(
        const domain::Portfolio& portfolio,
        const PricingContext& context);
};

} // namespace qrp::analytics
