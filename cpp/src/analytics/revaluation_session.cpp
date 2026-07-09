// Implements stateful quote-update and scenario revaluation over cached instruments.

#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/revaluation_session.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/instruments/instrument_factory.hpp>
#include <qrp/instruments/instrument_factory_common.hpp>
#include <qrp/market/factor_shock_resolver.hpp>
#include <qrp/market/market_snapshot.hpp>

#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace qrp::analytics {

namespace {

ValuationResult make_result_template(const domain::Trade& trade) {
    const auto support = ValuationService::support_profile(trade);

    ValuationResult result;
    result.asset_class = support.asset_class;
    result.currency = trade.currency;
    result.model_name = support.model_name;
    result.product_type = support.product_type;
    result.status_message = support.reason;
    result.support_status = support.status;
    result.tags["asset_class"] = domain::to_string(support.asset_class);
    result.tags["book"] = trade.book;
    result.tags["model"] = support.model_name;
    result.tags["product_type"] = domain::to_string(support.product_type);
    result.tags["status"] = domain::to_string(support.status);
    result.tags["strategy"] = trade.strategy;
    result.trade_id = trade.id;
    return result;
}

double total_npv_from(const std::map<std::string, double>& trade_values) {
    double total = 0.0;
    for (const auto& item : trade_values) {
        total += item.second;
    }
    return total;
}

std::vector<std::string> sorted_quote_ids(const std::unordered_map<std::string, double>& quote_values) {
    std::vector<std::string> quote_ids;
    quote_ids.reserve(quote_values.size());
    for (const auto& item : quote_values) {
        quote_ids.push_back(item.first);
    }
    std::sort(quote_ids.begin(), quote_ids.end());
    return quote_ids;
}

std::vector<std::string> sorted_quote_ids(const std::map<std::string, double>& quote_values) {
    std::vector<std::string> quote_ids;
    quote_ids.reserve(quote_values.size());
    for (const auto& item : quote_values) {
        quote_ids.push_back(item.first);
    }
    return quote_ids;
}

std::string curve_id_to_string(const domain::CurveId& curve_id) {
    return domain::to_string(curve_id.currency) + ":" + curve_id.family;
}

std::map<std::string, std::vector<std::string>>
factor_ids_by_quote(const std::vector<domain::FactorDefinition>& factors,
                    const std::vector<domain::FactorBinding>& bindings) {
    std::map<std::string, std::set<std::string>> unique_factor_ids;
    for (const auto& binding : bindings) {
        if (!binding.quote_id.empty()) {
            unique_factor_ids[binding.quote_id].insert(binding.factor_id);
        }
    }
    for (const auto& factor : factors) {
        for (const auto& quote_id : factor.quote_ids) {
            if (!quote_id.empty()) {
                unique_factor_ids[quote_id].insert(factor.factor_id);
            }
        }
    }

    std::map<std::string, std::vector<std::string>> result;
    for (const auto& [quote_id, factor_ids] : unique_factor_ids) {
        result[quote_id] = std::vector<std::string>(factor_ids.begin(), factor_ids.end());
    }
    return result;
}

std::map<std::string, double> scenario_quote_updates(const market::ScenarioDefinition& scenario,
                                                     const std::vector<domain::FactorDefinition>& factors,
                                                     const std::vector<domain::FactorBinding>& bindings,
                                                     const domain::MarketSnapshot& base_market) {
    const auto shocked_quotes =
        market::FactorShockResolver::resolve_quote_values(scenario, factors, bindings, base_market);

    std::set<std::string> touched_quote_ids;
    for (const auto& [factor_id, shock] : scenario.factor_shocks) {
        (void)shock;
        for (const auto& binding : bindings) {
            if (binding.factor_id == factor_id) {
                if (!binding.quote_id.empty()) {
                    touched_quote_ids.insert(binding.quote_id);
                }
            }
        }
    }

    std::map<std::string, double> quote_updates;
    for (const auto& quote_id : touched_quote_ids) {
        quote_updates[quote_id] = shocked_quotes.at(quote_id);
    }
    return quote_updates;
}

void sort_dependencies(std::vector<RevaluationDependency>& dependencies) {
    std::sort(dependencies.begin(), dependencies.end(), [](const auto& lhs, const auto& rhs) {
        return std::tie(lhs.quote_id, lhs.trade_id, lhs.dependency_type, lhs.curve_id) <
               std::tie(rhs.quote_id, rhs.trade_id, rhs.dependency_type, rhs.curve_id);
    });
}

std::vector<RevaluationDependency>
dependencies_for_quote_updates(const std::map<std::string, double>& quote_updates,
                               const std::map<std::string, std::vector<RevaluationDependency>>& dependencies_by_quote) {
    std::vector<RevaluationDependency> dependencies;
    std::set<std::tuple<std::string, std::string, std::string, std::string>> seen;
    for (const auto& [quote_id, value] : quote_updates) {
        (void)value;
        const auto it = dependencies_by_quote.find(quote_id);
        if (it == dependencies_by_quote.end()) {
            continue;
        }
        for (const auto& dependency : it->second) {
            const auto key = std::make_tuple(dependency.quote_id,
                                             dependency.trade_id,
                                             dependency.dependency_type,
                                             dependency.curve_id);
            if (seen.insert(key).second) {
                dependencies.push_back(dependency);
            }
        }
    }
    sort_dependencies(dependencies);
    return dependencies;
}

std::vector<std::string> unique_trade_ids(const std::vector<RevaluationDependency>& dependencies) {
    std::set<std::string> trade_ids;
    for (const auto& dependency : dependencies) {
        trade_ids.insert(dependency.trade_id);
    }
    return std::vector<std::string>(trade_ids.begin(), trade_ids.end());
}

std::vector<std::string> unique_quote_ids(const std::vector<RevaluationDependency>& dependencies) {
    std::set<std::string> quote_ids;
    for (const auto& dependency : dependencies) {
        quote_ids.insert(dependency.quote_id);
    }
    return std::vector<std::string>(quote_ids.begin(), quote_ids.end());
}

std::vector<RevaluationDependency>
flatten_dependencies(const std::map<std::string, std::vector<RevaluationDependency>>& dependencies_by_quote) {
    std::vector<RevaluationDependency> dependencies;
    for (const auto& [quote_id, quote_dependencies] : dependencies_by_quote) {
        (void)quote_id;
        dependencies.insert(dependencies.end(), quote_dependencies.begin(), quote_dependencies.end());
    }
    sort_dependencies(dependencies);
    return dependencies;
}

std::set<std::string> trade_id_set(const std::vector<std::string>& trade_ids) {
    return std::set<std::string>(trade_ids.begin(), trade_ids.end());
}

} // namespace

RevaluationSession::RevaluationSession(domain::Portfolio portfolio,
                                       domain::MarketSnapshot base_market,
                                       std::vector<domain::FactorDefinition> factors,
                                       std::vector<domain::FactorBinding> bindings)
    : base_market_(std::move(base_market)), bindings_(std::move(bindings)), factors_(std::move(factors)),
      portfolio_(std::move(portfolio)) {
    build_session();
}

void RevaluationSession::apply_quote_updates(const std::map<std::string, double>& quote_updates) {
    for (const auto& item : quote_updates) {
        ensure_quote_exists(item.first);
    }
    for (const auto& [quote_id, value] : quote_updates) {
        state_->add_quote(quote_id, value);
    }
}

void RevaluationSession::apply_scenario(const market::ScenarioDefinition& scenario) {
    if (factors_.empty() || bindings_.empty()) {
        throw std::runtime_error("RevaluationSession: factors and bindings are required for scenario revaluation");
    }
    market::ScenarioEngine::apply_scenario_to_state(*state_, base_market_, scenario, factors_, bindings_);
}

std::vector<ValuationResult> RevaluationSession::price() const {
    std::vector<ValuationResult> results;
    results.reserve(instruments_.size());

    for (const auto& item : instruments_) {
        const auto& trade = *item.trade;
        auto result = make_result_template(trade);
        if (item.instrument) {
            result.npv = ValuationService::price_instrument(trade, *item.instrument);
        } else {
            result.npv = 0.0;
            result.support_status = result.support_status == domain::SupportStatus::Unsupported
                                        ? domain::SupportStatus::Unsupported
                                        : domain::SupportStatus::Failed;
            result.status_message = "Instrument construction failed: " + result.status_message;
            result.tags["error"] = "Instrument construction failed";
            result.tags["status"] = domain::to_string(result.support_status);
        }
        results.push_back(std::move(result));
    }
    return results;
}

void RevaluationSession::reset() {
    state_->reset_to_snapshot(base_market_);
}

RevaluationReport RevaluationSession::revalue_scenario(const market::ScenarioDefinition& scenario) {
    if (factors_.empty() || bindings_.empty()) {
        throw std::runtime_error("RevaluationSession: factors and bindings are required for scenario revaluation");
    }

    reset();
    const auto base_trade_values = trade_values();
    const auto base_total = total_npv_from(base_trade_values);

    const auto shocked_quotes =
        market::FactorShockResolver::resolve_quote_values(scenario, factors_, bindings_, base_market_);
    const auto quote_ids = sorted_quote_ids(shocked_quotes);
    const auto before_quotes = quote_values_for(quote_ids);

    apply_scenario(scenario);
    const auto shocked_trade_values = trade_values();
    const auto shocked_total = total_npv_from(shocked_trade_values);
    const auto after_quotes = quote_values_for(quote_ids);

    reset();
    const auto restored_total = total_npv();

    std::map<std::string, double> trade_pnls;
    for (const auto& [trade_id, shocked_value] : shocked_trade_values) {
        trade_pnls[trade_id] = shocked_value - base_trade_values.at(trade_id);
    }

    RevaluationReport report;
    report.base_total_npv = base_total;
    report.quote_moves = quote_moves_for(before_quotes, after_quotes);
    report.restored_total_npv = restored_total;
    report.scenario_name = scenario.name;
    report.scenario_pnl = shocked_total - base_total;
    report.shocked_total_npv = shocked_total;
    report.trade_pnls = std::move(trade_pnls);
    return report;
}

RevaluationImpactPreview
RevaluationSession::preview_quote_update_impact(const std::map<std::string, double>& quote_updates) const {
    for (const auto& [quote_id, value] : quote_updates) {
        (void)value;
        ensure_quote_exists(quote_id);
    }

    build_dependency_index();

    RevaluationImpactPreview preview;
    preview.dependencies = dependencies_for_quote_updates(quote_updates, dependency_index_by_quote_);
    preview.potentially_affected_trade_ids = unique_trade_ids(preview.dependencies);
    preview.potentially_affected_trade_count = preview.potentially_affected_trade_ids.size();
    preview.updated_quote_ids = sorted_quote_ids(quote_updates);
    return preview;
}

RevaluationImpactPreview RevaluationSession::preview_scenario_impact(const market::ScenarioDefinition& scenario) const {
    if (factors_.empty() || bindings_.empty()) {
        throw std::runtime_error("RevaluationSession: factors and bindings are required for scenario revaluation");
    }

    return preview_quote_update_impact(scenario_quote_updates(scenario, factors_, bindings_, base_market_));
}

RevaluationImpactReport
RevaluationSession::revalue_quote_update_impact(const std::map<std::string, double>& quote_updates,
                                                double pnl_tolerance) {
    reset();
    const auto preview = preview_quote_update_impact(quote_updates);
    const auto quote_ids = preview.updated_quote_ids;
    const auto candidate_trade_ids = trade_id_set(preview.potentially_affected_trade_ids);

    const auto before_quotes = quote_values_for(quote_ids);
    const auto base_trade_values = trade_values_for(candidate_trade_ids);
    const auto candidate_base_total = total_npv_from(base_trade_values);

    apply_quote_updates(quote_updates);
    const auto shocked_trade_values = trade_values_for(candidate_trade_ids);
    const auto candidate_shocked_total = total_npv_from(shocked_trade_values);
    const auto after_quotes = quote_values_for(quote_ids);

    reset();
    const auto restored_trade_values = trade_values_for(candidate_trade_ids);

    std::map<std::string, std::set<std::string>> dependency_quotes_by_trade;
    for (const auto& dependency : preview.dependencies) {
        dependency_quotes_by_trade[dependency.trade_id].insert(dependency.quote_id);
    }

    std::map<std::string, const domain::Trade*> trades_by_id;
    for (const auto& item : instruments_) {
        trades_by_id[item.trade->id] = item.trade;
    }

    std::vector<TradeRevaluationDiff> trade_diffs;
    trade_diffs.reserve(preview.potentially_affected_trade_ids.size());
    for (const auto& trade_id : preview.potentially_affected_trade_ids) {
        const auto base_it = base_trade_values.find(trade_id);
        const auto shocked_it = shocked_trade_values.find(trade_id);
        const auto base_npv = base_it == base_trade_values.end() ? 0.0 : base_it->second;
        const auto shocked_npv = shocked_it == shocked_trade_values.end() ? 0.0 : shocked_it->second;
        const auto support = ValuationService::support_profile(*trades_by_id.at(trade_id));

        TradeRevaluationDiff diff;
        diff.asset_class = domain::to_string(support.asset_class);
        diff.base_npv = base_npv;
        diff.dependency_quote_ids = std::vector<std::string>(dependency_quotes_by_trade[trade_id].begin(),
                                                             dependency_quotes_by_trade[trade_id].end());
        diff.moved_above_tolerance = std::fabs(shocked_npv - base_npv) > pnl_tolerance;
        diff.pnl = shocked_npv - base_npv;
        diff.product_type = domain::to_string(support.product_type);
        diff.shocked_npv = shocked_npv;
        diff.trade_id = trade_id;
        trade_diffs.push_back(std::move(diff));
    }

    RevaluationImpactReport report;
    report.candidate_base_total_npv = candidate_base_total;
    report.candidate_pnl = candidate_shocked_total - candidate_base_total;
    report.candidate_restored_total_npv = total_npv_from(restored_trade_values);
    report.candidate_shocked_total_npv = candidate_shocked_total;
    report.dependencies = preview.dependencies;
    report.pnl_tolerance = pnl_tolerance;
    report.potentially_affected_trade_count = preview.potentially_affected_trade_count;
    report.potentially_affected_trade_ids = preview.potentially_affected_trade_ids;
    report.quote_moves = quote_moves_for(before_quotes, after_quotes);
    report.trade_diffs = std::move(trade_diffs);
    report.updated_quote_ids = preview.updated_quote_ids;
    return report;
}

RevaluationImpactReport RevaluationSession::revalue_scenario_impact(const market::ScenarioDefinition& scenario,
                                                                    double pnl_tolerance) {
    if (factors_.empty() || bindings_.empty()) {
        throw std::runtime_error("RevaluationSession: factors and bindings are required for scenario revaluation");
    }

    auto report =
        revalue_quote_update_impact(scenario_quote_updates(scenario, factors_, bindings_, base_market_), pnl_tolerance);
    report.scenario_name = scenario.name;
    return report;
}

RevaluationDependencyGraph RevaluationSession::dependency_graph() const {
    build_dependency_index();

    RevaluationDependencyGraph graph;
    graph.dependencies = flatten_dependencies(dependency_index_by_quote_);
    graph.dependency_count = graph.dependencies.size();
    graph.quote_ids = unique_quote_ids(graph.dependencies);
    graph.quote_count = graph.quote_ids.size();
    graph.trade_ids = unique_trade_ids(graph.dependencies);
    graph.trade_count = graph.trade_ids.size();
    return graph;
}

std::vector<RevaluationDependency> RevaluationSession::dependencies_for_quote(const std::string& quote_id) const {
    ensure_quote_exists(quote_id);
    build_dependency_index();

    const auto it = dependency_index_by_quote_.find(quote_id);
    if (it == dependency_index_by_quote_.end()) {
        return {};
    }

    auto dependencies = it->second;
    sort_dependencies(dependencies);
    return dependencies;
}

std::vector<RevaluationDependency> RevaluationSession::dependencies_for_trade(const std::string& trade_id) const {
    bool trade_found = false;
    for (const auto& item : instruments_) {
        if (item.trade != nullptr && item.trade->id == trade_id) {
            trade_found = true;
            break;
        }
    }
    if (!trade_found) {
        throw std::invalid_argument("RevaluationSession: dependency query references unknown trade id: " + trade_id);
    }

    build_dependency_index();

    std::vector<RevaluationDependency> dependencies;
    for (const auto& [quote_id, quote_dependencies] : dependency_index_by_quote_) {
        (void)quote_id;
        for (const auto& dependency : quote_dependencies) {
            if (dependency.trade_id == trade_id) {
                dependencies.push_back(dependency);
            }
        }
    }
    sort_dependencies(dependencies);
    return dependencies;
}

std::map<std::string, double> RevaluationSession::trade_values() const {
    std::map<std::string, double> values;
    for (const auto& result : price()) {
        values[result.trade_id] = result.npv;
    }
    return values;
}

std::map<std::string, double> RevaluationSession::trade_values_for(const std::set<std::string>& trade_ids) const {
    std::map<std::string, double> values;
    if (trade_ids.empty()) {
        return values;
    }

    for (const auto& item : instruments_) {
        const auto& trade = *item.trade;
        if (!trade_ids.contains(trade.id)) {
            continue;
        }
        values[trade.id] = item.instrument ? ValuationService::price_instrument(trade, *item.instrument) : 0.0;
    }
    return values;
}

double RevaluationSession::total_npv() const {
    return total_npv_from(trade_values());
}

void RevaluationSession::build_dependency_index() const {
    if (dependency_index_built_) {
        return;
    }

    dependency_index_by_quote_.clear();

    std::set<std::string> known_quote_ids;
    for (const auto& quote : base_market_.quotes) {
        known_quote_ids.insert(quote.id);
    }

    std::map<domain::CurveId, domain::CurveSpec> curves_by_id;
    for (const auto& curve : base_market_.curves) {
        curves_by_id[curve.id] = curve;
    }

    const auto factor_ids = factor_ids_by_quote(factors_, bindings_);
    std::set<std::tuple<std::string, std::string, std::string, std::string>> seen_dependencies;

    auto add_dependency = [&](const domain::Trade& trade,
                              const std::string& quote_id,
                              const std::string& dependency_type,
                              const std::string& curve_id = {}) {
        if (quote_id.empty() || !known_quote_ids.contains(quote_id)) {
            return;
        }

        const auto key = std::make_tuple(quote_id, trade.id, dependency_type, curve_id);
        if (!seen_dependencies.insert(key).second) {
            return;
        }

        const auto support = ValuationService::support_profile(trade);
        RevaluationDependency dependency;
        dependency.asset_class = domain::to_string(support.asset_class);
        dependency.curve_id = curve_id;
        dependency.dependency_type = dependency_type;
        const auto factor_it = factor_ids.find(quote_id);
        if (factor_it != factor_ids.end()) {
            dependency.factor_ids = factor_it->second;
        }
        dependency.product_type = domain::to_string(support.product_type);
        dependency.quote_id = quote_id;
        dependency.trade_id = trade.id;
        dependency_index_by_quote_[quote_id].push_back(std::move(dependency));
    };

    auto add_curve_dependencies =
        [&](const domain::Trade& trade, const domain::CurveId& curve_id, const std::string& dependency_type) {
            const auto curve_it = curves_by_id.find(curve_id);
            if (curve_it == curves_by_id.end()) {
                return;
            }
            const auto curve_label = curve_id_to_string(curve_id);
            for (const auto& quote_id : curve_it->second.quote_ids) {
                add_dependency(trade, quote_id, dependency_type, curve_label);
            }
        };

    auto add_discount_curve = [&](const domain::Trade& trade, domain::Currency currency) {
        if (currency == domain::Currency::UNKNOWN) {
            return;
        }
        add_curve_dependencies(trade, {currency, "OIS"}, "discount_curve");
    };

    auto add_forecast_curve =
        [&](const domain::Trade& trade, domain::Currency currency, const std::string& index_family) {
            if (currency == domain::Currency::UNKNOWN) {
                return;
            }
            const domain::CurveId forecast_curve{currency, instruments::normalize_index_family(index_family)};
            if (curves_by_id.contains(forecast_curve)) {
                add_curve_dependencies(trade, forecast_curve, "forecast_curve");
            } else {
                add_curve_dependencies(trade, {currency, "OIS"}, "forecast_curve");
            }
        };

    auto add_explicit_or_matching_quotes = [&](const domain::Trade& trade,
                                               const std::string& explicit_quote_id,
                                               const std::string& underlier,
                                               const std::string& tenor,
                                               std::initializer_list<domain::QuoteInstrumentType> accepted_types,
                                               const std::string& dependency_type) {
        if (!explicit_quote_id.empty() && known_quote_ids.contains(explicit_quote_id)) {
            add_dependency(trade, explicit_quote_id, "direct_quote");
            return std::size_t{1};
        }

        std::size_t added = 0;
        for (const auto& quote : base_market_.quotes) {
            if (instruments::quote_matches_underlier_and_tenor(quote, underlier, tenor, accepted_types)) {
                add_dependency(trade, quote.id, dependency_type);
                ++added;
            }
        }
        return added;
    };

    auto add_matching_quotes = [&](const domain::Trade& trade,
                                   const std::string& underlier,
                                   const std::string& tenor,
                                   std::initializer_list<domain::QuoteInstrumentType> accepted_types,
                                   const std::string& dependency_type) {
        return add_explicit_or_matching_quotes(trade, {}, underlier, tenor, accepted_types, dependency_type);
    };

    auto add_direct_quote = [&](const domain::Trade& trade,
                                const std::string& quote_id,
                                const std::string& dependency_type = "direct_quote") {
        add_dependency(trade, quote_id, dependency_type);
    };

    auto add_credit_spread_quotes = [&](const domain::Trade& trade,
                                        const std::string& underlier,
                                        const std::string& primary_quote_id,
                                        std::initializer_list<domain::QuoteInstrumentType> accepted_types) {
        add_direct_quote(trade, primary_quote_id, "direct_quote");
        for (const auto& quote : base_market_.quotes) {
            if (quote.id != primary_quote_id &&
                instruments::quote_matches_credit_underlier(quote, underlier, accepted_types)) {
                add_dependency(trade, quote.id, "market_quote_match");
            }
        }
    };

    auto add_recovery_quote =
        [&](const domain::Trade& trade, const std::string& underlier, const std::string& recovery_quote_id) {
            if (!recovery_quote_id.empty() && known_quote_ids.contains(recovery_quote_id)) {
                add_dependency(trade, recovery_quote_id, "direct_quote");
                return;
            }
            for (const auto& quote : base_market_.quotes) {
                if (quote.instrument_type == domain::QuoteInstrumentType::RecoveryRate &&
                    (underlier.empty() || quote.underlier == underlier)) {
                    add_dependency(trade, quote.id, "market_quote_match");
                }
            }
        };

    auto add_fx_spot_quote = [&](const domain::Trade& trade,
                                 const std::string& explicit_quote_id,
                                 const std::string& base_currency,
                                 const std::string& quote_currency) {
        add_direct_quote(trade,
                         explicit_quote_id.empty() ? instruments::fx_pair(base_currency, quote_currency)
                                                   : explicit_quote_id,
                         "direct_quote");
    };

    for (const auto& item : instruments_) {
        const auto& trade = *item.trade;
        const auto trade_currency = domain::from_string(trade.currency);

        if (const auto* deposit = dynamic_cast<const domain::DepositTrade*>(&trade)) {
            add_discount_curve(*deposit, trade_currency);
        } else if (const auto* fra = dynamic_cast<const domain::FraTrade*>(&trade)) {
            add_discount_curve(*fra, trade_currency);
            add_forecast_curve(*fra, trade_currency, fra->floating_index);
        } else if (const auto* future = dynamic_cast<const domain::InterestRateFutureTrade*>(&trade)) {
            add_forecast_curve(*future, trade_currency, future->floating_index);
            add_direct_quote(*future, future->future_quote_id);
        } else if (const auto* swap = dynamic_cast<const domain::VanillaSwapTrade*>(&trade)) {
            add_discount_curve(*swap, trade_currency);
            add_forecast_curve(*swap, trade_currency, swap->floating_index);
        } else if (const auto* ois = dynamic_cast<const domain::OisSwapTrade*>(&trade)) {
            add_discount_curve(*ois, trade_currency);
        } else if (const auto* callable_bond = dynamic_cast<const domain::CallableBondTrade*>(&trade)) {
            add_discount_curve(*callable_bond, trade_currency);
            add_direct_quote(*callable_bond, callable_bond->volatility_quote_id);
        } else if (const auto* bond = dynamic_cast<const domain::FixedRateBondTrade*>(&trade)) {
            add_discount_curve(*bond, trade_currency);
        } else if (const auto* frn = dynamic_cast<const domain::FloatingRateNoteTrade*>(&trade)) {
            add_discount_curve(*frn, trade_currency);
            add_forecast_curve(*frn, trade_currency, frn->floating_index);
        } else if (const auto* cap_floor = dynamic_cast<const domain::CapFloorTrade*>(&trade)) {
            add_discount_curve(*cap_floor, trade_currency);
            add_forecast_curve(*cap_floor, trade_currency, cap_floor->floating_index);
            add_direct_quote(*cap_floor, cap_floor->volatility_quote_id);
        } else if (const auto* swaption = dynamic_cast<const domain::EuropeanSwaptionTrade*>(&trade)) {
            add_discount_curve(*swaption, trade_currency);
            add_forecast_curve(*swaption, trade_currency, swaption->floating_index);
            add_direct_quote(*swaption, swaption->volatility_quote_id);
        } else if (const auto* bermudan = dynamic_cast<const domain::BermudanSwaptionTrade*>(&trade)) {
            add_discount_curve(*bermudan, trade_currency);
            add_direct_quote(*bermudan, bermudan->volatility_quote_id);
        } else if (const auto* fx_spot = dynamic_cast<const domain::FxSpotTrade*>(&trade)) {
            add_fx_spot_quote(*fx_spot, fx_spot->spot_quote_id, fx_spot->base_currency, fx_spot->quote_currency);
        } else if (const auto* fx_forward = dynamic_cast<const domain::FxForwardTrade*>(&trade)) {
            add_fx_spot_quote(*fx_forward,
                              fx_forward->spot_quote_id,
                              fx_forward->base_currency,
                              fx_forward->quote_currency);
            add_direct_quote(*fx_forward, fx_forward->forward_quote_id);
            add_direct_quote(*fx_forward, fx_forward->forward_points_quote_id);
            add_discount_curve(*fx_forward, domain::from_string(fx_forward->quote_currency));
            add_discount_curve(*fx_forward, domain::from_string(fx_forward->base_currency));
        } else if (const auto* fx_swap = dynamic_cast<const domain::FxSwapTrade*>(&trade)) {
            add_fx_spot_quote(*fx_swap, fx_swap->spot_quote_id, fx_swap->base_currency, fx_swap->quote_currency);
            add_direct_quote(*fx_swap, fx_swap->near_forward_quote_id);
            add_direct_quote(*fx_swap, fx_swap->near_forward_points_quote_id);
            add_direct_quote(*fx_swap, fx_swap->far_forward_quote_id);
            add_direct_quote(*fx_swap, fx_swap->far_forward_points_quote_id);
            add_discount_curve(*fx_swap, domain::from_string(fx_swap->quote_currency));
            add_discount_curve(*fx_swap, domain::from_string(fx_swap->base_currency));
        } else if (const auto* ndf = dynamic_cast<const domain::NdfTrade*>(&trade)) {
            add_fx_spot_quote(*ndf, ndf->spot_quote_id, ndf->base_currency, ndf->quote_currency);
            add_direct_quote(*ndf, ndf->fixing_quote_id);
            add_direct_quote(*ndf, ndf->forward_quote_id);
            add_direct_quote(*ndf, ndf->forward_points_quote_id);
            add_discount_curve(*ndf, domain::from_string(ndf->quote_currency));
            add_discount_curve(*ndf, domain::from_string(ndf->base_currency));
        } else if (const auto* fx_option = dynamic_cast<const domain::FxOptionTrade*>(&trade)) {
            add_fx_spot_quote(*fx_option,
                              fx_option->spot_quote_id,
                              fx_option->base_currency,
                              fx_option->quote_currency);
            add_direct_quote(*fx_option, fx_option->volatility_quote_id);
            add_discount_curve(*fx_option, domain::from_string(fx_option->quote_currency));
            add_discount_curve(*fx_option, domain::from_string(fx_option->base_currency));
        } else if (const auto* credit_bond = dynamic_cast<const domain::CreditBondTrade*>(&trade)) {
            add_discount_curve(*credit_bond, trade_currency);
            add_credit_spread_quotes(*credit_bond,
                                     credit_bond->issuer,
                                     credit_bond->spread_quote_id,
                                     {domain::QuoteInstrumentType::BondSpread,
                                      domain::QuoteInstrumentType::CDS,
                                      domain::QuoteInstrumentType::CreditSpread});
        } else if (const auto* cds = dynamic_cast<const domain::CdsTrade*>(&trade)) {
            add_discount_curve(*cds, trade_currency);
            add_credit_spread_quotes(*cds,
                                     cds->issuer,
                                     cds->spread_quote_id,
                                     {domain::QuoteInstrumentType::CDS, domain::QuoteInstrumentType::CreditSpread});
            add_recovery_quote(*cds, cds->issuer, cds->recovery_quote_id);
        } else if (const auto* index = dynamic_cast<const domain::CdsIndexTrade*>(&trade)) {
            add_discount_curve(*index, trade_currency);
            add_credit_spread_quotes(*index,
                                     index->index_name,
                                     index->spread_quote_id,
                                     {domain::QuoteInstrumentType::CDS,
                                      domain::QuoteInstrumentType::CreditIndex,
                                      domain::QuoteInstrumentType::CreditSpread});
            add_recovery_quote(*index, index->index_name, index->recovery_quote_id);
        } else if (const auto* cds_option = dynamic_cast<const domain::CdsOptionTrade*>(&trade)) {
            add_discount_curve(*cds_option, trade_currency);
            add_credit_spread_quotes(*cds_option,
                                     cds_option->issuer,
                                     cds_option->spread_quote_id,
                                     {domain::QuoteInstrumentType::CDS, domain::QuoteInstrumentType::CreditSpread});
            add_recovery_quote(*cds_option, cds_option->issuer, cds_option->recovery_quote_id);
            add_direct_quote(*cds_option, cds_option->volatility_quote_id);
        } else if (const auto* index_option = dynamic_cast<const domain::CreditIndexOptionTrade*>(&trade)) {
            add_discount_curve(*index_option, trade_currency);
            add_credit_spread_quotes(*index_option,
                                     index_option->index_name,
                                     index_option->spread_quote_id,
                                     {domain::QuoteInstrumentType::CDS,
                                      domain::QuoteInstrumentType::CreditIndex,
                                      domain::QuoteInstrumentType::CreditSpread});
            add_recovery_quote(*index_option, index_option->index_name, index_option->recovery_quote_id);
            add_direct_quote(*index_option, index_option->volatility_quote_id);
        } else if (const auto* commodity_spot = dynamic_cast<const domain::CommoditySpotTrade*>(&trade)) {
            add_explicit_or_matching_quotes(
                *commodity_spot,
                commodity_spot->spot_quote_id,
                commodity_spot->underlier,
                "SPOT",
                {domain::QuoteInstrumentType::CommoditySpot, domain::QuoteInstrumentType::CommodityForward},
                "market_quote_match");
        } else if (const auto* commodity_forward = dynamic_cast<const domain::CommodityForwardTrade*>(&trade)) {
            const auto forward_count = add_explicit_or_matching_quotes(
                *commodity_forward,
                commodity_forward->forward_quote_id,
                commodity_forward->underlier,
                commodity_forward->tenor,
                {domain::QuoteInstrumentType::CommodityForward, domain::QuoteInstrumentType::CommodityFuture},
                "market_quote_match");
            if (forward_count == 0) {
                add_explicit_or_matching_quotes(*commodity_forward,
                                                commodity_forward->spot_quote_id,
                                                commodity_forward->underlier,
                                                "SPOT",
                                                {domain::QuoteInstrumentType::CommoditySpot},
                                                "market_quote_match");
            }
            add_discount_curve(*commodity_forward, trade_currency);
        } else if (const auto* commodity_future = dynamic_cast<const domain::CommodityFutureTrade*>(&trade)) {
            add_explicit_or_matching_quotes(
                *commodity_future,
                commodity_future->future_quote_id,
                commodity_future->underlier,
                commodity_future->tenor,
                {domain::QuoteInstrumentType::CommodityFuture, domain::QuoteInstrumentType::Future},
                "market_quote_match");
        } else if (const auto* strip = dynamic_cast<const domain::CommodityFutureStripTrade*>(&trade)) {
            for (const auto& quote_id : strip->future_quote_ids) {
                add_direct_quote(*strip, quote_id);
            }
            add_discount_curve(*strip, trade_currency);
        } else if (const auto* future_option = dynamic_cast<const domain::CommodityFutureOptionTrade*>(&trade)) {
            add_explicit_or_matching_quotes(*future_option,
                                            future_option->future_quote_id,
                                            future_option->underlier,
                                            future_option->tenor,
                                            {domain::QuoteInstrumentType::CommodityFuture,
                                             domain::QuoteInstrumentType::CommodityForward,
                                             domain::QuoteInstrumentType::Future},
                                            "market_quote_match");
            add_explicit_or_matching_quotes(*future_option,
                                            future_option->volatility_quote_id,
                                            future_option->underlier,
                                            {},
                                            {domain::QuoteInstrumentType::CommodityVol},
                                            "market_quote_match");
            add_discount_curve(*future_option, trade_currency);
        } else if (const auto* spread_option =
                       dynamic_cast<const domain::CommodityCalendarSpreadOptionTrade*>(&trade)) {
            add_direct_quote(*spread_option, spread_option->near_future_quote_id);
            add_direct_quote(*spread_option, spread_option->far_future_quote_id);
            add_explicit_or_matching_quotes(*spread_option,
                                            spread_option->volatility_quote_id,
                                            spread_option->underlier,
                                            {},
                                            {domain::QuoteInstrumentType::CommodityVol},
                                            "market_quote_match");
            add_discount_curve(*spread_option, trade_currency);
        } else if (const auto* swing = dynamic_cast<const domain::CommoditySwingTrade*>(&trade)) {
            std::size_t direct_count = 0;
            for (const auto& quote_id : swing->forward_quote_ids) {
                if (known_quote_ids.contains(quote_id)) {
                    add_direct_quote(*swing, quote_id);
                    ++direct_count;
                }
            }
            if (direct_count == 0) {
                add_matching_quotes(
                    *swing,
                    swing->underlier,
                    {},
                    {domain::QuoteInstrumentType::CommodityForward, domain::QuoteInstrumentType::CommodityFuture},
                    "market_quote_match");
            }
            add_discount_curve(*swing, trade_currency);
        } else if (const auto* equity_spot = dynamic_cast<const domain::EquitySpotTrade*>(&trade)) {
            add_matching_quotes(*equity_spot,
                                equity_spot->underlier,
                                "SPOT",
                                {domain::QuoteInstrumentType::EquitySpot, domain::QuoteInstrumentType::Future},
                                "market_quote_match");
        } else if (const auto* equity_forward = dynamic_cast<const domain::EquityForwardTrade*>(&trade)) {
            add_explicit_or_matching_quotes(*equity_forward,
                                            equity_forward->spot_quote_id,
                                            equity_forward->underlier,
                                            "SPOT",
                                            {domain::QuoteInstrumentType::EquitySpot},
                                            "market_quote_match");
            add_explicit_or_matching_quotes(*equity_forward,
                                            equity_forward->dividend_yield_quote_id,
                                            equity_forward->underlier,
                                            {},
                                            {domain::QuoteInstrumentType::DividendYield},
                                            "market_quote_match");
            add_explicit_or_matching_quotes(*equity_forward,
                                            equity_forward->borrow_rate_quote_id,
                                            equity_forward->underlier,
                                            {},
                                            {domain::QuoteInstrumentType::BorrowRate},
                                            "market_quote_match");
            add_discount_curve(*equity_forward, trade_currency);
        } else if (const auto* equity_future = dynamic_cast<const domain::EquityFutureTrade*>(&trade)) {
            add_direct_quote(*equity_future, equity_future->future_quote_id);
            add_explicit_or_matching_quotes(*equity_future,
                                            equity_future->spot_quote_id,
                                            equity_future->underlier,
                                            "SPOT",
                                            {domain::QuoteInstrumentType::EquitySpot},
                                            "market_quote_match");
            add_explicit_or_matching_quotes(*equity_future,
                                            equity_future->dividend_yield_quote_id,
                                            equity_future->underlier,
                                            {},
                                            {domain::QuoteInstrumentType::DividendYield},
                                            "market_quote_match");
            add_explicit_or_matching_quotes(*equity_future,
                                            equity_future->borrow_rate_quote_id,
                                            equity_future->underlier,
                                            {},
                                            {domain::QuoteInstrumentType::BorrowRate},
                                            "market_quote_match");
            add_discount_curve(*equity_future, trade_currency);
        } else if (const auto* equity_option = dynamic_cast<const domain::EquityOptionTrade*>(&trade)) {
            add_explicit_or_matching_quotes(*equity_option,
                                            equity_option->spot_quote_id,
                                            equity_option->underlier,
                                            "SPOT",
                                            {domain::QuoteInstrumentType::EquitySpot},
                                            "market_quote_match");
            add_explicit_or_matching_quotes(*equity_option,
                                            equity_option->dividend_yield_quote_id,
                                            equity_option->underlier,
                                            {},
                                            {domain::QuoteInstrumentType::DividendYield},
                                            "market_quote_match");
            add_explicit_or_matching_quotes(*equity_option,
                                            equity_option->borrow_rate_quote_id,
                                            equity_option->underlier,
                                            {},
                                            {domain::QuoteInstrumentType::BorrowRate},
                                            "market_quote_match");
            add_explicit_or_matching_quotes(*equity_option,
                                            equity_option->volatility_quote_id,
                                            equity_option->underlier,
                                            {},
                                            {domain::QuoteInstrumentType::EquityVol},
                                            "market_quote_match");
            add_discount_curve(*equity_option, trade_currency);
        }
    }

    for (auto& [quote_id, dependencies] : dependency_index_by_quote_) {
        (void)quote_id;
        sort_dependencies(dependencies);
    }
    dependency_index_built_ = true;
}

void RevaluationSession::build_session() {
    market::MarketSnapshot market(base_market_);
    state_ = market.built_state();
    PricingContext context(state_);

    instruments_.clear();
    instruments_.reserve(portfolio_.trades.size());
    for (const auto& trade_ptr : portfolio_.trades) {
        const auto& trade = *trade_ptr;
        instruments_.push_back({&trade, instruments::InstrumentFactory::create_instrument(trade, context)});
    }
}

void RevaluationSession::ensure_quote_exists(const std::string& quote_id) const {
    if (!state_->get_quote_handle(quote_id)) {
        throw std::invalid_argument("RevaluationSession: quote update references unknown quote id: " + quote_id);
    }
}

std::vector<QuoteMove> RevaluationSession::quote_moves_for(const std::map<std::string, double>& before,
                                                           const std::map<std::string, double>& after) const {
    std::vector<QuoteMove> moves;
    moves.reserve(before.size());
    for (const auto& [quote_id, before_value] : before) {
        QuoteMove move;
        move.after = after.at(quote_id);
        move.before = before_value;
        move.quote_id = quote_id;
        move.restored = state_->get_quote(quote_id);
        moves.push_back(std::move(move));
    }
    return moves;
}

std::map<std::string, double> RevaluationSession::quote_values_for(const std::vector<std::string>& quote_ids) const {
    std::map<std::string, double> values;
    for (const auto& quote_id : quote_ids) {
        ensure_quote_exists(quote_id);
        values[quote_id] = state_->get_quote(quote_id);
    }
    return values;
}

} // namespace qrp::analytics
