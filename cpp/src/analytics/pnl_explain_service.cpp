#include <qrp/analytics/pnl_explain_service.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <map>
#include <memory>

namespace qrp::analytics {

std::vector<PnlExplainResult> PnlExplainService::explain_pnl(
    const domain::Portfolio& portfolio,
    const domain::MarketSnapshot& prev_market_dto,
    const domain::MarketSnapshot& curr_market_dto) {

    qrp::market::MarketSnapshot prev_market(prev_market_dto);
    qrp::market::MarketSnapshot curr_market(curr_market_dto);

    auto prev_state = prev_market.built_state();
    auto curr_state = curr_market.built_state();

    PricingContext prev_context(prev_state);
    PricingContext curr_context(curr_state);

    // Roll date context: t1 valuation date but using t0 market data
    QuantLib::Date t0 = prev_state->valuation_date();
    QuantLib::Date t1 = curr_state->valuation_date();
    
    // Create a temporary state for carry calculation
    auto rolled_state = std::make_shared<qrp::market::MarketState>(t1);
    // Copy all curves and handles from prev_state to rolled_state
    // In a production system, we'd ensure handles point to correct data.
    // Here, we can rebuild the prev_market at t1
    domain::MarketSnapshot rolled_dto = prev_market_dto;
    rolled_dto.valuation_date = curr_market_dto.valuation_date;
    qrp::market::MarketSnapshot rolled_market(rolled_dto);
    PricingContext rolled_context(rolled_market.built_state());

    auto prev_results = ValuationService::price_portfolio(portfolio, prev_context);
    auto curr_results = ValuationService::price_portfolio(portfolio, curr_context);
    auto rolled_results = ValuationService::price_portfolio(portfolio, rolled_context);

    std::map<std::string, double> prev_map, curr_map, rolled_map;
    for (const auto& r : prev_results) prev_map[r.trade_id] = r.npv;
    for (const auto& r : curr_results) curr_map[r.trade_id] = r.npv;
    for (const auto& r : rolled_results) rolled_map[r.trade_id] = r.npv;

    std::vector<PnlExplainResult> results;
    for (const auto& trade : portfolio.trades) {
        if (prev_map.contains(trade.id) && curr_map.contains(trade.id)) {
            PnlExplainResult res;
            res.trade_id = trade.id;
            res.prev_npv = prev_map[trade.id];
            res.curr_npv = curr_map[trade.id];
            res.total_pnl = res.curr_npv - res.prev_npv;
            
            double rolled_npv = rolled_map.contains(trade.id) ? rolled_map[trade.id] : res.prev_npv;
            
            // 1. Carry (Roll date P&L: NPV at t1 with t0 market - NPV at t0 with t0 market)
            res.carry_pnl = rolled_npv - res.prev_npv;
            
            // 2. Realized Cash (Coupons, fixings)
            // Simplified: extract from total move if we had a cashflow engine
            res.cash_pnl = 0.0; 

            // 3. Market Move (Residual from total - carry - cash)
            res.market_move_pnl = res.total_pnl - res.carry_pnl - res.cash_pnl;
            res.residual = 0.0; 
            
            results.push_back(res);
        } else {
            PnlExplainResult res;
            res.trade_id = trade.id;
            res.prev_npv = prev_map.contains(trade.id) ? prev_map[trade.id] : 0.0;
            res.curr_npv = curr_map.contains(trade.id) ? curr_map[trade.id] : 0.0;
            res.total_pnl = res.curr_npv - res.prev_npv;
            res.carry_pnl = 0.0;
            res.cash_pnl = 0.0;
            res.market_move_pnl = 0.0;
            res.residual = res.total_pnl;
            results.push_back(res);
        }
    }
    return results;
}

} // namespace qrp::analytics
