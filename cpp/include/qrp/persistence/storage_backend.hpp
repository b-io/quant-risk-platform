#pragma once

// Declares the storage abstraction used by application services and concrete persistence backends.

#include <qrp/domain/factors.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/market_event.hpp>
#include <qrp/domain/portfolio.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

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
    /**
     * @brief Allows concrete storage backends to be destroyed through the base type.
     */
    virtual ~StorageBackend() = default;

    /**
     * @brief Creates or migrates all required storage schema objects.
     */
    virtual void initialize_schema() = 0;

    // Static / reference-like entities
    /**
     * @brief Stores a portfolio header.
     */
    virtual void store_portfolio(const std::string& portfolio_id, const std::string& name, const std::string& base_ccy) = 0;

    /**
     * @brief Stores a book header associated with a portfolio.
     */
    virtual void store_book(const std::string& book_id, const std::string& portfolio_id, const std::string& name) = 0;

    /**
     * @brief Stores a normalized trade row and product-specific economics JSON.
     */
    virtual void store_trade(const std::string& trade_id, const std::string& portfolio_id, const std::string& book_id,
                             const std::string& asset_class, const std::string& trade_type, const std::string& ccy,
                             double notional, const std::string& start_date, const std::string& maturity_date,
                             const std::string& direction, const std::string& economics_json) = 0;

    // Market snapshots and bitemporal events
    /**
     * @brief Stores a market snapshot header and curve specification payload.
     */
    virtual void store_market_snapshot(const std::string& snapshot_id, const std::string& as_of_date, const std::string& base_ccy, const std::string& curves_json = "[]") = 0;

    /**
     * @brief Stores a raw quote attached to a market snapshot.
     */
    virtual void store_market_quote(const std::string& snapshot_id, const std::string& quote_id, double value, const std::string& ccy, const std::string& metadata_json = "{}") = 0;

    /**
     * @brief Stores one bitemporal market quote event.
     */
    virtual void store_market_quote_event(const domain::MarketQuoteEvent& event) = 0;

    /**
     * @brief Reconstructs a market snapshot as of market and knowledge timestamps.
     */
    virtual domain::MarketSnapshot load_market_snapshot_asof(
        const std::string& market_ts,
        const std::string& recorded_ts,
        const std::string& base_ccy,
        const std::string& overlay_set_id = "") = 0;

    // Factors and History
    /**
     * @brief Stores a risk-factor definition.
     */
    virtual void store_factor_definition(const domain::FactorDefinition& factor) = 0;

    /**
     * @brief Stores one historical factor observation.
     */
    virtual void store_factor_observation(const domain::FactorObservation& obs) = 0;

    /**
     * @brief Stores one factor-to-quote binding.
     */
    virtual void store_factor_binding(const domain::FactorBinding& binding) = 0;

    /**
     * @brief Loads factor definitions relevant to a portfolio.
     */
    virtual std::vector<domain::FactorDefinition> load_factor_definitions(const std::string& portfolio_id) = 0;

    /**
     * @brief Loads quote bindings for the requested factor ids.
     */
    virtual std::vector<domain::FactorBinding> load_factor_bindings(const std::vector<std::string>& factor_ids) = 0;

    /**
     * @brief Loads historical observations for requested factors over a date range.
     */
    virtual std::vector<domain::FactorObservation> load_factor_history(
        const std::vector<std::string>& factor_ids,
        const std::string& start_date,
        const std::string& end_date) = 0;

    // Load methods
    /**
     * @brief Loads all trades in a portfolio as canonical trade DTOs.
     */
    virtual std::vector<std::shared_ptr<domain::Trade>> load_trades(const std::string& portfolio_id) = 0;

    /**
     * @brief Loads a complete market snapshot DTO by id.
     */
    virtual domain::MarketSnapshot load_market_snapshot(const std::string& snapshot_id) = 0;

    // Run persistence
    /**
     * @brief Stores an analysis run header.
     */
    virtual void store_analysis_run(const std::string& run_id, const std::string& type, const std::string& portfolio_id, const std::string& snapshot_id) = 0;

    /**
     * @brief Stores aggregate PnL for one scenario in a run.
     */
    virtual void store_scenario_result(const std::string& run_id, const std::string& scenario_name, double portfolio_pnl) = 0;

    /**
     * @brief Stores normalized valuation output for one trade in a run.
     */
    virtual void store_valuation_result(
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
        const std::string& status_message) = 0;

    /**
     * @brief Stores aggregate VaR and ES metrics for a run.
     */
    virtual void store_var_result(const std::string& run_id, const std::string& method, double confidence_level, double var_value, double expected_shortfall, int scenario_count) = 0;

    /**
     * @brief Stores one trade-level risk measure.
     */
    virtual void store_risk_result(const std::string& run_id, const std::string& trade_id, const std::string& measure, const std::string& rf_id, double value) = 0;

    // Scenarios
    /**
     * @brief Stores a scenario set header.
     */
    virtual void store_scenario_set(const std::string& set_id, const std::string& name) = 0;

    /**
     * @brief Stores one factor shock within a named scenario.
     */
    virtual void store_scenario_factor_shock(const std::string& set_id, const std::string& scenario_name, const std::string& factor_id, double shock) = 0;

    /**
     * @brief Loads a scenario set as scenario-name to factor-shock maps.
     */
    virtual std::map<std::string, std::map<std::string, double>> load_scenario_set(const std::string& set_id) = 0;

    // Ad hoc queries
    /**
     * @brief Lists stored portfolio ids.
     */
    virtual std::vector<std::string> list_portfolios() = 0;

    /**
     * @brief Lists stored market snapshot ids.
     */
    virtual std::vector<std::string> list_snapshots() = 0;

    /**
     * @brief Lists analysis run ids for a portfolio.
     */
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
        std::string asset_class;
        std::string product_type;
        std::string support_status;
        std::string model_name;
        std::string status_message;
        std::string error_message;
    };

    /**
     * @brief Fetches all valuation results for a given run.
     */
    virtual std::vector<ValuationRecord> get_valuation_results(const std::string& run_id) = 0;
};

} // namespace qrp::persistence
