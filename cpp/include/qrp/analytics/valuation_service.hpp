#pragma once
#include <qrp/analytics/pricing_context.hpp>
#include <qrp/domain/portfolio.hpp>
#include <qrp/domain/product.hpp>
#include <map>
#include <string>
#include <vector>

namespace QuantLib {
class Instrument;
}

namespace qrp::analytics {

struct ValuationResult {
    std::string trade_id;
    double npv;
    std::string currency;
    domain::AssetClass asset_class = domain::AssetClass::Unknown;
    domain::ProductType product_type = domain::ProductType::Unknown;
    domain::SupportStatus support_status = domain::SupportStatus::Failed;
    std::string model_name;
    std::string status_message;
    std::map<std::string, std::string> tags;
};

struct InstrumentPricingProfile {
    double multiplier = 1.0;
    double additive_npv = 0.0;
};

struct ProductSupportProfile {
    domain::AssetClass asset_class = domain::AssetClass::Unknown;
    domain::ProductType product_type = domain::ProductType::Unknown;
    domain::SupportStatus status = domain::SupportStatus::Unsupported;
    std::string model_name;
    std::string reason;
};

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
