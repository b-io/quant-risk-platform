// Implements reconciled PnL explain using valuation diagnostics and sequential factor revaluation.

#include <qrp/analytics/cashflow_extractor.hpp>
#include <qrp/analytics/pnl_explain_service.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/market/market_snapshot.hpp>

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace qrp::analytics {
namespace {

constexpr double reconciliation_tolerance = 1.0e-8;

std::map<std::string, ValuationResult> by_trade_id(const std::vector<ValuationResult>& results) {
    std::map<std::string, ValuationResult> by_trade;
    for (const auto& result : results) {
        by_trade[result.trade_id] = result;
    }
    return by_trade;
}

PnlExplainComponent make_component(std::string component_id,
                                   PnlExplainComponentType component_type,
                                   std::string label,
                                   double amount,
                                   int sequence,
                                   domain::SupportStatus support_status = domain::SupportStatus::Supported,
                                   std::string status_message = {}) {
    PnlExplainComponent component;
    component.sequence = sequence;
    component.component_id = std::move(component_id);
    component.component_type = component_type;
    component.label = std::move(label);
    component.amount = amount;
    component.support_status = support_status;
    component.status_message = std::move(status_message);
    return component;
}

std::map<std::string, domain::MarketQuote> quote_map(const domain::MarketSnapshot& snapshot) {
    std::map<std::string, domain::MarketQuote> quotes;
    for (const auto& quote : snapshot.quotes) {
        quotes[quote.id] = quote;
    }
    return quotes;
}

std::string bool_string(bool value) {
    return value ? "true" : "false";
}

std::string factor_group_from_type(domain::FactorType factor_type) {
    switch (factor_type) {
        case domain::FactorType::CommodityForward:
        case domain::FactorType::CommoditySpot:
            return "commodity";
        case domain::FactorType::CreditRecovery:
        case domain::FactorType::CreditSpread:
        case domain::FactorType::HazardRate:
            return "credit";
        case domain::FactorType::EquityBorrowRate:
        case domain::FactorType::EquityDividendYield:
        case domain::FactorType::EquityForward:
        case domain::FactorType::EquitySpot:
            return "equity";
        case domain::FactorType::FXForwardPoint:
        case domain::FactorType::FXSpot:
            return "fx";
        case domain::FactorType::BasisSpread:
        case domain::FactorType::RateForward:
        case domain::FactorType::RateZero:
            return "rates";
        case domain::FactorType::Volatility:
            return "volatility";
        case domain::FactorType::Custom:
            return "custom";
    }
    return "custom";
}

std::string factor_group_from_id(const std::string& factor_id) {
    if (factor_id.rfind("RF:COM", 0) == 0) {
        return "commodity";
    }
    if (factor_id.rfind("RF:CREDIT", 0) == 0) {
        return "credit";
    }
    if (factor_id.rfind("RF:EQ", 0) == 0) {
        return "equity";
    }
    if (factor_id.rfind("RF:FX", 0) == 0) {
        return "fx";
    }
    if (factor_id.rfind("RF:RATES", 0) == 0) {
        return "rates";
    }
    if (factor_id.find("VOL") != std::string::npos) {
        return "volatility";
    }
    return "custom";
}

std::map<std::string, domain::FactorDefinition>
factor_definitions_by_id(const std::vector<domain::FactorDefinition>& factors) {
    std::map<std::string, domain::FactorDefinition> by_id;
    for (const auto& factor : factors) {
        by_id[factor.factor_id] = factor;
    }
    return by_id;
}

std::map<std::string, std::vector<domain::FactorBinding>>
bindings_by_factor_id(const std::vector<domain::FactorBinding>& bindings) {
    std::map<std::string, std::vector<domain::FactorBinding>> by_factor;
    for (const auto& binding : bindings) {
        by_factor[binding.factor_id].push_back(binding);
    }
    return by_factor;
}

std::string joined_quote_ids(const std::vector<domain::FactorBinding>& bindings) {
    std::string joined;
    std::set<std::string> quote_ids;
    for (const auto& binding : bindings) {
        quote_ids.insert(binding.quote_id);
    }

    for (const auto& quote_id : quote_ids) {
        if (!joined.empty()) {
            joined += ",";
        }
        joined += quote_id;
    }
    return joined;
}

bool apply_current_quotes_for_factor(qrp::market::MarketState& state,
                                     const std::vector<domain::FactorBinding>& bindings,
                                     const std::map<std::string, domain::MarketQuote>& current_quotes,
                                     PnlExplainComponent& component_template) {
    bool applied_any_quote = false;
    std::set<std::string> missing_quote_ids;
    std::set<std::string> applied_quote_ids;

    for (const auto& binding : bindings) {
        const auto quote_it = current_quotes.find(binding.quote_id);
        if (quote_it == current_quotes.end()) {
            missing_quote_ids.insert(binding.quote_id);
            continue;
        }
        state.add_quote(binding.quote_id, quote_it->second.value);
        applied_quote_ids.insert(binding.quote_id);
        applied_any_quote = true;
    }

    component_template.tags["applied_quote_ids"] = joined_quote_ids(bindings);
    component_template.tags["applied_quote_count"] = std::to_string(applied_quote_ids.size());
    if (!missing_quote_ids.empty()) {
        std::string missing;
        for (const auto& quote_id : missing_quote_ids) {
            if (!missing.empty()) {
                missing += ",";
            }
            missing += quote_id;
        }
        component_template.support_status = domain::SupportStatus::PartiallySupported;
        component_template.status_message = "Current market is missing one or more factor-bound quotes";
        component_template.tags["missing_quote_ids"] = missing;
    }
    return applied_any_quote;
}

double explicit_component_sum(const PnlExplainResult& result) {
    return result.carry_pnl + result.roll_down_pnl + result.market_move_pnl + result.cash_pnl +
           result.trade_activity_pnl + result.fx_translation_pnl + result.model_change_pnl;
}

} // namespace

std::vector<PnlExplainResult> PnlExplainService::explain_pnl(const domain::Portfolio& portfolio,
                                                             const domain::MarketSnapshot& prev_market_dto,
                                                             const domain::MarketSnapshot& curr_market_dto,
                                                             const std::vector<domain::FactorDefinition>& factors,
                                                             const std::vector<domain::FactorBinding>& bindings) {

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

    const bool factor_explain_enabled = !factors.empty() && !bindings.empty();
    const auto current_quotes = quote_map(curr_market_dto);
    const auto factor_map = factor_definitions_by_id(factors);
    const auto bindings_map = bindings_by_factor_id(bindings);

    std::map<std::string, std::vector<PnlExplainComponent>> factor_components_by_trade;
    std::map<std::string, double> factor_pnl_by_trade;
    if (factor_explain_enabled) {
        qrp::market::MarketSnapshot factor_market(rolled_dto);
        auto factor_state = factor_market.built_state();
        PricingContext factor_context(factor_state);
        auto last_map = rolled_map;
        int factor_sequence = 3;

        for (const auto& [factor_id, factor_bindings] : bindings_map) {
            const auto factor_it = factor_map.find(factor_id);
            const auto risk_factor_group = factor_it != factor_map.end()
                                               ? factor_group_from_type(factor_it->second.factor_type)
                                               : factor_group_from_id(factor_id);
            PnlExplainComponent component_template = make_component("market_move:" + factor_id,
                                                                    PnlExplainComponentType::MarketMove,
                                                                    "Market move " + factor_id,
                                                                    0.0,
                                                                    factor_sequence,
                                                                    domain::SupportStatus::Supported);
            component_template.factor_id = factor_id;
            component_template.risk_factor_group = risk_factor_group;
            component_template.tags["factor_explain_method"] = "sequential_full_revaluation";
            component_template.tags["quote_ids"] = joined_quote_ids(factor_bindings);

            const bool applied_any_quote =
                apply_current_quotes_for_factor(*factor_state, factor_bindings, current_quotes, component_template);

            auto factor_results = applied_any_quote ? ValuationService::price_portfolio(portfolio, factor_context)
                                                    : std::vector<ValuationResult>{};
            const auto factor_result_map = applied_any_quote ? by_trade_id(factor_results) : last_map;

            for (const auto& trade_ptr : portfolio.trades) {
                const auto& trade = *trade_ptr;
                auto component = component_template;
                component.amount = factor_result_map.at(trade.id).npv - last_map.at(trade.id).npv;
                component.model_name = factor_result_map.at(trade.id).model_name;
                component.support_status = factor_result_map.at(trade.id).support_status;
                if (!component_template.status_message.empty()) {
                    component.status_message = component_template.status_message;
                    if (component.support_status == domain::SupportStatus::Supported) {
                        component.support_status = component_template.support_status;
                    }
                } else {
                    component.status_message = factor_result_map.at(trade.id).status_message;
                }
                factor_components_by_trade[trade.id].push_back(component);
                factor_pnl_by_trade[trade.id] += component.amount;
            }

            last_map = factor_result_map;
            ++factor_sequence;
        }
    }

    std::vector<PnlExplainResult> results;
    for (const auto& trade_ptr : portfolio.trades) {
        const auto& trade = *trade_ptr;
        PnlExplainResult res;
        const auto support = ValuationService::support_profile(trade);
        res.asset_class = domain::to_string(support.asset_class);
        res.book = trade.book;
        res.currency = trade.currency;
        res.product_type = domain::to_string(support.product_type);
        res.strategy = trade.strategy;
        res.trade_id = trade.id;

        // ValuationService returns one result per input trade, including failed instrument
        // construction, so missing valuation rows would indicate an internal contract violation
        // rather than a normal state.
        res.prev_valuation_available = true;
        res.prev_valuation = prev_map.at(trade.id);
        res.prev_npv = res.prev_valuation.npv;
        res.diagnostics["prev_support_status"] = domain::to_string(res.prev_valuation.support_status);

        res.curr_valuation_available = true;
        res.curr_valuation = curr_map.at(trade.id);
        res.curr_npv = res.curr_valuation.npv;
        res.diagnostics["curr_support_status"] = domain::to_string(res.curr_valuation.support_status);

        res.rolled_valuation_available = true;
        res.rolled_valuation = rolled_map.at(trade.id);
        res.diagnostics["rolled_support_status"] = domain::to_string(res.rolled_valuation.support_status);

        const auto rolled_npv = res.rolled_valuation.npv;
        res.carry_pnl = rolled_npv - res.prev_npv;

        res.cashflow_extraction =
            CashflowExtractor::extract_realized_cashflows(trade, prev_market_dto, curr_market_dto);
        res.cash_pnl = res.cashflow_extraction.realized_cash_pnl;
        res.diagnostics["cashflow_extraction_supported"] = bool_string(res.cashflow_extraction.extraction_supported);
        res.diagnostics["cashflow_support_status"] = domain::to_string(res.cashflow_extraction.support_status);
        if (!res.cashflow_extraction.status_message.empty()) {
            res.diagnostics["cashflow_status_message"] = res.cashflow_extraction.status_message;
        }

        res.total_pnl = res.curr_npv - res.prev_npv + res.cash_pnl;
        res.market_move_pnl = factor_explain_enabled ? factor_pnl_by_trade[trade.id] : res.curr_npv - rolled_npv;
        res.explained_pnl = explicit_component_sum(res);
        res.residual_pnl = res.total_pnl - res.explained_pnl;
        res.residual = res.residual_pnl;
        res.reconciliation_difference = res.total_pnl - res.explained_pnl - res.residual_pnl;
        res.reconciliation_passed = std::abs(res.reconciliation_difference) <= reconciliation_tolerance;
        res.diagnostics["factor_explain_enabled"] = bool_string(factor_explain_enabled);
        res.diagnostics["reconciliation_passed"] = bool_string(res.reconciliation_passed);
        res.diagnostics["risk_factor_group"] = "all";

        auto carry = make_component("carry", PnlExplainComponentType::Carry, "Carry", res.carry_pnl, 1);
        carry.model_name = res.rolled_valuation.model_name;
        carry.risk_factor_group = "aging";
        res.components.push_back(carry);

        auto roll_down =
            make_component("roll_down", PnlExplainComponentType::RollDown, "Roll-down", res.roll_down_pnl, 2);
        roll_down.model_name = res.rolled_valuation.model_name;
        roll_down.risk_factor_group = "aging";
        roll_down.status_message = "Roll-down is not split from frozen-market carry in this model";
        roll_down.tags["split_from_carry"] = "false";
        res.components.push_back(roll_down);

        auto cash = make_component(
            "cash",
            PnlExplainComponentType::Cash,
            "Realized cash",
            res.cash_pnl,
            factor_explain_enabled ? static_cast<int>(factor_components_by_trade[trade.id].size()) + 3 : 3,
            res.cashflow_extraction.support_status,
            res.cashflow_extraction.status_message);
        cash.model_name = res.cashflow_extraction.model_name;
        cash.risk_factor_group = "cash";
        cash.tags = res.cashflow_extraction.tags;
        cash.tags["extraction_supported"] = bool_string(res.cashflow_extraction.extraction_supported);
        res.components.push_back(cash);

        if (factor_explain_enabled) {
            for (const auto& component : factor_components_by_trade[trade.id]) {
                res.components.push_back(component);
            }
        } else {
            auto market_move = make_component("market_move",
                                              PnlExplainComponentType::MarketMove,
                                              "Market move",
                                              res.market_move_pnl,
                                              4);
            market_move.model_name = res.curr_valuation.model_name;
            market_move.risk_factor_group = "all";
            market_move.tags["factor_explain_method"] = "aggregate_revaluation";
            res.components.push_back(market_move);
        }

        const int non_market_sequence =
            factor_explain_enabled ? static_cast<int>(factor_components_by_trade[trade.id].size()) + 4 : 5;

        auto trade_activity = make_component("trade_activity",
                                             PnlExplainComponentType::TradeActivity,
                                             "Trade activity",
                                             res.trade_activity_pnl,
                                             non_market_sequence);
        trade_activity.risk_factor_group = "trade_activity";
        trade_activity.status_message = "Portfolio trade population changes are not present in this explain request";
        res.components.push_back(trade_activity);

        auto fx_translation = make_component("fx_translation",
                                             PnlExplainComponentType::FxTranslation,
                                             "FX translation",
                                             res.fx_translation_pnl,
                                             non_market_sequence + 1);
        fx_translation.risk_factor_group = "fx";
        fx_translation.status_message = "Reporting-currency translation is zero because trade and reporting currency "
                                        "are not separated in this request";
        res.components.push_back(fx_translation);

        auto model_change = make_component("model_change",
                                           PnlExplainComponentType::ModelChange,
                                           "Model / configuration change",
                                           res.model_change_pnl,
                                           non_market_sequence + 2);
        model_change.risk_factor_group = "model";
        model_change.status_message = "Model and configuration changes are not present in this explain request";
        res.components.push_back(model_change);

        auto residual = make_component("residual",
                                       PnlExplainComponentType::Residual,
                                       "Residual",
                                       res.residual_pnl,
                                       non_market_sequence + 3);
        residual.risk_factor_group = std::abs(res.residual_pnl) <= reconciliation_tolerance ? "none" : "unexplained";
        residual.status_message =
            res.reconciliation_passed ? "Reconciled" : "Residual remains after explicit components";
        residual.tags["asset_class"] = res.asset_class;
        residual.tags["book"] = res.book;
        residual.tags["currency"] = res.currency;
        residual.tags["product_type"] = res.product_type;
        residual.tags["reconciliation_difference"] = std::to_string(res.reconciliation_difference);
        residual.tags["reconciliation_passed"] = bool_string(res.reconciliation_passed);
        res.components.push_back(residual);

        results.push_back(res);
    }
    return results;
}

} // namespace qrp::analytics
