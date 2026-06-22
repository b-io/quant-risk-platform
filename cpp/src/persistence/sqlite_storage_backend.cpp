#include <qrp/persistence/sqlite_storage_backend.hpp>
#include <fmt/format.h>

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

} // namespace qrp::persistence
