#pragma once
#include <qrp/analytics/valuation_service.hpp>
#include <vector>

namespace qrp::analytics {

struct PnlExplainResult {
    std::string trade_id;
    double prev_npv;
    double curr_npv;
    double total_pnl;
    double market_move_pnl;
    double residual;
};

class PnlExplainService {
public:
    static std::vector<PnlExplainResult> explain_pnl(
        const domain::Portfolio& portfolio,
        const domain::MarketSnapshot& prev_market_dto,
        const domain::MarketSnapshot& curr_market_dto) {

        market::MarketSnapshot prev_market(prev_market_dto);
        market::MarketSnapshot curr_market(curr_market_dto);

        auto prev_results = ValuationService::price_portfolio(portfolio, prev_market);
        auto curr_results = ValuationService::price_portfolio(portfolio, curr_market);

        std::map<std::string, double> prev_map, curr_map;
        for (const auto& r : prev_results) prev_map[r.trade_id] = r.npv;
        for (const auto& r : curr_results) curr_map[r.trade_id] = r.npv;

        std::vector<PnlExplainResult> results;
        for (const auto& trade : portfolio.trades) {
            if (prev_map.contains(trade.id) && curr_map.contains(trade.id)) {
                PnlExplainResult res;
                res.trade_id = trade.id;
                res.prev_npv = prev_map[trade.id];
                res.curr_npv = curr_map[trade.id];
                res.total_pnl = res.curr_npv - res.prev_npv;
                
                // For a simple explain, market move pnl is the difference in NPV
                // In a more complex setup, we'd decompose by factor (Delta, Gamma, etc.)
                res.market_move_pnl = res.total_pnl; 
                res.residual = 0.0; 
                
                results.push_back(res);
            }
        }
        return results;
    }
};

} // namespace qrp::analytics
