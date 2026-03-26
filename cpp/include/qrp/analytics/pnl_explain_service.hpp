#pragma once
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/domain/market_data.hpp>
#include <vector>

namespace qrp::analytics {

struct PnlExplainResult {
    std::string trade_id;
    double prev_npv;
    double curr_npv;
    double total_pnl;
    
    // Decomposed drivers
    double carry_pnl;       // P&L from time decay / roll-down
    double market_move_pnl; // P&L from market factor changes
    double cash_pnl;        // P&L from realized cashflows (coupons, fixings)
    double residual;        // Unexplained P&L
};

class PnlExplainService {
public:
    static std::vector<PnlExplainResult> explain_pnl(
        const domain::Portfolio& portfolio,
        const domain::MarketSnapshot& prev_market_dto,
        const domain::MarketSnapshot& curr_market_dto);
};

} // namespace qrp::analytics
