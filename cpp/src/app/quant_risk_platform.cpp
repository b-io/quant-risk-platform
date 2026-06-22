#include <qrp/app/quant_risk_platform.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <qrp/market/scenario_engine.hpp>
#include <qrp/analytics/pricing_context.hpp>
#include <qrp/util/logger.hpp>
#include <fstream>
#include <nlohmann/json.hpp>
#include <fmt/format.h>
#include <chrono>
#include <algorithm>
#include <map>
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

domain::FactorType parse_factor_type(const std::string& value) {
    static const std::map<std::string, domain::FactorType> factor_types = {
        {"BasisSpread", domain::FactorType::BasisSpread},
        {"CommodityForward", domain::FactorType::CommodityForward},
        {"CreditSpread", domain::FactorType::CreditSpread},
        {"Custom", domain::FactorType::Custom},
        {"EquitySpot", domain::FactorType::EquitySpot},
        {"FXSpot", domain::FactorType::FXSpot},
        {"HazardRate", domain::FactorType::HazardRate},
        {"RateForward", domain::FactorType::RateForward},
        {"RateZero", domain::FactorType::RateZero},
        {"Volatility", domain::FactorType::Volatility}
    };

    const auto it = factor_types.find(value);
    if (it == factor_types.end()) {
        throw std::invalid_argument("Unknown factor_type: " + value);
    }
    return it->second;
}

domain::ShockMeasure parse_shock_measure(const std::string& value) {
    static const std::map<std::string, domain::ShockMeasure> shock_measures = {
        {"Absolute", domain::ShockMeasure::Absolute},
        {"BasisPoints", domain::ShockMeasure::BasisPoints},
        {"LogReturn", domain::ShockMeasure::LogReturn},
        {"Relative", domain::ShockMeasure::Relative},
        {"VolPoints", domain::ShockMeasure::VolPoints}
    };

    const auto it = shock_measures.find(value);
    if (it == shock_measures.end()) {
        throw std::invalid_argument("Unknown shock_measure: " + value);
    }
    return it->second;
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
    if (!f.is_open()) throw std::runtime_error("Could not open market snapshot file: " + json_path);
    
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
        if (q.contains("tenor")) metadata["tenor"] = q["tenor"];
        metadata["instrument_type"] = q.at("instrument_type");
        if (q.contains("instrument_family")) metadata["instrument_family"] = q["instrument_family"];
        if (q.contains("index_family")) metadata["index_family"] = q["index_family"];
        if (q.contains("day_count")) metadata["day_count"] = q["day_count"];
        if (q.contains("calendar")) metadata["calendar"] = q["calendar"];
        if (q.contains("bdc")) metadata["bdc"] = q["bdc"];
        if (q.contains("settlement_days")) metadata["settlement_days"] = q["settlement_days"];

        storage_->store_market_quote(snapshot_id, q["id"], q["value"], q["currency"], metadata.dump());
    }
    
    spdlog::info("Successfully imported market snapshot: {}", snapshot_id);
}

void QuantRiskPlatform::import_portfolio(const std::string& json_path) {
    spdlog::info("Importing portfolio from: {}", json_path);

    std::ifstream f(json_path);
    if (!f.is_open()) throw std::runtime_error("Could not open portfolio file: " + json_path);
    
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
        if (start_date.empty()) start_date = t.value("effective_date", "");
        
        std::string maturity_date = t.value("maturity_date", "");
        if (maturity_date.empty()) maturity_date = t.value("termination_date", "");

        json economics = t.contains("details") ? t["details"] : (t.contains("economics") ? t["economics"] : json::object());
        
        storage_->store_trade(t["id"], portfolio_id, book_id, asset_class, trade_type, ccy, notional, start_date, maturity_date, direction, economics.dump());
    }

    spdlog::info("Successfully imported portfolio: {}", portfolio_id);
}

void QuantRiskPlatform::import_scenario_set(const std::string& json_path) {
    spdlog::info("Importing scenario set from: {}", json_path);
    std::ifstream f(json_path);
    if (!f.is_open()) throw std::runtime_error("Could not open scenario set file: " + json_path);
    
    json data = json::parse(f);
    std::string set_id = data["scenario_set_id"];
    std::string name = data.value("name", set_id);
    
    storage_->store_scenario_set(set_id, name);

    for (const auto& factor_json : data.at("factors")) {
        domain::FactorDefinition factor;
        factor.factor_id = factor_json.at("factor_id").get<std::string>();
        factor.factor_type = parse_factor_type(factor_json.at("factor_type").get<std::string>());
        factor.shock_measure = parse_shock_measure(factor_json.at("shock_measure").get<std::string>());
        factor.currency = domain::from_string(factor_json.value("currency", "UNKNOWN"));
        factor.curve_id = factor_json.value("curve_id", "");
        factor.tenor = factor_json.value("tenor", "");
        if (factor_json.contains("quote_ids")) {
            factor.quote_ids = factor_json.at("quote_ids").get<std::vector<std::string>>();
        }
        factor.description = factor_json.value("description", "");
        storage_->store_factor_definition(factor);
    }

    for (const auto& binding_json : data.at("bindings")) {
        domain::FactorBinding binding;
        binding.factor_id = binding_json.at("factor_id").get<std::string>();
        binding.quote_id = binding_json.at("quote_id").get<std::string>();
        binding.shock_measure = parse_shock_measure(binding_json.at("shock_measure").get<std::string>());
        binding.weight = binding_json.value("weight", 1.0);
        binding.transform = binding_json.value("transform", "");
        binding.selector_json = binding_json.value("selector_json", "");
        storage_->store_factor_binding(binding);
    }
    
    for (const auto& [sc_name, scenario_json] : data.at("scenarios").items()) {
        const auto& shocks = scenario_json.at("factor_shocks");
        for (const auto& [factor_id, shock] : shocks.items()) {
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
        storage_->store_valuation_result(run_id, res.trade_id, res.npv, res.currency, failed ? "FAILED" : "SUCCESS", error);
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
        for (const auto& [tenor, risk] : res.bucketed_risk) {
            storage_->store_risk_result(run_id, res.trade_id, "BUCKETED_RISK", tenor, risk);
        }
    }
    
    return run_id;
}

std::string QuantRiskPlatform::run_historical_var(const std::string& portfolio_id, const std::string& snapshot_id, const std::string& scenario_set_id) {
    spdlog::info("Running Historical VaR for portfolio {} using snapshot {} and scenario set {}", portfolio_id, snapshot_id, scenario_set_id);

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

    std::vector<double> pnl_distribution;
    std::vector<std::pair<std::string, double>> scenario_pnls;
    
    // Initial NPV
    market::MarketSnapshot market(market_dto);
    analytics::PricingContext context(market.built_state());
    auto base_results = analytics::ValuationService::price_portfolio(portfolio, context);
    double base_npv = 0;
    for (const auto& r : base_results) base_npv += r.npv;

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
        
        double shocked_npv = 0;
        for (const auto& r : shocked_results) shocked_npv += r.npv;
        
        double scenario_pnl = shocked_npv - base_npv;
        pnl_distribution.push_back(scenario_pnl);
        scenario_pnls.push_back({sc_name, scenario_pnl});
    }

    if (pnl_distribution.empty()) throw std::runtime_error("No scenarios to run for VaR");

    spdlog::debug("PnL distribution size: {}", pnl_distribution.size());
    for(auto p : pnl_distribution) spdlog::trace("PnL: {}", p);

    std::sort(pnl_distribution.begin(), pnl_distribution.end());
    
    // Simple 95% VaR (5th percentile of PnL)
    size_t idx_95 = static_cast<size_t>(0.05 * pnl_distribution.size());
    double var_95 = -pnl_distribution[idx_95]; // VaR is usually reported as a positive loss
    double es_sum = 0.0;
    for (size_t i = 0; i <= idx_95; ++i) {
        es_sum += pnl_distribution[i];
    }
    double expected_shortfall_95 = -(es_sum / static_cast<double>(idx_95 + 1));

    std::string run_id = fmt::format("RUN:HVAR:{}", std::chrono::system_clock::now().time_since_epoch().count());
    storage_->store_analysis_run(run_id, "HISTORICAL_VAR", portfolio_id, snapshot_id);

    for (const auto& [scenario_name, scenario_pnl] : scenario_pnls) {
        storage_->store_scenario_result(run_id, scenario_name, scenario_pnl);
    }
    storage_->store_var_result(
        run_id,
        "HISTORICAL",
        0.95,
        var_95,
        expected_shortfall_95,
        static_cast<int>(pnl_distribution.size()));

    spdlog::info("Historical VaR 95% run completed. VaR: {}, ES: {}", var_95, expected_shortfall_95);
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

    fmt::print("{:<20} | {:<15} | {:<10} | {:<10}\n", "Trade ID", "NPV", "CCY", "Status");
    fmt::print("{:-<60}\n", "");
    for (const auto& r : results) {
        fmt::print("{:<20} | {:>15.2f} | {:<10} | {:<10}\n", r.trade_id, r.npv, r.ccy, r.status);
        if (r.ccy == base_ccy) total_npv += r.npv;
    }
    fmt::print("{:-<60}\n", "");
    fmt::print("{:<20} | {:>15.2f} | {:<10}\n", "TOTAL (Base CCY)", total_npv, base_ccy);
}

void QuantRiskPlatform::compare_runs(const std::string& run_id_1, const std::string& run_id_2) {
    spdlog::info("--- Comparing Run {} vs {} ---", run_id_1, run_id_2);
    auto res1 = storage_->get_valuation_results(run_id_1);
    auto res2 = storage_->get_valuation_results(run_id_2);

    std::map<std::string, double> map1;
    for (const auto& r : res1) map1[r.trade_id] = r.npv;

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
