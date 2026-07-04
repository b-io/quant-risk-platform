// Implements SQLite persistence for valuation, risk, scenario, and VaR results.

#include <qrp/persistence/sqlite_storage_backend.hpp>

#include <fmt/format.h>

#include <stdexcept>
#include <string>

// Stores and retrieves analytics result sets with stable run identifiers.

namespace qrp::persistence {
namespace {

/**
 * @brief RAII wrapper around sqlite3_stmt for result persistence queries.
 */
class Statement {
public:
    /**
     * @brief Prepares a SQL statement and throws on preparation failure.
     */
    Statement(sqlite3* db, const char* sql)
        : db_(db) {
        if (sqlite3_prepare_v2(db_, sql, -1, &stmt_, nullptr) != SQLITE_OK) {
            throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
        }
    }

    /**
     * @brief Finalizes the prepared statement.
     */
    ~Statement() {
        sqlite3_finalize(stmt_);
    }

    /**
     * @brief Statement wrappers own sqlite3_stmt and are not copyable.
     */
    Statement(const Statement&) = delete;

    /**
     * @brief Statement wrappers own sqlite3_stmt and are not assignable.
     */
    Statement& operator=(const Statement&) = delete;

    /**
     * @brief Binds a double parameter by 1-based SQLite index.
     */
    void bind_double(int index, double value) {
        sqlite3_bind_double(stmt_, index, value);
    }

    /**
     * @brief Binds an integer parameter by 1-based SQLite index.
     */
    void bind_int(int index, int value) {
        sqlite3_bind_int(stmt_, index, value);
    }

    /**
     * @brief Binds a text parameter by 1-based SQLite index.
     */
    void bind_text(int index, const std::string& value) {
        sqlite3_bind_text(stmt_, index, value.c_str(), -1, SQLITE_TRANSIENT);
    }

    /**
     * @brief Reads a text column from the current row.
     */
    std::string column_text(int index) const {
        const auto* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt_, index));
        return text ? text : "";
    }

    /**
     * @brief Reads a double column from the current row.
     */
    double column_double(int index) const {
        return sqlite3_column_double(stmt_, index);
    }

    /**
     * @brief Steps to the next row and returns false at the end of the result set.
     */
    bool step_row() {
        const int rc = sqlite3_step(stmt_);
        if (rc == SQLITE_ROW) return true;
        if (rc == SQLITE_DONE) return false;
        throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
    }

    /**
     * @brief Executes a non-row statement and requires SQLITE_DONE.
     */
    void step_done() {
        const int rc = sqlite3_step(stmt_);
        if (rc != SQLITE_DONE) {
            throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
        }
    }

private:
    sqlite3* db_;
    sqlite3_stmt* stmt_ = nullptr;
};

} // namespace

void SQLiteStorageBackend::store_analysis_run(const std::string& run_id, const std::string& type, const std::string& portfolio_id, const std::string& snapshot_id) {
    const char* sql = "INSERT OR REPLACE INTO analysis_runs (run_id, run_type, portfolio_id, snapshot_id, status) VALUES (?, ?, ?, ?, 'COMPLETED');";
    Statement stmt(db_, sql);
    stmt.bind_text(1, run_id);
    stmt.bind_text(2, type);
    stmt.bind_text(3, portfolio_id);
    stmt.bind_text(4, snapshot_id);
    stmt.step_done();
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
    Statement stmt(db_, sql);
    stmt.bind_text(1, run_id);
    stmt.bind_text(2, trade_id);
    stmt.bind_double(3, npv);
    stmt.bind_text(4, ccy);
    stmt.bind_text(5, status);
    stmt.bind_text(6, asset_class);
    stmt.bind_text(7, product_type);
    stmt.bind_text(8, support_status);
    stmt.bind_text(9, model_name);
    stmt.bind_text(10, status_message);
    stmt.bind_text(11, error);
    stmt.step_done();
}

void SQLiteStorageBackend::store_scenario_result(const std::string& run_id, const std::string& scenario_name, double portfolio_pnl) {
    const char* sql = "INSERT OR REPLACE INTO scenario_results (run_id, scenario_name, portfolio_pnl) VALUES (?, ?, ?);";
    Statement stmt(db_, sql);
    stmt.bind_text(1, run_id);
    stmt.bind_text(2, scenario_name);
    stmt.bind_double(3, portfolio_pnl);
    stmt.step_done();
}

void SQLiteStorageBackend::store_var_result(const std::string& run_id, const std::string& method, double confidence_level, double var_value, double expected_shortfall, int scenario_count) {
    const char* sql = "INSERT OR REPLACE INTO var_results (run_id, method, confidence_level, var_value, expected_shortfall, scenario_count) VALUES (?, ?, ?, ?, ?, ?);";
    Statement stmt(db_, sql);
    stmt.bind_text(1, run_id);
    stmt.bind_text(2, method);
    stmt.bind_double(3, confidence_level);
    stmt.bind_double(4, var_value);
    stmt.bind_double(5, expected_shortfall);
    stmt.bind_int(6, scenario_count);
    stmt.step_done();
}

void SQLiteStorageBackend::store_risk_result(const std::string& run_id, const std::string& trade_id, const std::string& measure, const std::string& rf_id, double value) {
    const char* sql = "INSERT OR REPLACE INTO risk_results (run_id, trade_id, risk_measure, risk_factor_id, value) VALUES (?, ?, ?, ?, ?);";
    Statement stmt(db_, sql);
    stmt.bind_text(1, run_id);
    stmt.bind_text(2, trade_id);
    stmt.bind_text(3, measure);
    stmt.bind_text(4, rf_id);
    stmt.bind_double(5, value);
    stmt.step_done();
}

void SQLiteStorageBackend::store_scenario_set(const std::string& set_id, const std::string& name) {
    const char* sql = "INSERT OR REPLACE INTO scenario_sets (scenario_set_id, name) VALUES (?, ?);";
    Statement stmt(db_, sql);
    stmt.bind_text(1, set_id);
    stmt.bind_text(2, name);
    stmt.step_done();
}

void SQLiteStorageBackend::store_scenario_factor_shock(const std::string& set_id, const std::string& scenario_name, const std::string& factor_id, double shock) {
    const char* sql = "INSERT OR REPLACE INTO scenario_factor_shocks (scenario_set_id, scenario_name, factor_id, shock) VALUES (?, ?, ?, ?);";
    Statement stmt(db_, sql);
    stmt.bind_text(1, set_id);
    stmt.bind_text(2, scenario_name);
    stmt.bind_text(3, factor_id);
    stmt.bind_double(4, shock);
    stmt.step_done();
}

std::map<std::string, std::map<std::string, double>> SQLiteStorageBackend::load_scenario_set(const std::string& set_id) {
    std::map<std::string, std::map<std::string, double>> set;
    const char* sql = "SELECT scenario_name, factor_id, shock FROM scenario_factor_shocks WHERE scenario_set_id = ?;";
    Statement stmt(db_, sql);
    stmt.bind_text(1, set_id);
    while (stmt.step_row()) {
        std::string sc_name = stmt.column_text(0);
        std::string factor_id = stmt.column_text(1);
        double shock = stmt.column_double(2);
        set[sc_name][factor_id] = shock;
    }
    return set;
}

std::vector<std::string> SQLiteStorageBackend::list_portfolios() {
    std::vector<std::string> ids;
    const char* sql = "SELECT portfolio_id FROM portfolios;";
    Statement stmt(db_, sql);
    while (stmt.step_row()) {
        ids.push_back(stmt.column_text(0));
    }
    return ids;
}

std::vector<std::string> SQLiteStorageBackend::list_snapshots() {
    std::vector<std::string> ids;
    const char* sql = "SELECT snapshot_id FROM market_snapshots;";
    Statement stmt(db_, sql);
    while (stmt.step_row()) {
        ids.push_back(stmt.column_text(0));
    }
    return ids;
}

std::vector<std::string> SQLiteStorageBackend::list_runs(const std::string& portfolio_id) {
    std::vector<std::string> ids;
    const char* sql = "SELECT run_id FROM analysis_runs WHERE portfolio_id = ?;";
    Statement stmt(db_, sql);
    stmt.bind_text(1, portfolio_id);
    while (stmt.step_row()) {
        ids.push_back(stmt.column_text(0));
    }
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
    Statement stmt(db_, sql);
    stmt.bind_text(1, run_id);

    while (stmt.step_row()) {
        ValuationRecord r;
        r.run_id = stmt.column_text(0);
        r.trade_id = stmt.column_text(1);
        r.npv = stmt.column_double(2);
        r.ccy = stmt.column_text(3);
        r.status = stmt.column_text(4);
        r.asset_class = stmt.column_text(5);
        r.product_type = stmt.column_text(6);
        r.support_status = stmt.column_text(7);
        r.model_name = stmt.column_text(8);
        r.status_message = stmt.column_text(9);
        r.error_message = stmt.column_text(10);
        results.push_back(r);
    }
    return results;
}

} // namespace qrp::persistence

