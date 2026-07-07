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
    Statement(sqlite3* db, const char* sql);

    /**
     * @brief Finalizes the prepared statement.
     */
    ~Statement();

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
    void bind_double(int index, double value);

    /**
     * @brief Binds an integer parameter by 1-based SQLite index.
     */
    void bind_int(int index, int value);

    /**
     * @brief Binds a text parameter by 1-based SQLite index.
     */
    void bind_text(int index, const std::string& value);

    /**
     * @brief Reads a text column from the current row.
     */
    std::string column_text(int index) const;

    /**
     * @brief Reads a double column from the current row.
     */
    double column_double(int index) const;

    /**
     * @brief Steps to the next row and returns false at the end of the result set.
     */
    bool step_row();

    /**
     * @brief Executes a non-row statement and requires SQLITE_DONE.
     */
    void step_done();

private:
    sqlite3* db_;
    sqlite3_stmt* stmt_ = nullptr;
};

Statement::Statement(sqlite3* db, const char* sql) : db_(db) {
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt_, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
}

Statement::~Statement() {
    sqlite3_finalize(stmt_);
}

void Statement::bind_double(int index, double value) {
    sqlite3_bind_double(stmt_, index, value);
}

void Statement::bind_int(int index, int value) {
    sqlite3_bind_int(stmt_, index, value);
}

void Statement::bind_text(int index, const std::string& value) {
    sqlite3_bind_text(stmt_, index, value.c_str(), -1, SQLITE_TRANSIENT);
}

std::string Statement::column_text(int index) const {
    const auto* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt_, index));
    return text ? text : "";
}

double Statement::column_double(int index) const {
    return sqlite3_column_double(stmt_, index);
}

bool Statement::step_row() {
    const int rc = sqlite3_step(stmt_);
    if (rc == SQLITE_ROW)
        return true;
    if (rc == SQLITE_DONE)
        return false;
    throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
}

void Statement::step_done() {
    const int rc = sqlite3_step(stmt_);
    if (rc != SQLITE_DONE) {
        throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
    }
}

} // namespace

void SQLiteStorageBackend::store_analysis_run(const std::string& run_id,
                                              const std::string& type,
                                              const std::string& portfolio_id,
                                              const std::string& snapshot_id) {
    const char* sql = "INSERT OR REPLACE INTO analysis_runs (run_id, run_type, portfolio_id, "
                      "snapshot_id, status) VALUES (?, ?, ?, ?, 'COMPLETED');";
    Statement stmt(db_, sql);
    stmt.bind_text(1, run_id);
    stmt.bind_text(2, type);
    stmt.bind_text(3, portfolio_id);
    stmt.bind_text(4, snapshot_id);
    stmt.step_done();
}

void SQLiteStorageBackend::store_valuation_result(const std::string& run_id,
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

void SQLiteStorageBackend::store_scenario_result(const std::string& run_id,
                                                 const std::string& scenario_name,
                                                 double portfolio_pnl) {
    const char* sql = "INSERT OR REPLACE INTO scenario_results (run_id, scenario_name, "
                      "portfolio_pnl) VALUES (?, ?, ?);";
    Statement stmt(db_, sql);
    stmt.bind_text(1, run_id);
    stmt.bind_text(2, scenario_name);
    stmt.bind_double(3, portfolio_pnl);
    stmt.step_done();
}

void SQLiteStorageBackend::store_pnl_explain_result(const PnlExplainRecord& record) {
    const char* sql = R"(
        INSERT OR REPLACE INTO pnl_explain_results (
            run_id,
            trade_id,
            asset_class,
            book,
            currency,
            product_type,
            strategy,
            prev_npv,
            curr_npv,
            total_pnl,
            carry_pnl,
            roll_down_pnl,
            market_move_pnl,
            cash_pnl,
            trade_activity_pnl,
            fx_translation_pnl,
            model_change_pnl,
            residual_pnl,
            explained_pnl,
            reconciliation_difference,
            reconciliation_passed,
            support_status,
            model_name,
            status_message
        )
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";
    Statement stmt(db_, sql);
    stmt.bind_text(1, record.run_id);
    stmt.bind_text(2, record.trade_id);
    stmt.bind_text(3, record.asset_class);
    stmt.bind_text(4, record.book);
    stmt.bind_text(5, record.currency);
    stmt.bind_text(6, record.product_type);
    stmt.bind_text(7, record.strategy);
    stmt.bind_double(8, record.prev_npv);
    stmt.bind_double(9, record.curr_npv);
    stmt.bind_double(10, record.total_pnl);
    stmt.bind_double(11, record.carry_pnl);
    stmt.bind_double(12, record.roll_down_pnl);
    stmt.bind_double(13, record.market_move_pnl);
    stmt.bind_double(14, record.cash_pnl);
    stmt.bind_double(15, record.trade_activity_pnl);
    stmt.bind_double(16, record.fx_translation_pnl);
    stmt.bind_double(17, record.model_change_pnl);
    stmt.bind_double(18, record.residual_pnl);
    stmt.bind_double(19, record.explained_pnl);
    stmt.bind_double(20, record.reconciliation_difference);
    stmt.bind_int(21, record.reconciliation_passed ? 1 : 0);
    stmt.bind_text(22, record.support_status);
    stmt.bind_text(23, record.model_name);
    stmt.bind_text(24, record.status_message);
    stmt.step_done();
}

void SQLiteStorageBackend::store_pnl_explain_component(const PnlExplainComponentRecord& record) {
    const char* sql = R"(
        INSERT OR REPLACE INTO pnl_explain_components (
            run_id,
            trade_id,
            component_sequence,
            component_id,
            component_type,
            label,
            amount,
            factor_id,
            risk_factor_group,
            model_name,
            support_status,
            status_message,
            tags_json
        )
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";
    Statement stmt(db_, sql);
    stmt.bind_text(1, record.run_id);
    stmt.bind_text(2, record.trade_id);
    stmt.bind_int(3, record.sequence);
    stmt.bind_text(4, record.component_id);
    stmt.bind_text(5, record.component_type);
    stmt.bind_text(6, record.label);
    stmt.bind_double(7, record.amount);
    stmt.bind_text(8, record.factor_id);
    stmt.bind_text(9, record.risk_factor_group);
    stmt.bind_text(10, record.model_name);
    stmt.bind_text(11, record.support_status);
    stmt.bind_text(12, record.status_message);
    stmt.bind_text(13, record.tags_json);
    stmt.step_done();
}

void SQLiteStorageBackend::store_var_result(const std::string& run_id,
                                            const std::string& method,
                                            double confidence_level,
                                            double var_value,
                                            double expected_shortfall,
                                            int scenario_count) {
    const char* sql = "INSERT OR REPLACE INTO var_results (run_id, method, confidence_level, "
                      "var_value, expected_shortfall, scenario_count) VALUES (?, ?, ?, ?, ?, ?);";
    Statement stmt(db_, sql);
    stmt.bind_text(1, run_id);
    stmt.bind_text(2, method);
    stmt.bind_double(3, confidence_level);
    stmt.bind_double(4, var_value);
    stmt.bind_double(5, expected_shortfall);
    stmt.bind_int(6, scenario_count);
    stmt.step_done();
}

void SQLiteStorageBackend::store_risk_result(const std::string& run_id,
                                             const std::string& trade_id,
                                             const std::string& measure,
                                             const std::string& rf_id,
                                             double value) {
    const char* sql = "INSERT OR REPLACE INTO risk_results (run_id, trade_id, risk_measure, "
                      "risk_factor_id, value) VALUES (?, ?, ?, ?, ?);";
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

void SQLiteStorageBackend::store_scenario_factor_shock(const std::string& set_id,
                                                       const std::string& scenario_name,
                                                       const std::string& factor_id,
                                                       double shock) {
    const char* sql = "INSERT OR REPLACE INTO scenario_factor_shocks (scenario_set_id, "
                      "scenario_name, factor_id, shock) VALUES (?, ?, ?, ?);";
    Statement stmt(db_, sql);
    stmt.bind_text(1, set_id);
    stmt.bind_text(2, scenario_name);
    stmt.bind_text(3, factor_id);
    stmt.bind_double(4, shock);
    stmt.step_done();
}

std::map<std::string, std::map<std::string, double>>
SQLiteStorageBackend::load_scenario_set(const std::string& set_id) {
    std::map<std::string, std::map<std::string, double>> set;
    const char* sql = "SELECT scenario_name, factor_id, shock FROM scenario_factor_shocks WHERE "
                      "scenario_set_id = ?;";
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
