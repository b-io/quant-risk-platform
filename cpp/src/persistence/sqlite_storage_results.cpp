#include <qrp/persistence/sqlite_storage_backend.hpp>
#include <fmt/format.h>

namespace qrp::persistence {

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

void SQLiteStorageBackend::store_valuation_result(
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
    const std::string& status_message) {
    const char* sql = R"(
        INSERT OR REPLACE INTO valuation_results (
            run_id,
            trade_id,
            npv_base,
            valuation_ccy,
            status,
            asset_class,
            product_type,
            support_status,
            model_name,
            status_message,
            error_message
        )
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, run_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, trade_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, npv);
    sqlite3_bind_text(stmt, 4, ccy.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, asset_class.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, product_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, support_status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, model_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, status_message.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, error.c_str(), -1, SQLITE_TRANSIENT);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_finalize(stmt);
}

void SQLiteStorageBackend::store_scenario_result(const std::string& run_id, const std::string& scenario_name, double portfolio_pnl) {
    const char* sql = "INSERT OR REPLACE INTO scenario_results (run_id, scenario_name, portfolio_pnl) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, run_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, scenario_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, portfolio_pnl);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_finalize(stmt);
}

void SQLiteStorageBackend::store_var_result(const std::string& run_id, const std::string& method, double confidence_level, double var_value, double expected_shortfall, int scenario_count) {
    const char* sql = "INSERT OR REPLACE INTO var_results (run_id, method, confidence_level, var_value, expected_shortfall, scenario_count) VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, run_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, method.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, confidence_level);
    sqlite3_bind_double(stmt, 4, var_value);
    sqlite3_bind_double(stmt, 5, expected_shortfall);
    sqlite3_bind_int(stmt, 6, scenario_count);

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

void SQLiteStorageBackend::store_scenario_factor_shock(const std::string& set_id, const std::string& scenario_name, const std::string& factor_id, double shock) {
    const char* sql = "INSERT OR REPLACE INTO scenario_factor_shocks (scenario_set_id, scenario_name, factor_id, shock) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, set_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, scenario_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, factor_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, shock);
    
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_finalize(stmt);
}

std::map<std::string, std::map<std::string, double>> SQLiteStorageBackend::load_scenario_set(const std::string& set_id) {
    std::map<std::string, std::map<std::string, double>> set;
    std::string sql = fmt::format("SELECT scenario_name, factor_id, shock FROM scenario_factor_shocks WHERE scenario_set_id = '{}';", set_id);
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string sc_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string factor_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        double shock = sqlite3_column_double(stmt, 2);
        set[sc_name][factor_id] = shock;
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
    const char* sql = R"(
        SELECT
            run_id,
            trade_id,
            npv_base,
            valuation_ccy,
            status,
            asset_class,
            product_type,
            support_status,
            model_name,
            status_message,
            error_message
        FROM valuation_results
        WHERE run_id = ?;
    )";
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
        r.asset_class = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        r.product_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        r.support_status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        r.model_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        r.status_message = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        const char* error_message = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        if (error_message) r.error_message = error_message;
        results.push_back(r);
    }
    sqlite3_finalize(stmt);
    return results;
}

} // namespace qrp::persistence

