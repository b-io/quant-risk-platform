#include <qrp/persistence/sqlite_storage_backend.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

// Stores portfolios and canonical trade JSON while preserving idempotent upsert behavior.

namespace qrp::persistence {

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
                                       const std::string& asset_class, const std::string& trade_type, const std::string& ccy,
                                       double notional, const std::string& start_date, const std::string& maturity_date,
                                       const std::string& direction, const std::string& economics_json) {
    const char* sql = "INSERT OR REPLACE INTO trades (trade_id, portfolio_id, book_id, asset_class, trade_type, currency, notional, start_date, maturity_date, direction, economics_json) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, trade_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, portfolio_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, book_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, asset_class.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, trade_type.c_str(), -1, SQLITE_TRANSIENT);
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

std::vector<std::shared_ptr<domain::Trade>> SQLiteStorageBackend::load_trades(const std::string& portfolio_id) {
    std::vector<std::shared_ptr<domain::Trade>> trades;
    const char* sql = "SELECT trade_id, asset_class, trade_type, currency, notional, start_date, maturity_date, direction, economics_json FROM trades WHERE portfolio_id = ?;";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, portfolio_id.c_str(), -1, SQLITE_TRANSIENT);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::shared_ptr<domain::Trade> t;
        try {
            t = domain::make_trade(type);
        } catch (...) {
            sqlite3_finalize(stmt);
            throw;
        }

        t->id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        t->asset_class = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        t->type = type;
        t->trade_type = domain::parse_trade_type(type);
        t->currency = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        t->direction = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        t->book = ""; // Database schema might need book/strategy in separate columns or economics
        t->strategy = "";

        const char* econ_json_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        if (econ_json_ptr) {
            nlohmann::json econ = nlohmann::json::parse(econ_json_ptr);
            // Construct a full trade JSON payload and delegate to domain parsing.
            nlohmann::json full_j;
            full_j["id"] = t->id;
            full_j["asset_class"] = t->asset_class;
            full_j["type"] = t->type;
            full_j["currency"] = t->currency;
            full_j["direction"] = t->direction;
            full_j["book"] = t->book;
            full_j["strategy"] = t->strategy;
            
            // Map flat columns if missing in econ
            if (!econ.contains("notional")) {
                double notional = sqlite3_column_double(stmt, 4);
                if (type == "equity_spot") full_j["quantity"] = notional;
                else full_j["notional"] = notional;
            }
            if (!econ.contains("start_date")) full_j["start_date"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            if (!econ.contains("maturity_date")) full_j["maturity_date"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));

            full_j["details"] = econ;
            t->from_json(full_j);
        }
        trades.push_back(t);
    }
    sqlite3_finalize(stmt);
    return trades;
}

} // namespace qrp::persistence

