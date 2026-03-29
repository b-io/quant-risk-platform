#include <qrp/persistence/sqlite_storage_backend.hpp>
#include <fmt/format.h>
#include <iostream>

namespace qrp::persistence {

SQLiteStorageBackend::SQLiteStorageBackend(const std::string& db_path)
    : db_path_(db_path) {
    int rc = sqlite3_open(db_path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::string err_msg = sqlite3_errmsg(db_);
        sqlite3_close(db_);
        throw std::runtime_error(fmt::format("Cannot open database: {} - {}", db_path, err_msg));
    }
}

SQLiteStorageBackend::~SQLiteStorageBackend() {
    if (db_) {
        sqlite3_close(db_);
    }
}

void SQLiteStorageBackend::execute_sql(const std::string& sql) {
    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        std::string msg = err_msg;
        sqlite3_free(err_msg);
        throw std::runtime_error(fmt::format("SQL error: {}", msg));
    }
}

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
            product_type TEXT,
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
            as_of_date TEXT,
            scenario_name TEXT,
            base_currency TEXT,
            curves_json TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )");

    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS market_quotes (
            snapshot_id TEXT,
            quote_id TEXT,
            risk_factor_id TEXT,
            value REAL,
            currency TEXT,
            tenor TEXT,
            instrument_type TEXT,
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
        CREATE TABLE IF NOT EXISTS scenario_sets (
            scenario_set_id TEXT PRIMARY KEY,
            name TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
    )");

    execute_sql(R"(
        CREATE TABLE IF NOT EXISTS scenario_quotes (
            scenario_set_id TEXT,
            scenario_name TEXT,
            quote_id TEXT,
            shock REAL,
            PRIMARY KEY(scenario_set_id, scenario_name, quote_id),
            FOREIGN KEY(scenario_set_id) REFERENCES scenario_sets(scenario_set_id)
        );
    )");
}

void SQLiteStorageBackend::store_portfolio(const std::string& portfolio_id, const std::string& name, const std::string& base_ccy) {
    const char* sql = "INSERT OR REPLACE INTO portfolios (portfolio_id, portfolio_name, base_currency) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, portfolio_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, base_ccy.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_finalize(stmt);
}

void SQLiteStorageBackend::store_book(const std::string& book_id, const std::string& portfolio_id, const std::string& name) {
    const char* sql = "INSERT OR REPLACE INTO books (book_id, portfolio_id, book_name) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, book_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, portfolio_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, name.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_finalize(stmt);
}

void SQLiteStorageBackend::store_trade(const std::string& trade_id, const std::string& portfolio_id, const std::string& book_id,
                                       const std::string& asset_class, const std::string& product_type, const std::string& ccy,
                                       double notional, const std::string& start_date, const std::string& maturity_date,
                                       const std::string& direction, const std::string& economics_json) {
    const char* sql = "INSERT OR REPLACE INTO trades (trade_id, portfolio_id, book_id, asset_class, product_type, currency, notional, start_date, maturity_date, direction, economics_json) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, trade_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, portfolio_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, book_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, asset_class.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, product_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, ccy.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 7, notional);
    sqlite3_bind_text(stmt, 8, start_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, maturity_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, direction.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, economics_json.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_finalize(stmt);
}

void SQLiteStorageBackend::store_market_snapshot(const std::string& snapshot_id, const std::string& as_of_date, const std::string& base_ccy, const std::string& curves_json) {
    const char* sql = "INSERT OR REPLACE INTO market_snapshots (snapshot_id, as_of_date, base_currency, curves_json) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, snapshot_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, as_of_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, base_ccy.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, curves_json.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_finalize(stmt);
}

void SQLiteStorageBackend::store_market_quote(const std::string& snapshot_id, const std::string& quote_id, double value, const std::string& ccy, const std::string& metadata_json) {
    const char* sql = "INSERT OR REPLACE INTO market_quotes (snapshot_id, quote_id, value, currency, metadata_json) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, snapshot_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, quote_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, value);
    sqlite3_bind_text(stmt, 4, ccy.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, metadata_json.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_finalize(stmt);
}

std::vector<domain::Trade> SQLiteStorageBackend::load_trades(const std::string& portfolio_id) {
    std::vector<domain::Trade> trades;
    const char* sql = "SELECT trade_id, asset_class, product_type, currency, notional, start_date, maturity_date, direction, economics_json FROM trades WHERE portfolio_id = ?;";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, portfolio_id.c_str(), -1, SQLITE_TRANSIENT);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        domain::Trade t;
        t.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        t.asset_class = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        t.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        t.currency = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        t.notional = sqlite3_column_double(stmt, 4);
        t.start_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        t.maturity_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        t.direction = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        const char* econ_json_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        if (econ_json_ptr) {
            t.details = nlohmann::json::parse(econ_json_ptr);
        }
        trades.push_back(t);
    }
    sqlite3_finalize(stmt);
    return trades;
}

domain::MarketSnapshot SQLiteStorageBackend::load_market_snapshot(const std::string& snapshot_id) {
    domain::MarketSnapshot snapshot;
    const char* sql = "SELECT as_of_date, curves_json FROM market_snapshots WHERE snapshot_id = ?;";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, snapshot_id.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        snapshot.valuation_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        const char* curves_json_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        if (curves_json_ptr) {
            snapshot.curves = nlohmann::json::parse(curves_json_ptr).get<std::vector<domain::CurveSpec>>();
        }
    } else {
        sqlite3_finalize(stmt);
        throw std::runtime_error("Snapshot not found: " + snapshot_id);
    }
    sqlite3_finalize(stmt);
    
    // Load quotes
    sql = "SELECT quote_id, value, currency, metadata_json FROM market_quotes WHERE snapshot_id = ?;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, snapshot_id.c_str(), -1, SQLITE_TRANSIENT);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        domain::MarketQuote q;
        q.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        q.value = sqlite3_column_double(stmt, 1);
        const char* ccy_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        if (ccy_ptr) q.currency = domain::from_string(ccy_ptr);
        
        const char* metadata_json_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        if (metadata_json_ptr) {
            nlohmann::json j = nlohmann::json::parse(metadata_json_ptr);
            // Restore fields from metadata
            if (j.contains("tenor")) q.tenor = j["tenor"];
            if (j.contains("instrument_type")) q.instrument_type = j["instrument_type"];
            if (j.contains("instrument_family")) q.instrument_family = j["instrument_family"];
            if (j.contains("index_family")) q.index_family = j["index_family"];
            if (j.contains("day_count")) q.day_count = j["day_count"];
            if (j.contains("calendar")) q.calendar = j["calendar"];
            if (j.contains("bdc")) q.bdc = j["bdc"];
            if (j.contains("settlement_days")) q.settlement_days = j["settlement_days"];
        }
        snapshot.quotes.push_back(q);
    }
    sqlite3_finalize(stmt);
    
    return snapshot;
}

void SQLiteStorageBackend::store_analysis_run(const std::string& run_id, const std::string& type, const std::string& portfolio_id, const std::string& snapshot_id) {
    const char* sql = "INSERT OR REPLACE INTO analysis_runs (run_id, run_type, portfolio_id, snapshot_id, status) VALUES (?, ?, ?, ?, 'COMPLETED');";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, run_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, portfolio_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, snapshot_id.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_finalize(stmt);
}

void SQLiteStorageBackend::store_valuation_result(const std::string& run_id, const std::string& trade_id, double npv, const std::string& ccy, const std::string& status, const std::string& error) {
    const char* sql = "INSERT OR REPLACE INTO valuation_results (run_id, trade_id, npv_base, valuation_ccy, status, error_message) VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, run_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, trade_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, npv);
    sqlite3_bind_text(stmt, 4, ccy.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, error.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_finalize(stmt);
}

void SQLiteStorageBackend::store_risk_result(const std::string& run_id, const std::string& trade_id, const std::string& measure, const std::string& rf_id, double value) {
    const char* sql = "INSERT OR REPLACE INTO risk_results (run_id, trade_id, risk_measure, risk_factor_id, value) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, run_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, trade_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, measure.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, rf_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 5, value);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_finalize(stmt);
}

void SQLiteStorageBackend::store_scenario_set(const std::string& set_id, const std::string& name) {
    const char* sql = "INSERT OR REPLACE INTO scenario_sets (scenario_set_id, name) VALUES (?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, set_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, name.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_finalize(stmt);
}

void SQLiteStorageBackend::store_scenario_quote_shock(const std::string& set_id, const std::string& scenario_name, const std::string& quote_id, double shock) {
    const char* sql = "INSERT OR REPLACE INTO scenario_quotes (scenario_set_id, scenario_name, quote_id, shock) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, set_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, scenario_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, quote_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, shock);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_finalize(stmt);
}

std::map<std::string, std::map<std::string, double>> SQLiteStorageBackend::load_scenario_set(const std::string& set_id) {
    std::map<std::string, std::map<std::string, double>> set;
    std::string sql = fmt::format("SELECT scenario_name, quote_id, shock FROM scenario_quotes WHERE scenario_set_id = '{}';", set_id);
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string sc_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string q_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        double shock = sqlite3_column_double(stmt, 2);
        set[sc_name][q_id] = shock;
    }
    sqlite3_finalize(stmt);
    return set;
}

std::vector<std::string> SQLiteStorageBackend::list_portfolios() {
    std::vector<std::string> ids;
    const char* sql = "SELECT portfolio_id FROM portfolios;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ids.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }
    sqlite3_finalize(stmt);
    return ids;
}

std::vector<std::string> SQLiteStorageBackend::list_snapshots() {
    std::vector<std::string> ids;
    const char* sql = "SELECT snapshot_id FROM market_snapshots;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ids.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }
    sqlite3_finalize(stmt);
    return ids;
}

std::vector<std::string> SQLiteStorageBackend::list_runs(const std::string& portfolio_id) {
    std::vector<std::string> ids;
    const char* sql = "SELECT run_id FROM analysis_runs WHERE portfolio_id = ?;";
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, portfolio_id.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ids.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }
    sqlite3_finalize(stmt);
    return ids;
}

std::vector<StorageBackend::ValuationRecord> SQLiteStorageBackend::get_valuation_results(const std::string& run_id) {
    std::vector<ValuationRecord> results;
    const char* sql = "SELECT run_id, trade_id, npv_base, valuation_ccy, status FROM valuation_results WHERE run_id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
         throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, run_id.c_str(), -1, SQLITE_TRANSIENT);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ValuationRecord r;
        r.run_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        r.trade_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        r.npv = sqlite3_column_double(stmt, 2);
        r.ccy = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        r.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        results.push_back(r);
    }
    sqlite3_finalize(stmt);
    return results;
}

} // namespace qrp::persistence
