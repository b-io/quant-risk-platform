#include <qrp/persistence/sqlite_storage_backend.hpp>
#include <fmt/format.h>

namespace qrp::persistence {

/**
 * @brief Creates the SQLite schema for reference data, market data, scenarios, and results.
 */
void SQLiteStorageBackend::initialize_schema() {
    // 1. Portfolios and Trades
    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS portfolios (
            portfolio_id TEXT PRIMARY KEY,
            portfolio_name TEXT,
            base_currency TEXT,
            as_of_date TEXT,
            status TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )");

    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS books (
            book_id TEXT PRIMARY KEY,
            portfolio_id TEXT,
            book_name TEXT,
            desk TEXT,
            legal_entity TEXT,
            base_currency TEXT,
            FOREIGN KEY(portfolio_id) REFERENCES portfolios(portfolio_id)
        );
    )");

    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS trades (
            trade_id TEXT PRIMARY KEY,
            portfolio_id TEXT,
            book_id TEXT,
            asset_class TEXT,
            trade_type TEXT,
            currency TEXT,
            notional REAL,
            start_date TEXT,
            maturity_date TEXT,
            direction TEXT,
            economics_json TEXT,
            tags_json TEXT,
            FOREIGN KEY(portfolio_id) REFERENCES portfolios(portfolio_id),
            FOREIGN KEY(book_id) REFERENCES books(book_id)
        );
    )");

    // 2. Market Data
    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS market_snapshots (
            snapshot_id TEXT PRIMARY KEY,
            schema_version INTEGER NOT NULL DEFAULT 2,
            as_of_date TEXT NOT NULL,
            base_currency TEXT,
            source_name TEXT,
            recorded_ts TEXT,
            default_stale_after_days INTEGER DEFAULT -1,
            scenario_name TEXT,
            curves_json TEXT,
            diagnostics_json TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )");

    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS market_quotes (
            snapshot_id TEXT,
            quote_id TEXT,
            risk_factor_id TEXT,
            quote_type TEXT,
            underlier TEXT,
            expiry TEXT,
            strike TEXT,
            value REAL,
            currency TEXT,
            tenor TEXT,
            instrument_type TEXT,
            market_ts TEXT,
            recorded_ts TEXT,
            source_name TEXT,
            source_ts TEXT,
            stale_after_days INTEGER DEFAULT -1,
            metadata_json TEXT,
            PRIMARY KEY(snapshot_id, quote_id),
            FOREIGN KEY(snapshot_id) REFERENCES market_snapshots(snapshot_id)
        );
    )");

    // 3. Analysis Runs and Results
    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS analysis_runs (
            run_id TEXT PRIMARY KEY,
            run_type TEXT,
            portfolio_id TEXT,
            snapshot_id TEXT,
            status TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )");

    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS valuation_results (
            run_id TEXT,
            trade_id TEXT,
            npv_base REAL,
            valuation_ccy TEXT,
            status TEXT,
            asset_class TEXT,
            product_type TEXT,
            support_status TEXT,
            model_name TEXT,
            status_message TEXT,
            error_message TEXT,
            PRIMARY KEY(run_id, trade_id),
            FOREIGN KEY(run_id) REFERENCES analysis_runs(run_id)
        );
    )");

    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS risk_results (
            run_id TEXT,
            trade_id TEXT,
            risk_measure TEXT,
            risk_factor_id TEXT,
            value REAL,
            PRIMARY KEY(run_id, trade_id, risk_measure, risk_factor_id),
            FOREIGN KEY(run_id) REFERENCES analysis_runs(run_id)
        );
    )");

    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS factor_definitions (
            factor_id TEXT PRIMARY KEY,
            factor_type TEXT,
            shock_measure TEXT,
            currency TEXT,
            curve_id TEXT,
            tenor TEXT,
            quote_ids_json TEXT,
            description TEXT
        );
    )");

    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS factor_observations (
            factor_id TEXT,
            market_date TEXT,
            level REAL,
            move REAL,
            move_unit TEXT,
            PRIMARY KEY(factor_id, market_date),
            FOREIGN KEY(factor_id) REFERENCES factor_definitions(factor_id)
        );
    )");

    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS market_quote_events (
            event_id TEXT PRIMARY KEY,
            quote_id TEXT,
            factor_id TEXT,
            market_ts TEXT,
            recorded_ts TEXT,
            source_name TEXT,
            source_ts TEXT,
            value REAL,
            currency TEXT,
            tenor TEXT,
            instrument_type TEXT,
            overlay_set_id TEXT,
            metadata_json TEXT
        );
    )");

    execute_sql(R"(
        CREATE INDEX IF NOT EXISTS idx_quote_events_qid_market_recorded
        ON market_quote_events(quote_id, market_ts, recorded_ts);
    )");

    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS factor_quote_bindings (
            factor_id TEXT NOT NULL,
            quote_id TEXT NOT NULL,
            shock_measure TEXT NOT NULL,
            weight REAL NOT NULL DEFAULT 1.0,
            transform TEXT,
            selector_json TEXT,
            PRIMARY KEY (factor_id, quote_id),
            FOREIGN KEY (factor_id) REFERENCES factor_definitions(factor_id)
        );
    )");

    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS scenario_sets (
            scenario_set_id TEXT PRIMARY KEY,
            name TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )");

    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS scenario_factor_shocks (
            scenario_set_id TEXT,
            scenario_name TEXT,
            factor_id TEXT,
            shock REAL,
            PRIMARY KEY(scenario_set_id, scenario_name, factor_id),
            FOREIGN KEY(scenario_set_id) REFERENCES scenario_sets(scenario_set_id),
            FOREIGN KEY(factor_id) REFERENCES factor_definitions(factor_id)
        );
    )");

    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS scenario_results (
            run_id TEXT,
            scenario_name TEXT,
            portfolio_pnl REAL,
            PRIMARY KEY(run_id, scenario_name),
            FOREIGN KEY(run_id) REFERENCES analysis_runs(run_id)
        );
    )");

    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS var_results (
            run_id TEXT,
            method TEXT,
            confidence_level REAL,
            var_value REAL,
            expected_shortfall REAL,
            scenario_count INTEGER,
            PRIMARY KEY(run_id, method, confidence_level),
            FOREIGN KEY(run_id) REFERENCES analysis_runs(run_id)
        );
    )");
}

} // namespace qrp::persistence

