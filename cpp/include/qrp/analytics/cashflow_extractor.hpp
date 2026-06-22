#pragma once

#include <qrp/analytics/pricing_profiles.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/portfolio.hpp>

namespace qrp::analytics {

class CashflowExtractor {
public:
    static CashflowExtractionResult extract_realized_cashflows(
        const domain::Trade& trade,
        const domain::MarketSnapshot& previous_market,
        const domain::MarketSnapshot& current_market);
};

} // namespace qrp::analytics
