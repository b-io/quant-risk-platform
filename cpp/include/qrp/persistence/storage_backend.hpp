#pragma once

#include <string>
#include <vector>
#include <memory>
#include <qrp/domain/portfolio.hpp>
#include <qrp/domain/market_data.hpp>

namespace qrp::persistence {

/**
 * @brief Abstract interface for the platform's storage layer.
 * 
 * Why this interface?
 * To decouple the analytics services and the application facade from a specific database implementation.
 * While SQLite is the default, this allows future backends (PostgreSQL, DuckDB) without service changes.
 */
class StorageBackend {
public:
    virtual ~StorageBackend() = default;

    virtual void initialize_schema() = 0;
    
    // Static / reference-like entities
    virtual void store_portfolio(const std::string& portfolio_id, const std::string& name, const std::string& base_ccy) = 0;
    virtual void store_book(const std::string& book_id, const std::string& portfolio_id, const std::string& name) = 0;
    virtual void store_trade(const std::string& trade_id, const std::string& portfolio_id, const std::string& book_id,
                             const std::string& asset_class, const std::string& product_type, const std::string& ccy,
                             double notional, const std::string& start_date, const std::string& maturity_date,
                             const std::string& direction, const std::string& economics_json) = 0;

    // Market snapshots
    virtual void store_market_snapshot(const std::string& snapshot_id, const std::string& as_of_date, const std::string& base_ccy, const std::string& curves_json = "[]") = 0;
    virtual void store_market_quote(const std::string& snapshot_id, const std::string& quote_id, double value, const std::string& ccy, const std::string& metadata_json = "{}") = 0;

    // Load methods
    virtual std::vector<domain::Trade> load_trades(const std::string& portfolio_id) = 0;
    virtual domain::MarketSnapshot load_market_snapshot(const std::string& snapshot_id) = 0;

    // Run persistence
    virtual void store_analysis_run(const std::string& run_id, const std::string& type, const std::string& portfolio_id, const std::string& snapshot_id) = 0;
    virtual void store_valuation_result(const std::string& run_id, const std::string& trade_id, double npv, const std::string& ccy, const std::string& status, const std::string& error) = 0;
    virtual void store_risk_result(const std::string& run_id, const std::string& trade_id, const std::string& measure, const std::string& rf_id, double value) = 0;

    // Scenarios
    virtual void store_scenario_set(const std::string& set_id, const std::string& name) = 0;
    virtual void store_scenario_quote_shock(const std::string& set_id, const std::string& scenario_name, const std::string& quote_id, double shock) = 0;
    virtual std::map<std::string, std::map<std::string, double>> load_scenario_set(const std::string& set_id) = 0;

    // Ad hoc queries
    virtual std::vector<std::string> list_portfolios() = 0;
    virtual std::vector<std::string> list_snapshots() = 0;
    virtual std::vector<std::string> list_runs(const std::string& portfolio_id) = 0;

    /**
     * @brief Result record for ad hoc valuation queries.
     */
    struct ValuationRecord {
        std::string run_id;
        std::string trade_id;
        double npv;
        std::string ccy;
        std::string status;
    };

    /**
     * @brief Fetches all valuation results for a given run.
     */
    virtual std::vector<ValuationRecord> get_valuation_results(const std::string& run_id) = 0;
};

} // namespace qrp::persistence
