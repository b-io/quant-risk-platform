#pragma once

#include <qrp/persistence/storage_backend.hpp>
#include <sqlite3.h>
#include <string>
#include <stdexcept>

namespace qrp::persistence {

/**
 * @brief SQLite-backed implementation of the StorageBackend.
 * 
 * Why SQLite?
 * Zero configuration, file-based, deterministic, and supports standard SQL.
 * It is the ideal default for local development and historical run persistence.
 */
class SQLiteStorageBackend : public StorageBackend {
public:
    explicit SQLiteStorageBackend(const std::string& db_path);
    ~SQLiteStorageBackend() override;

    void initialize_schema() override;

    void store_portfolio(const std::string& portfolio_id, const std::string& name, const std::string& base_ccy) override;
    void store_book(const std::string& book_id, const std::string& portfolio_id, const std::string& name) override;
    void store_trade(const std::string& trade_id, const std::string& portfolio_id, const std::string& book_id,
                     const std::string& asset_class, const std::string& product_type, const std::string& ccy,
                     double notional, const std::string& start_date, const std::string& maturity_date,
                     const std::string& direction, const std::string& economics_json) override;

    void store_market_snapshot(const std::string& snapshot_id, const std::string& as_of_date, const std::string& base_ccy, const std::string& curves_json = "[]") override;
    void store_market_quote(const std::string& snapshot_id, const std::string& quote_id, double value, const std::string& ccy, const std::string& metadata_json = "{}") override;

    /**
     * @brief Loads all trades for a given portfolio from storage.
     * Rebuilds domain::Trade objects including their JSON economics.
     */
    std::vector<domain::Trade> load_trades(const std::string& portfolio_id) override;

    /**
     * @brief Loads a complete market snapshot.
     * Fetches metadata and all associated quotes to rebuild the domain::MarketSnapshot DTO.
     */
    domain::MarketSnapshot load_market_snapshot(const std::string& snapshot_id) override;

    void store_analysis_run(const std::string& run_id, const std::string& type, const std::string& portfolio_id, const std::string& snapshot_id) override;

    /**
     * @brief Stores the result of a single trade valuation.
     * Includes NPV, currency, and any error message if valuation failed.
     */
    void store_valuation_result(const std::string& run_id, const std::string& trade_id, double npv, const std::string& ccy, const std::string& status, const std::string& error) override;

    /**
     * @brief Stores a single risk measure result (e.g., PV01, Delta).
     * Maps the result to a specific risk factor (or 'ALL' for aggregate risk).
     */
    void store_risk_result(const std::string& run_id, const std::string& trade_id, const std::string& measure, const std::string& rf_id, double value) override;

    void store_scenario_set(const std::string& set_id, const std::string& name) override;
    void store_scenario_quote_shock(const std::string& set_id, const std::string& scenario_name, const std::string& quote_id, double shock) override;
    std::map<std::string, std::map<std::string, double>> load_scenario_set(const std::string& set_id) override;

    std::vector<std::string> list_portfolios() override;
    std::vector<std::string> list_snapshots() override;
    std::vector<std::string> list_runs(const std::string& portfolio_id) override;

    std::vector<ValuationRecord> get_valuation_results(const std::string& run_id) override;

private:
    void execute_sql(const std::string& sql);
    
    sqlite3* db_ = nullptr;
    std::string db_path_;
};

} // namespace qrp::persistence
