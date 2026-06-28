#pragma once

// Declares cashflow extraction helpers for instrument-level diagnostics and reporting.

#include <qrp/analytics/pricing_profiles.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/portfolio.hpp>

namespace qrp::analytics {

/**
 * @brief Extracts realized cashflows between two market snapshots for supported trades.
 */
class CashflowExtractor {
public:
    /**
     * @brief Returns realized cashflow diagnostics for the interval bounded by two market snapshots.
     */
    static CashflowExtractionResult extract_realized_cashflows(
        const domain::Trade& trade,
        const domain::MarketSnapshot& previous_market,
        const domain::MarketSnapshot& current_market);
};

} // namespace qrp::analytics
