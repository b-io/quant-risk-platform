// Implements PnL explain by combining valuation diagnostics with aging and market-move components.

#include <qrp/analytics/pnl_explain_service.hpp>
#include <qrp/analytics/cashflow_extractor.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace qrp::analytics {
namespace {

std::map<std::string, ValuationResult> by_trade_id(const std::vector<ValuationResult>& results) {
    std::map<std::string, ValuationResult> by_trade;
    for (const auto& result : results) {
        by_trade[result.trade_id] = result;
    }
    return by_trade;
}

PnlExplainComponent make_component(
    std::string component_id,
    PnlExplainComponentType component_type,
    std::string label,
    double amount,
    domain::SupportStatus support_status = domain::SupportStatus::Supported,
    std::string status_message = {}) {
    PnlExplainComponent component;
    component.component_id = std::move(component_id);
    component.component_type = component_type;
    component.label = std::move(label);
    component.amount = amount;
    component.support_status = support_status;
    component.status_message = std::move(status_message);
    return component;
}

std::string bool_string(bool value) {
    return value ? "true" : "false";
}

} // namespace

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

    // Carry uses the previous market snapshot rebuilt at the current valuation date.
    // This recreates quote handles and curves consistently instead of sharing mutable
    // handles between independent market states.
    domain::MarketSnapshot rolled_dto = prev_market_dto;
    rolled_dto.valuation_date = curr_market_dto.valuation_date;
    qrp::market::MarketSnapshot rolled_market(rolled_dto);
    PricingContext rolled_context(rolled_market.built_state());

    auto prev_results = ValuationService::price_portfolio(portfolio, prev_context);
    auto curr_results = ValuationService::price_portfolio(portfolio, curr_context);
    auto rolled_results = ValuationService::price_portfolio(portfolio, rolled_context);

    const auto prev_map = by_trade_id(prev_results);
    const auto curr_map = by_trade_id(curr_results);
    const auto rolled_map = by_trade_id(rolled_results);

    std::vector<PnlExplainResult> results;
    for (const auto& trade_ptr : portfolio.trades) {
        const auto& trade = *trade_ptr;
        PnlExplainResult res;
        res.trade_id = trade.id;

        if (const auto prev_it = prev_map.find(trade.id); prev_it != prev_map.end()) {
            res.prev_valuation_available = true;
            res.prev_valuation = prev_it->second;
            res.prev_npv = res.prev_valuation.npv;
            res.diagnostics["prev_support_status"] = domain::to_string(res.prev_valuation.support_status);
        } else {
            res.diagnostics["prev_valuation_missing"] = "true";
        }

        if (const auto curr_it = curr_map.find(trade.id); curr_it != curr_map.end()) {
            res.curr_valuation_available = true;
            res.curr_valuation = curr_it->second;
            res.curr_npv = res.curr_valuation.npv;
            res.diagnostics["curr_support_status"] = domain::to_string(res.curr_valuation.support_status);
        } else {
            res.diagnostics["curr_valuation_missing"] = "true";
        }

        if (const auto rolled_it = rolled_map.find(trade.id); rolled_it != rolled_map.end()) {
            res.rolled_valuation_available = true;
            res.rolled_valuation = rolled_it->second;
            res.diagnostics["rolled_support_status"] = domain::to_string(res.rolled_valuation.support_status);
        } else {
            res.diagnostics["rolled_valuation_missing"] = "true";
        }

        res.total_pnl = res.curr_npv - res.prev_npv;

        if (res.prev_valuation_available && res.curr_valuation_available) {
            const auto rolled_npv = res.rolled_valuation_available ? res.rolled_valuation.npv : res.prev_npv;
            res.carry_pnl = rolled_npv - res.prev_npv;

            res.cashflow_extraction = CashflowExtractor::extract_realized_cashflows(
                trade,
                prev_market_dto,
                curr_market_dto);
            res.cash_pnl = res.cashflow_extraction.realized_cash_pnl;
            res.diagnostics["cashflow_extraction_supported"] = bool_string(res.cashflow_extraction.extraction_supported);
            res.diagnostics["cashflow_support_status"] = domain::to_string(res.cashflow_extraction.support_status);
            if (!res.cashflow_extraction.status_message.empty()) {
                res.diagnostics["cashflow_status_message"] = res.cashflow_extraction.status_message;
            }

            res.market_move_pnl = res.total_pnl - res.carry_pnl - res.cash_pnl;
            res.residual = 0.0;

            auto carry = make_component(
                "carry",
                PnlExplainComponentType::Carry,
                "Carry / roll-down",
                res.carry_pnl,
                res.rolled_valuation_available ? domain::SupportStatus::Supported : domain::SupportStatus::Failed,
                res.rolled_valuation_available ? "" : "Rolled valuation missing; carry falls back to zero");
            if (res.rolled_valuation_available) {
                carry.model_name = res.rolled_valuation.model_name;
            }
            res.components.push_back(carry);

            auto cash = make_component(
                "cash",
                PnlExplainComponentType::Cash,
                "Realized cash",
                res.cash_pnl,
                res.cashflow_extraction.support_status,
                res.cashflow_extraction.status_message);
            cash.model_name = res.cashflow_extraction.model_name;
            cash.tags = res.cashflow_extraction.tags;
            cash.tags["extraction_supported"] = bool_string(res.cashflow_extraction.extraction_supported);
            res.components.push_back(cash);

            auto market_move = make_component(
                "market_move",
                PnlExplainComponentType::MarketMove,
                "Market move",
                res.market_move_pnl);
            market_move.model_name = res.curr_valuation.model_name;
            res.components.push_back(market_move);

            res.components.push_back(make_component(
                "residual",
                PnlExplainComponentType::Residual,
                "Residual",
                res.residual));
        } else {
            res.carry_pnl = 0.0;
            res.cash_pnl = 0.0;
            res.market_move_pnl = 0.0;
            res.residual = res.total_pnl;

            auto residual = make_component(
                "residual",
                PnlExplainComponentType::Residual,
                "Residual",
                res.residual,
                domain::SupportStatus::Failed,
                "Previous or current valuation is missing");
            residual.tags["curr_valuation_available"] = bool_string(res.curr_valuation_available);
            residual.tags["prev_valuation_available"] = bool_string(res.prev_valuation_available);
            res.components.push_back(residual);
        }

        results.push_back(res);
    }
    return results;
}

} // namespace qrp::analytics
