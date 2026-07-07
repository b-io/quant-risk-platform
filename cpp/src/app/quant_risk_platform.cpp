// Implements the high-level application facade used by CLI, tests, and Python bindings.

#include <qrp/analytics/pnl_explain_service.hpp>
#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/var_contribution_service.hpp>
#include <qrp/app/quant_risk_platform.hpp>
#include <qrp/domain/factors.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <qrp/market/scenario_engine.hpp>
#include <qrp/util/logger.hpp>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

using json = nlohmann::json;

namespace qrp::app {

namespace {

std::vector<std::string> collect_factor_ids(const std::vector<domain::FactorDefinition>& factors) {
    std::vector<std::string> factor_ids;
    factor_ids.reserve(factors.size());
    for (const auto& factor : factors) {
        factor_ids.push_back(factor.factor_id);
    }
    return factor_ids;
}

std::string component_type_to_string(analytics::PnlExplainComponentType component_type) {
    switch (component_type) {
        case analytics::PnlExplainComponentType::Carry:
            return "carry";
        case analytics::PnlExplainComponentType::Cash:
            return "cash";
        case analytics::PnlExplainComponentType::FxTranslation:
            return "fx_translation";
        case analytics::PnlExplainComponentType::MarketMove:
            return "market_move";
        case analytics::PnlExplainComponentType::ModelChange:
            return "model_change";
        case analytics::PnlExplainComponentType::RollDown:
            return "roll_down";
        case analytics::PnlExplainComponentType::TradeActivity:
            return "trade_activity";
        case analytics::PnlExplainComponentType::Residual:
            return "residual";
    }
    return "residual";
}

persistence::StorageBackend::PnlExplainRecord make_pnl_explain_record(const std::string& run_id,
                                                                      const analytics::PnlExplainResult& result) {
    persistence::StorageBackend::PnlExplainRecord record;
    record.run_id = run_id;
    record.trade_id = result.trade_id;
    record.asset_class = result.asset_class;
    record.book = result.book;
    record.currency = result.currency;
    record.product_type = result.product_type;
    record.strategy = result.strategy;
    record.prev_npv = result.prev_npv;
    record.curr_npv = result.curr_npv;
    record.total_pnl = result.total_pnl;
    record.carry_pnl = result.carry_pnl;
    record.roll_down_pnl = result.roll_down_pnl;
    record.market_move_pnl = result.market_move_pnl;
    record.cash_pnl = result.cash_pnl;
    record.trade_activity_pnl = result.trade_activity_pnl;
    record.fx_translation_pnl = result.fx_translation_pnl;
    record.model_change_pnl = result.model_change_pnl;
    record.residual_pnl = result.residual_pnl;
    record.explained_pnl = result.explained_pnl;
    record.reconciliation_difference = result.reconciliation_difference;
    record.reconciliation_passed = result.reconciliation_passed;
    record.support_status = domain::to_string(result.curr_valuation.support_status);
    record.model_name = result.curr_valuation.model_name;
    record.status_message = result.curr_valuation.status_message;
    return record;
}

persistence::StorageBackend::PnlExplainComponentRecord
make_pnl_explain_component_record(const std::string& run_id,
                                  const analytics::PnlExplainResult& result,
                                  const analytics::PnlExplainComponent& component) {
    persistence::StorageBackend::PnlExplainComponentRecord record;
    record.run_id = run_id;
    record.trade_id = result.trade_id;
    record.sequence = component.sequence;
    record.component_id = component.component_id;
    record.component_type = component_type_to_string(component.component_type);
    record.label = component.label;
    record.amount = component.amount;
    record.factor_id = component.factor_id;
    record.risk_factor_group = component.risk_factor_group;
    record.model_name = component.model_name;
    record.support_status = domain::to_string(component.support_status);
    record.status_message = component.status_message;
    record.tags_json = json(component.tags).dump();
    return record;
}

std::map<std::string, double> npv_by_trade(const std::vector<analytics::ValuationResult>& results) {
    std::map<std::string, double> values;
    for (const auto& result : results) {
        values[result.trade_id] = result.npv;
    }
    return values;
}

persistence::StorageBackend::VarScenarioPnlRecord
make_var_scenario_pnl_record(const std::string& run_id,
                             const analytics::HistoricalScenarioPnl& scenario,
                             const std::string& trade_id,
                             double trade_pnl) {
    persistence::StorageBackend::VarScenarioPnlRecord record;
    record.run_id = run_id;
    record.scenario_name = scenario.scenario_name;
    record.trade_id = trade_id;
    record.portfolio_pnl = scenario.portfolio_pnl;
    record.trade_pnl = trade_pnl;
    return record;
}

persistence::StorageBackend::VarContributionRecord
make_var_contribution_record(const std::string& run_id,
                             const analytics::VarContributionReport& report,
                             const analytics::VarContribution& contribution) {
    persistence::StorageBackend::VarContributionRecord record;
    record.run_id = run_id;
    record.method = report.method;
    record.confidence_level = contribution.confidence_level;
    record.aggregation_type = contribution.aggregation_type;
    record.aggregation_key = contribution.aggregation_key;
    record.var_contribution = contribution.var_contribution;
    record.expected_shortfall_contribution = contribution.expected_shortfall_contribution;
    record.portfolio_var_share = contribution.portfolio_var_share;
    record.portfolio_expected_shortfall_share = contribution.portfolio_expected_shortfall_share;
    record.standalone_var = contribution.standalone_var;
    record.standalone_expected_shortfall = contribution.standalone_expected_shortfall;
    record.incremental_var = contribution.incremental_var;
    record.incremental_expected_shortfall = contribution.incremental_expected_shortfall;
    record.marginal_var = contribution.marginal_var;
    record.marginal_expected_shortfall = contribution.marginal_expected_shortfall;
    record.scenario_count = contribution.scenario_count;
    record.tail_scenario_count = contribution.tail_scenario_count;
    record.var_scenario_name = contribution.var_scenario_name;
    record.sign_convention = contribution.sign_convention;
    record.aggregation_rule = contribution.aggregation_rule;
    record.calculation_method = contribution.calculation_method;
    return record;
}

} // namespace

QuantRiskPlatform::QuantRiskPlatform(std::shared_ptr<persistence::StorageBackend> storage)
    : storage_(std::move(storage)) {}

void QuantRiskPlatform::initialize() {
    spdlog::info("Initializing Quant Risk Platform...");
    // Why: Centralized schema initialization ensures all storage entities
    // (portfolios, market, runs) are ready before use.
    storage_->initialize_schema();
}

void QuantRiskPlatform::import_market_snapshot(const std::string& json_path) {
    spdlog::info("Importing market snapshot from: {}", json_path);

    std::ifstream f(json_path);
    if (!f.is_open())
        throw std::runtime_error("Could not open market snapshot file: " + json_path);

    json data = json::parse(f);

    // Why: Snapshot IDs are prefixed to distinguish them from raw dates
    // and enable versioning of market states.
    std::string as_of_date = data["valuation_date"];
    std::string snapshot_id = "SNAP:" + as_of_date;
    std::string base_ccy = data.value("base_currency", "USD");

    std::string curves_json = data.contains("curves") ? data["curves"].dump() : "[]";
    storage_->store_market_snapshot(snapshot_id, as_of_date, base_ccy, curves_json);

    for (const auto& q : data["quotes"]) {
        json metadata;
        if (q.contains("tenor"))
            metadata["tenor"] = q["tenor"];
        metadata["instrument_type"] = q.at("instrument_type");
        if (q.contains("instrument_family"))
            metadata["instrument_family"] = q["instrument_family"];
        if (q.contains("index_family"))
            metadata["index_family"] = q["index_family"];
        if (q.contains("day_count"))
            metadata["day_count"] = q["day_count"];
        if (q.contains("calendar"))
            metadata["calendar"] = q["calendar"];
        if (q.contains("bdc"))
            metadata["bdc"] = q["bdc"];
        if (q.contains("settlement_days"))
            metadata["settlement_days"] = q["settlement_days"];

        storage_->store_market_quote(snapshot_id, q["id"], q["value"], q["currency"], metadata.dump());
    }

    spdlog::info("Successfully imported market snapshot: {}", snapshot_id);
}

void QuantRiskPlatform::import_portfolio(const std::string& json_path) {
    spdlog::info("Importing portfolio from: {}", json_path);

    std::ifstream f(json_path);
    if (!f.is_open())
        throw std::runtime_error("Could not open portfolio file: " + json_path);

    json data = json::parse(f);

    std::string portfolio_id = data["portfolio_id"];
    std::string portfolio_name = data.value("portfolio_name", portfolio_id);
    std::string base_ccy = data.value("base_currency", "USD");

    storage_->store_portfolio(portfolio_id, portfolio_name, base_ccy);

    for (const auto& t : data["trades"]) {
        std::string book_id = t.value("book", "DEFAULT_BOOK");
        storage_->store_book(book_id, portfolio_id, book_id);

        std::string asset_class = t["asset_class"];
        std::string trade_type = t.at("type").get<std::string>();
        std::string ccy = t["currency"];
        double notional = t.value("notional", t.value("quantity", 0.0));

        std::string direction = t.value("direction", "unknown");
        std::string start_date = t.value("start_date", "");
        if (start_date.empty())
            start_date = t.value("effective_date", "");

        std::string maturity_date = t.value("maturity_date", "");
        if (maturity_date.empty())
            maturity_date = t.value("termination_date", "");

        json economics =
            t.contains("details") ? t["details"] : (t.contains("economics") ? t["economics"] : json::object());

        storage_->store_trade(t["id"],
                              portfolio_id,
                              book_id,
                              asset_class,
                              trade_type,
                              ccy,
                              notional,
                              start_date,
                              maturity_date,
                              direction,
                              economics.dump());
    }

    spdlog::info("Successfully imported portfolio: {}", portfolio_id);
}

void QuantRiskPlatform::import_scenario_set(const std::string& json_path) {
    spdlog::info("Importing scenario set from: {}", json_path);
    std::ifstream f(json_path);
    if (!f.is_open())
        throw std::runtime_error("Could not open scenario set file: " + json_path);

    json data = json::parse(f);
    std::string set_id = data["scenario_set_id"];
    std::string name = data.value("name", set_id);

    storage_->store_scenario_set(set_id, name);
    std::set<std::string> factor_ids;

    for (const auto& factor_json : data.at("factors")) {
        domain::FactorDefinition factor;
        factor.factor_id = factor_json.at("factor_id").get<std::string>();
        if (!domain::is_canonical_factor_id(factor.factor_id)) {
            throw std::invalid_argument("Non-canonical factor_id in scenario set: " + factor.factor_id);
        }
        factor.factor_type = domain::parse_factor_type(factor_json.at("factor_type").get<std::string>());
        factor.shock_measure = domain::parse_shock_measure(factor_json.at("shock_measure").get<std::string>());
        factor.currency = domain::from_string(factor_json.value("currency", "UNKNOWN"));
        factor.curve_id = factor_json.value("curve_id", "");
        factor.tenor = factor_json.value("tenor", "");
        if (factor_json.contains("quote_ids")) {
            factor.quote_ids = factor_json.at("quote_ids").get<std::vector<std::string>>();
        }
        factor.description = factor_json.value("description", "");
        storage_->store_factor_definition(factor);
        factor_ids.insert(factor.factor_id);
    }

    for (const auto& binding_json : data.at("bindings")) {
        domain::FactorBinding binding;
        binding.factor_id = binding_json.at("factor_id").get<std::string>();
        if (!factor_ids.contains(binding.factor_id)) {
            throw std::invalid_argument("Binding references undefined factor_id: " + binding.factor_id);
        }
        binding.quote_id = binding_json.at("quote_id").get<std::string>();
        binding.shock_measure = domain::parse_shock_measure(binding_json.at("shock_measure").get<std::string>());
        binding.weight = binding_json.value("weight", 1.0);
        binding.transform = binding_json.value("transform", "");
        binding.selector_json = binding_json.value("selector_json", "");
        storage_->store_factor_binding(binding);
    }

    for (const auto& [sc_name, scenario_json] : data.at("scenarios").items()) {
        const auto& shocks = scenario_json.at("factor_shocks");
        for (const auto& [factor_id, shock] : shocks.items()) {
            if (!factor_ids.contains(factor_id)) {
                throw std::invalid_argument("Scenario references undefined factor_id: " + factor_id);
            }
            storage_->store_scenario_factor_shock(set_id, sc_name, factor_id, shock.get<double>());
        }
    }
    spdlog::info("Successfully imported scenario set: {}", set_id);
}

std::string QuantRiskPlatform::run_valuation(const std::string& portfolio_id, const std::string& snapshot_id) {
    spdlog::info("Running valuation for portfolio {} and snapshot {}", portfolio_id, snapshot_id);

    auto trades = storage_->load_trades(portfolio_id);
    auto market_dto = storage_->load_market_snapshot(snapshot_id);

    market::MarketSnapshot market(market_dto);
    analytics::PricingContext context(market.built_state());

    domain::Portfolio portfolio;
    portfolio.portfolio_id = portfolio_id;
    portfolio.trades = trades;

    auto results = analytics::ValuationService::price_portfolio(portfolio, context);

    std::string run_id = fmt::format("RUN_VAL_{}", std::chrono::system_clock::now().time_since_epoch().count());
    storage_->store_analysis_run(run_id, "VALUATION", portfolio_id, snapshot_id);

    for (const auto& res : results) {
        const auto status_it = res.tags.find("status");
        const bool failed = status_it != res.tags.end() && status_it->second == "failed";
        const auto error_it = res.tags.find("error");
        const std::string error = error_it != res.tags.end() ? error_it->second : "";
        storage_->store_valuation_result(run_id,
                                         res.trade_id,
                                         res.npv,
                                         res.currency,
                                         failed ? "FAILED" : "SUCCESS",
                                         error,
                                         domain::to_string(res.asset_class),
                                         domain::to_string(res.product_type),
                                         domain::to_string(res.support_status),
                                         res.model_name,
                                         res.status_message);
    }

    return run_id;
}

std::string QuantRiskPlatform::run_risk(const std::string& portfolio_id, const std::string& snapshot_id) {
    spdlog::info("Running risk for portfolio {} and snapshot {}", portfolio_id, snapshot_id);

    auto trades = storage_->load_trades(portfolio_id);
    auto market_dto = storage_->load_market_snapshot(snapshot_id);

    domain::Portfolio portfolio;
    portfolio.portfolio_id = portfolio_id;
    portfolio.trades = trades;

    auto factors = storage_->load_factor_definitions(portfolio_id);
    auto bindings = storage_->load_factor_bindings(collect_factor_ids(factors));
    if (factors.empty() || bindings.empty()) {
        throw std::runtime_error("Risk requires imported factor definitions and factor quote bindings");
    }

    auto results = analytics::RiskService::compute_risk(portfolio, market_dto, factors, bindings);

    std::string run_id = fmt::format("RUN_RISK_{}", std::chrono::system_clock::now().time_since_epoch().count());
    storage_->store_analysis_run(run_id, "RISK", portfolio_id, snapshot_id);

    for (const auto& res : results) {
        storage_->store_risk_result(run_id, res.trade_id, "PV01", "ALL", res.pv01);
        storage_->store_risk_result(run_id, res.trade_id, "FX_DELTA", "ALL", res.fx_delta);
        storage_->store_risk_result(run_id, res.trade_id, "FX_VEGA", "ALL", res.fx_vega);
        for (const auto& [tenor, risk] : res.bucketed_risk) {
            storage_->store_risk_result(run_id, res.trade_id, "BUCKETED_RISK", tenor, risk);
        }
    }

    return run_id;
}

std::string QuantRiskPlatform::run_pnl_explain(const std::string& portfolio_id,
                                               const std::string& previous_snapshot_id,
                                               const std::string& current_snapshot_id) {
    spdlog::info("Running PnL explain for portfolio {} from snapshot {} to {}",
                 portfolio_id,
                 previous_snapshot_id,
                 current_snapshot_id);

    auto trades = storage_->load_trades(portfolio_id);
    auto previous_market_dto = storage_->load_market_snapshot(previous_snapshot_id);
    auto current_market_dto = storage_->load_market_snapshot(current_snapshot_id);

    domain::Portfolio portfolio;
    portfolio.portfolio_id = portfolio_id;
    portfolio.trades = trades;

    auto factors = storage_->load_factor_definitions(portfolio_id);
    auto bindings = storage_->load_factor_bindings(collect_factor_ids(factors));
    if (factors.empty() || bindings.empty()) {
        factors.clear();
        bindings.clear();
    }

    auto results = analytics::PnlExplainService::explain_pnl(portfolio,
                                                             previous_market_dto,
                                                             current_market_dto,
                                                             factors,
                                                             bindings);

    std::string run_id = fmt::format("RUN_PNL_{}", std::chrono::system_clock::now().time_since_epoch().count());
    storage_->store_analysis_run(run_id, "PNL_EXPLAIN", portfolio_id, current_snapshot_id);

    for (const auto& result : results) {
        storage_->store_pnl_explain_result(make_pnl_explain_record(run_id, result));
        for (const auto& component : result.components) {
            storage_->store_pnl_explain_component(make_pnl_explain_component_record(run_id, result, component));
        }
    }

    return run_id;
}

std::string QuantRiskPlatform::run_historical_var(const std::string& portfolio_id,
                                                  const std::string& snapshot_id,
                                                  const std::string& scenario_set_id) {
    spdlog::info("Running Historical VaR for portfolio {} using snapshot {} and scenario set {}",
                 portfolio_id,
                 snapshot_id,
                 scenario_set_id);

    auto trades = storage_->load_trades(portfolio_id);
    auto market_dto = storage_->load_market_snapshot(snapshot_id);
    auto scenarios = storage_->load_scenario_set(scenario_set_id);
    auto factors = storage_->load_factor_definitions(portfolio_id);
    auto bindings = storage_->load_factor_bindings(collect_factor_ids(factors));

    if (factors.empty() || bindings.empty()) {
        throw std::runtime_error("Historical VaR requires imported factor definitions and factor quote bindings");
    }

    domain::Portfolio portfolio;
    portfolio.portfolio_id = portfolio_id;
    portfolio.trades = trades;

    std::vector<analytics::HistoricalScenarioPnl> scenario_pnls;

    // Initial NPV
    market::MarketSnapshot market(market_dto);
    analytics::PricingContext context(market.built_state());
    auto base_results = analytics::ValuationService::price_portfolio(portfolio, context);
    const auto base_trade_npvs = npv_by_trade(base_results);
    double base_npv = 0.0;
    for (const auto& r : base_results) {
        base_npv += r.npv;
    }

    for (const auto& [sc_name, factor_shocks] : scenarios) {
        spdlog::debug("Applying scenario: {} with {} shocks", sc_name, factor_shocks.size());
        market::MarketSnapshot shocked_market(market_dto);
        auto shocked_state = shocked_market.built_state();
        market::ScenarioDefinition scenario;
        scenario.name = sc_name;
        scenario.factor_shocks = factor_shocks;
        market::ScenarioEngine::apply_scenario_to_state(*shocked_state, market_dto, scenario, factors, bindings);
        analytics::PricingContext shocked_context(shocked_state);
        auto shocked_results = analytics::ValuationService::price_portfolio(portfolio, shocked_context);

        analytics::HistoricalScenarioPnl scenario_pnl;
        scenario_pnl.scenario_name = sc_name;
        scenario_pnl.factor_shocks = factor_shocks;

        double shocked_npv = 0.0;
        for (const auto& r : shocked_results) {
            shocked_npv += r.npv;
            const auto base_it = base_trade_npvs.find(r.trade_id);
            const double base_trade_npv = base_it == base_trade_npvs.end() ? 0.0 : base_it->second;
            scenario_pnl.trade_pnls[r.trade_id] = r.npv - base_trade_npv;
        }

        scenario_pnl.portfolio_pnl = shocked_npv - base_npv;
        scenario_pnls.push_back(std::move(scenario_pnl));
    }

    if (scenario_pnls.empty())
        throw std::runtime_error("No scenarios to run for VaR");

    auto contribution_report =
        analytics::VarContributionService::calculate_historical_contributions(portfolio, scenario_pnls, 0.95);
    spdlog::debug("PnL distribution size: {}", contribution_report.scenario_count);
    for (const auto& scenario_pnl : scenario_pnls) {
        spdlog::trace("Scenario {} PnL: {}", scenario_pnl.scenario_name, scenario_pnl.portfolio_pnl);
    }

    std::string run_id = fmt::format("RUN:HVAR:{}", std::chrono::system_clock::now().time_since_epoch().count());
    storage_->store_analysis_run(run_id, "HISTORICAL_VAR", portfolio_id, snapshot_id);

    for (const auto& scenario_pnl : contribution_report.scenario_pnls) {
        storage_->store_scenario_result(run_id, scenario_pnl.scenario_name, scenario_pnl.portfolio_pnl);
        for (const auto& [trade_id, trade_pnl] : scenario_pnl.trade_pnls) {
            storage_->store_var_scenario_pnl(make_var_scenario_pnl_record(run_id, scenario_pnl, trade_id, trade_pnl));
        }
    }
    storage_->store_var_result(run_id,
                               contribution_report.method,
                               contribution_report.confidence_level,
                               contribution_report.var_value,
                               contribution_report.expected_shortfall,
                               contribution_report.scenario_count);
    for (const auto& contribution : contribution_report.contributions) {
        storage_->store_var_contribution(make_var_contribution_record(run_id, contribution_report, contribution));
    }

    spdlog::info("Historical VaR 95% run completed. VaR: {}, ES: {}",
                 contribution_report.var_value,
                 contribution_report.expected_shortfall);
    return run_id;
}

void QuantRiskPlatform::get_run_report(const std::string& run_id) {
    spdlog::info("--- Run Report: {} ---", run_id);
    auto results = storage_->get_valuation_results(run_id);
    if (results.empty()) {
        spdlog::warn("No valuation results found for this run.");
        return;
    }

    double total_npv = 0;
    std::string base_ccy = results[0].ccy;

    fmt::print("{:<20} | {:<15} | {:<10} | {:<18} | {:<20}\n", "Trade ID", "NPV", "CCY", "Product", "Support");
    fmt::print("{:-<95}\n", "");
    for (const auto& r : results) {
        fmt::print("{:<20} | {:>15.2f} | {:<10} | {:<18} | {:<20}\n",
                   r.trade_id,
                   r.npv,
                   r.ccy,
                   r.product_type,
                   r.support_status);
        if (r.ccy == base_ccy)
            total_npv += r.npv;
    }
    fmt::print("{:-<95}\n", "");
    fmt::print("{:<20} | {:>15.2f} | {:<10}\n", "TOTAL (Base CCY)", total_npv, base_ccy);
}

void QuantRiskPlatform::compare_runs(const std::string& run_id_1, const std::string& run_id_2) {
    spdlog::info("--- Comparing Run {} vs {} ---", run_id_1, run_id_2);
    auto res1 = storage_->get_valuation_results(run_id_1);
    auto res2 = storage_->get_valuation_results(run_id_2);

    std::map<std::string, double> map1;
    for (const auto& r : res1)
        map1[r.trade_id] = r.npv;

    fmt::print("{:<20} | {:>15} | {:>15} | {:>15}\n", "Trade ID", "Run 1 NPV", "Run 2 NPV", "Diff");
    fmt::print("{:-<70}\n", "");

    double total_diff = 0;
    for (const auto& r2 : res2) {
        double npv1 = map1.contains(r2.trade_id) ? map1.at(r2.trade_id) : 0.0;
        double diff = r2.npv - npv1;
        fmt::print("{:<20} | {:>15.2f} | {:>15.2f} | {:>15.2f}\n", r2.trade_id, npv1, r2.npv, diff);
        total_diff += diff;
    }
    fmt::print("{:-<70}\n", "");
    fmt::print("{:<20} | {:>49.2f}\n", "TOTAL DIFF", total_diff);
}

void QuantRiskPlatform::list_data() {
    auto portfolios = storage_->list_portfolios();
    auto snapshots = storage_->list_snapshots();

    spdlog::info("--- Portfolios ---");
    for (const auto& p : portfolios) {
        spdlog::info(" - {}", p);
        auto runs = storage_->list_runs(p);
        for (const auto& r : runs) {
            spdlog::info("    * Run: {}", r);
        }
    }

    spdlog::info("--- Market Snapshots ---");
    for (const auto& s : snapshots) {
        spdlog::info(" - {}", s);
    }
}

} // namespace qrp::app
