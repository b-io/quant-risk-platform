#pragma once

// Declares the SQLite implementation of the platform storage backend.

#include <qrp/persistence/storage_backend.hpp>
#include <sqlite3.h>
#include <stdexcept>
#include <string>

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
    /**
     * @brief Opens a SQLite database at the supplied path.
     */
    explicit SQLiteStorageBackend(const std::string& db_path);

    /**
     * @brief Closes the owned SQLite connection.
     */
    ~SQLiteStorageBackend() override;

    /**
     * @brief Creates or migrates all SQLite schema objects.
     */
    void initialize_schema() override;

    /**
     * @brief Stores a portfolio header.
     */
    void store_portfolio(const std::string& portfolio_id, const std::string& name, const std::string& base_ccy) override;

    /**
     * @brief Stores a book header associated with a portfolio.
     */
    void store_book(const std::string& book_id, const std::string& portfolio_id, const std::string& name) override;

    /**
     * @brief Stores a normalized trade row and economics JSON.
     */
    void store_trade(const std::string& trade_id, const std::string& portfolio_id, const std::string& book_id,
                     const std::string& asset_class, const std::string& trade_type, const std::string& ccy,
                     double notional, const std::string& start_date, const std::string& maturity_date,
                     const std::string& direction, const std::string& economics_json) override;

    /**
     * @brief Stores a market snapshot header and curve specification payload.
     */
    void store_market_snapshot(const std::string& snapshot_id, const std::string& as_of_date, const std::string& base_ccy, const std::string& curves_json = "[]") override;

    /**
     * @brief Stores a raw quote attached to a market snapshot.
     */
    void store_market_quote(const std::string& snapshot_id, const std::string& quote_id, double value, const std::string& ccy, const std::string& metadata_json = "{}") override;

    /**
     * @brief Stores one bitemporal market quote event.
     */
    void store_market_quote_event(const domain::MarketQuoteEvent& event) override;

    /**
     * @brief Reconstructs a market snapshot as of market and knowledge timestamps.
     */
    domain::MarketSnapshot load_market_snapshot_asof(
        const std::string& market_ts,
        const std::string& recorded_ts,
        const std::string& base_ccy,
        const std::string& overlay_set_id = "") override;

    /**
     * @brief Stores a risk-factor definition.
     */
    void store_factor_definition(const domain::FactorDefinition& factor) override;

    /**
     * @brief Stores one historical factor observation.
     */
    void store_factor_observation(const domain::FactorObservation& obs) override;

    /**
     * @brief Stores one factor-to-quote binding.
     */
    void store_factor_binding(const domain::FactorBinding& binding) override;

    /**
     * @brief Loads factor definitions relevant to a portfolio.
     */
    std::vector<domain::FactorDefinition> load_factor_definitions(const std::string& portfolio_id) override;

    /**
     * @brief Loads quote bindings for requested factor ids.
     */
    std::vector<domain::FactorBinding> load_factor_bindings(const std::vector<std::string>& factor_ids) override;

    /**
     * @brief Loads historical observations for requested factors over a date range.
     */
    std::vector<domain::FactorObservation> load_factor_history(
        const std::vector<std::string>& factor_ids,
        const std::string& start_date,
        const std::string& end_date) override;

    /**
     * @brief Loads all trades for a given portfolio from storage.
     * Rebuilds domain::Trade objects including their JSON economics.
     */
    std::vector<std::shared_ptr<domain::Trade>> load_trades(const std::string& portfolio_id) override;

    /**
     * @brief Loads a complete market snapshot.
     * Fetches metadata and all associated quotes to rebuild the domain::MarketSnapshot DTO.
     */
    domain::MarketSnapshot load_market_snapshot(const std::string& snapshot_id) override;

    /**
     * @brief Stores an analysis run header.
     */
    void store_analysis_run(const std::string& run_id, const std::string& type, const std::string& portfolio_id, const std::string& snapshot_id) override;

    /**
     * @brief Stores aggregate PnL for one scenario in a run.
     */
    void store_scenario_result(const std::string& run_id, const std::string& scenario_name, double portfolio_pnl) override;

    /**
     * @brief Stores the result of a single trade valuation.
     * Includes NPV, currency, and any error message if valuation failed.
     */
    void store_valuation_result(
        const std::string& run_id,
        const std::string& trade_id,
        double npv,
        const std::string& ccy,
        const std::string& status,
        const std::string& error,
        const std::string& asset_class,
        const std::string& product_type,
        const std::string& support_status,
        const std::string& model_name,
        const std::string& status_message) override;

    /**
     * @brief Stores aggregate VaR and ES metrics for a run.
     */
    void store_var_result(const std::string& run_id, const std::string& method, double confidence_level, double var_value, double expected_shortfall, int scenario_count) override;

    /**
     * @brief Stores a single risk measure result (e.g., PV01, Delta).
     * Maps the result to a specific risk factor (or 'ALL' for aggregate risk).
     */
    void store_risk_result(const std::string& run_id, const std::string& trade_id, const std::string& measure, const std::string& rf_id, double value) override;

    /**
     * @brief Stores a scenario set header.
     */
    void store_scenario_set(const std::string& set_id, const std::string& name) override;

    /**
     * @brief Stores one factor shock within a named scenario.
     */
    void store_scenario_factor_shock(const std::string& set_id, const std::string& scenario_name, const std::string& factor_id, double shock) override;

    /**
     * @brief Loads a scenario set as scenario-name to factor-shock maps.
     */
    std::map<std::string, std::map<std::string, double>> load_scenario_set(const std::string& set_id) override;

    /**
     * @brief Lists stored portfolio ids.
     */
    std::vector<std::string> list_portfolios() override;

    /**
     * @brief Lists stored market snapshot ids.
     */
    std::vector<std::string> list_snapshots() override;

    /**
     * @brief Lists analysis run ids for a portfolio.
     */
    std::vector<std::string> list_runs(const std::string& portfolio_id) override;

    /**
     * @brief Fetches all valuation results for a given run.
     */
    std::vector<ValuationRecord> get_valuation_results(const std::string& run_id) override;

private:
    /**
     * @brief Executes SQL that does not return rows and throws on failure.
     */
    void execute_sql(const std::string& sql);
    
    sqlite3* db_ = nullptr;
    std::string db_path_;
};

} // namespace qrp::persistence
