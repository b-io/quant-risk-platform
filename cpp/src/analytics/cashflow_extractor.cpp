// Implements cashflow extraction hooks for supported instrument diagnostics.

#include <qrp/analytics/cashflow_extractor.hpp>
#include <qrp/analytics/product_pricing_registry.hpp>

namespace qrp::analytics {

CashflowExtractionResult CashflowExtractor::extract_realized_cashflows(
    const domain::Trade& trade,
    const domain::MarketSnapshot& previous_market,
    const domain::MarketSnapshot& current_market) {
    return ProductPricingRegistry::extract_realized_cashflows(trade, previous_market, current_market);
}

} // namespace qrp::analytics
