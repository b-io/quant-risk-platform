#pragma once
#include <qrp/market/market_state.hpp>
#include <qrp/domain/types.hpp>
#include <memory>

namespace qrp::analytics {

/**
 * @brief PricingContext holds the market state and any pricing-specific configurations.
 * It acts as the "environment" in which instruments are priced.
 */
class PricingContext {
public:
    explicit PricingContext(std::shared_ptr<market::MarketState> market_state)
        : market_state_(std::move(market_state)) {}

    const market::MarketState& market_state() const { return *market_state_; }
    std::shared_ptr<market::MarketState> market_state_ptr() const { return market_state_; }

    // Logic to map a trade to its required curves can live here
    domain::CurveId get_discount_curve_id(domain::Currency currency) const {
        // Use the standard family name present in sample data and conventions
        // See docs/design/CURVE_BOOTSTRAP_DESIGN.md for curve family naming
        return {currency, "OIS"};
    }

    domain::CurveId get_forecast_curve_id(domain::Currency currency, const std::string& index_family) const {
        return {currency, index_family};
    }

private:
    std::shared_ptr<market::MarketState> market_state_;
};

} // namespace qrp::analytics
