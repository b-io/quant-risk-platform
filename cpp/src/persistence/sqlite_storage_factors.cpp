// Implements SQLite persistence for factor definitions, factor history, and quote bindings.

#include <qrp/domain/factors.hpp>
#include <qrp/persistence/sqlite_storage_backend.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

// Stores factor definitions, bindings, and scenario shocks with taxonomy validation.

namespace qrp::persistence {

void SQLiteStorageBackend::store_factor_definition(const domain::FactorDefinition& factor) {
    const char* sql = "INSERT OR REPLACE INTO factor_definitions (factor_id, factor_type, shock_measure, currency, curve_id, tenor, quote_ids_json, description) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, factor.factor_id.c_str(), -1, SQLITE_TRANSIENT);
    const std::string type_str = domain::to_string(factor.factor_type);
    sqlite3_bind_text(stmt, 2, type_str.c_str(), -1, SQLITE_TRANSIENT);
    
    const std::string measure_str = domain::to_string(factor.shock_measure);
    sqlite3_bind_text(stmt, 3, measure_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, domain::to_string(factor.currency).c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, factor.curve_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, factor.tenor.c_str(), -1, SQLITE_TRANSIENT);
    
    std::string quotes_json = nlohmann::json(factor.quote_ids).dump();
    sqlite3_bind_text(stmt, 7, quotes_json.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, factor.description.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_finalize(stmt);
}

void SQLiteStorageBackend::store_factor_observation(const domain::FactorObservation& obs) {
    const char* sql = "INSERT OR REPLACE INTO factor_observations (factor_id, market_date, level, move, move_unit) VALUES (?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, obs.factor_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, obs.market_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, obs.level);
    sqlite3_bind_double(stmt, 4, obs.move);
    sqlite3_bind_text(stmt, 5, obs.move_unit.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_finalize(stmt);
}

std::vector<domain::FactorDefinition> SQLiteStorageBackend::load_factor_definitions(const std::string& /*portfolio_id*/) {
    std::vector<domain::FactorDefinition> factors;
    const char* sql = "SELECT factor_id, factor_type, shock_measure, currency, curve_id, tenor, quote_ids_json, description FROM factor_definitions;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        domain::FactorDefinition f;
        f.factor_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string type_s = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        f.factor_type = domain::parse_factor_type(type_s);
        
        std::string measure_s = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        f.shock_measure = domain::parse_shock_measure(measure_s);
        
        f.currency = domain::from_string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)));
        f.curve_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        f.tenor = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        const char* q_json = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        if (q_json) f.quote_ids = nlohmann::json::parse(q_json).get<std::vector<std::string>>();
        f.description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        factors.push_back(f);
    }
    sqlite3_finalize(stmt);
    return factors;
}

std::vector<domain::FactorObservation> SQLiteStorageBackend::load_factor_history(
    const std::vector<std::string>& factor_ids,
    const std::string& start_date,
    const std::string& end_date) {
    
    std::vector<domain::FactorObservation> history;
    // Simple IN clause construction
    std::string in_clause = "(";
    for (size_t i = 0; i < factor_ids.size(); ++i) {
        in_clause += "'" + factor_ids[i] + "'" + (i == factor_ids.size() - 1 ? "" : ",");
    }
    in_clause += ")";

    std::string sql = fmt::format("SELECT factor_id, market_date, level, move, move_unit FROM factor_observations "
                                  "WHERE factor_id IN {} AND market_date BETWEEN ? AND ? ORDER BY market_date ASC;", in_clause);
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, start_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, end_date.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        domain::FactorObservation o;
        o.factor_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        o.market_date = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        o.level = sqlite3_column_double(stmt, 2);
        o.move = sqlite3_column_double(stmt, 3);
        o.move_unit = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        history.push_back(o);
    }
    sqlite3_finalize(stmt);
    return history;
}

void SQLiteStorageBackend::store_factor_binding(const domain::FactorBinding& binding) {
    const char* sql = "INSERT OR REPLACE INTO factor_quote_bindings (factor_id, quote_id, shock_measure, weight, transform, selector_json) "
                      "VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, binding.factor_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, binding.quote_id.c_str(), -1, SQLITE_TRANSIENT);
    
    const std::string measure_str = domain::to_string(binding.shock_measure);
    sqlite3_bind_text(stmt, 3, measure_str.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 4, binding.weight);
    sqlite3_bind_text(stmt, 5, binding.transform.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, binding.selector_json.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_finalize(stmt);
}

std::vector<domain::FactorBinding> SQLiteStorageBackend::load_factor_bindings(const std::vector<std::string>& factor_ids) {
    std::vector<domain::FactorBinding> bindings;
    if (factor_ids.empty()) return bindings;

    std::string in_clause = "(";
    for (size_t i = 0; i < factor_ids.size(); ++i) {
        in_clause += "'" + factor_ids[i] + "'" + (i == factor_ids.size() - 1 ? "" : ",");
    }
    in_clause += ")";

    std::string sql = fmt::format("SELECT factor_id, quote_id, shock_measure, weight, transform, selector_json FROM factor_quote_bindings "
                                  "WHERE factor_id IN {};", in_clause);
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        domain::FactorBinding b;
        b.factor_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        b.quote_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        
        std::string measure_s = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        b.shock_measure = domain::parse_shock_measure(measure_s);

        b.weight = sqlite3_column_double(stmt, 3);
        const char* trans = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        if (trans) b.transform = trans;
        const char* select = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        if (select) b.selector_json = select;
        
        bindings.push_back(b);
    }
    sqlite3_finalize(stmt);
    return bindings;
}

} // namespace qrp::persistence

