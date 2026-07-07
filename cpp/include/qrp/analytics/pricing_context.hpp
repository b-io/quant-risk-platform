#pragma once

// Declares the market-state wrapper passed into pricing and instrument construction.

#include <qrp/domain/types.hpp>
#include <qrp/market/market_state.hpp>

#include <memory>

namespace qrp::analytics {

/**
 * @brief PricingContext holds the market state and any pricing-specific configurations.
 * It acts as the "environment" in which instruments are priced.
 */
class PricingContext {
public:
    /**
     * @brief Creates a pricing context over a built market state.
     */
    explicit PricingContext(std::shared_ptr<market::MarketState> market_state);

    /**
     * @brief Returns the referenced market state.
     */
    const market::MarketState& market_state() const;

    /**
     * @brief Returns the shared market-state handle for downstream builders.
     */
    std::shared_ptr<market::MarketState> market_state_ptr() const;

    /**
     * @brief Resolves the standard discount curve for a currency.
     */
    domain::CurveId get_discount_curve_id(domain::Currency currency) const;

    /**
     * @brief Resolves a forecast curve by currency and index family.
     */
    domain::CurveId get_forecast_curve_id(domain::Currency currency, const std::string& index_family) const;

private:
    std::shared_ptr<market::MarketState> market_state_; // Shared built market state used for pricing.
};

} // namespace qrp::analytics
