#pragma once

#include <ql/instrument.hpp>
#include <ql/shared_ptr.hpp>
#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/pricing_profiles.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/portfolio.hpp>
#include <functional>

namespace qrp::analytics {

struct ProductPricingDefinition {
    using CashflowExtractor = std::function<CashflowExtractionResult(
        const domain::Trade&,
        const domain::MarketSnapshot&,
        const domain::MarketSnapshot&)>;
    using InstrumentBuilder = std::function<QuantLib::ext::shared_ptr<QuantLib::Instrument>(
        const domain::Trade&,
        const PricingContext&)>;
    using PricingProfileBuilder = std::function<InstrumentPricingProfile(const domain::Trade&)>;

    CashflowExtractor cashflow_extractor;
    InstrumentBuilder instrument_builder;
    std::string model_name;
    PricingProfileBuilder pricing_profile_builder;
    domain::ProductType product_type = domain::ProductType::Unknown;
    std::string reason;
    domain::SupportStatus status = domain::SupportStatus::Unsupported;
};

class ProductPricingRegistry {
public:
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_instrument(
        const domain::Trade& trade,
        const PricingContext& context);

    static CashflowExtractionResult extract_realized_cashflows(
        const domain::Trade& trade,
        const domain::MarketSnapshot& previous_market,
        const domain::MarketSnapshot& current_market);

    static const ProductPricingDefinition& definition_for(const domain::Trade& trade);

    static InstrumentPricingProfile pricing_profile(const domain::Trade& trade);

    static ProductSupportProfile support_profile(const domain::Trade& trade);
};

} // namespace qrp::analytics
