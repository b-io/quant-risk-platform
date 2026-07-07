// Implements the market-state wrapper passed into pricing and instrument construction.

#include <qrp/analytics/pricing_context.hpp>

#include <utility>

namespace qrp::analytics {

PricingContext::PricingContext(std::shared_ptr<market::MarketState> market_state)
    : market_state_(std::move(market_state)) {}

const market::MarketState& PricingContext::market_state() const {
    return *market_state_;
}

std::shared_ptr<market::MarketState> PricingContext::market_state_ptr() const {
    return market_state_;
}

domain::CurveId PricingContext::get_discount_curve_id(domain::Currency currency) const {
    return {currency, "OIS"};
}

domain::CurveId PricingContext::get_forecast_curve_id(domain::Currency currency,
                                                      const std::string& index_family) const {
    return {currency, index_family};
}

} // namespace qrp::analytics
