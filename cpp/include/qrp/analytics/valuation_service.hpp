#pragma once

// Declares valuation orchestration for trades, portfolios, and pricing-support diagnostics.

#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/pricing_profiles.hpp>
#include <qrp/domain/portfolio.hpp>

#include <vector>

namespace QuantLib {
class Instrument;
}

namespace qrp::analytics {

/**
 * @brief Prices trades and portfolios while preserving product-support diagnostics.
 */
class ValuationService {
public:
    /**
     * @brief Returns the support metadata declared for a trade's product type.
     */
    static ProductSupportProfile support_profile(const domain::Trade& trade);

    /**
     * @brief Returns product-specific NPV adjustments used when normalizing QuantLib prices.
     */
    static InstrumentPricingProfile pricing_profile(const domain::Trade& trade);

    /**
     * @brief Prices a constructed QuantLib instrument using the trade's pricing profile.
     */
    static double price_instrument(
        const domain::Trade& trade,
        const QuantLib::Instrument& instrument);

    /**
     * @brief Builds and prices each supported trade in the portfolio.
     */
    static std::vector<ValuationResult> price_portfolio(
        const domain::Portfolio& portfolio,
        const PricingContext& context);
};

} // namespace qrp::analytics
