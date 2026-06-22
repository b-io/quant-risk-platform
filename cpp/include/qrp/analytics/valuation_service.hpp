#pragma once
#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/pricing_profiles.hpp>
#include <qrp/domain/portfolio.hpp>
#include <vector>

namespace QuantLib {
class Instrument;
}

namespace qrp::analytics {

class ValuationService {
public:
    static ProductSupportProfile support_profile(const domain::Trade& trade);

    static InstrumentPricingProfile pricing_profile(const domain::Trade& trade);

    static double price_instrument(
        const domain::Trade& trade,
        const QuantLib::Instrument& instrument);

    static std::vector<ValuationResult> price_portfolio(
        const domain::Portfolio& portfolio,
        const PricingContext& context);
};

} // namespace qrp::analytics
