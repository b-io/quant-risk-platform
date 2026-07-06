#pragma once

// Declares the registry of product support, pricing models, and required market inputs.

#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/pricing_profiles.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/portfolio.hpp>

#include <functional>
#include <ql/instrument.hpp>
#include <ql/shared_ptr.hpp>

namespace qrp::analytics {

/**
 * @brief Registry entry describing one product's pricing and support behavior.
 */
struct ProductPricingDefinition {
    /**
     * @brief Extracts realized cashflows for PnL explain and reporting.
     */
    using CashflowExtractor = std::function<
        CashflowExtractionResult(const domain::Trade&, const domain::MarketSnapshot&, const domain::MarketSnapshot&)>;

    /**
     * @brief Builds the QuantLib instrument for a canonical trade.
     */
    using InstrumentBuilder =
        std::function<QuantLib::ext::shared_ptr<QuantLib::Instrument>(const domain::Trade&, const PricingContext&)>;

    /**
     * @brief Builds product-specific NPV normalization adjustments.
     */
    using PricingProfileBuilder = std::function<InstrumentPricingProfile(const domain::Trade&)>;

    CashflowExtractor cashflow_extractor;                              // Cashflow extraction hook.
    InstrumentBuilder instrument_builder;                              // QuantLib instrument construction hook.
    std::string model_name;                                            // Pricing model or approximation name.
    PricingProfileBuilder pricing_profile_builder;                     // NPV normalization hook.
    domain::ProductType product_type = domain::ProductType::Unknown;   // Registered product type.
    std::string reason;                                                // Human-readable support reason.
    domain::SupportStatus status = domain::SupportStatus::Unsupported; // Registered support status.
};

/**
 * @brief Central lookup for product support, instrument builders, and pricing profiles.
 */
class ProductPricingRegistry {
public:
    /**
     * @brief Creates the QuantLib instrument for a supported trade, or nullptr when unsupported.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_instrument(const domain::Trade& trade,
                                                                             const PricingContext& context);

    /**
     * @brief Extracts realized cashflows using the product's registered extraction hook.
     */
    static CashflowExtractionResult extract_realized_cashflows(const domain::Trade& trade,
                                                               const domain::MarketSnapshot& previous_market,
                                                               const domain::MarketSnapshot& current_market);

    /**
     * @brief Returns the immutable product-pricing definition for a trade.
     */
    static const ProductPricingDefinition& definition_for(const domain::Trade& trade);

    /**
     * @brief Returns product-specific adjustments used after instrument pricing.
     */
    static InstrumentPricingProfile pricing_profile(const domain::Trade& trade);

    /**
     * @brief Returns user-facing support status and model metadata for a trade.
     */
    static ProductSupportProfile support_profile(const domain::Trade& trade);
};

} // namespace qrp::analytics
