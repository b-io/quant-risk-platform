#include <qrp/persistence/sqlite_storage_backend.hpp>
#include <fmt/format.h>
#include <nlohmann/json.hpp>

namespace qrp::persistence {

void SQLiteStorageBackend::store_market_quote_event(const domain::MarketQuoteEvent& event) {
    const char* sql = "INSERT OR REPLACE INTO market_quote_events (event_id, quote_id, factor_id, market_ts, recorded_ts, source_name, source_ts, value, currency, tenor, instrument_type, overlay_set_id, metadata_json) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, event.event_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, event.quote_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, event.factor_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, event.market_ts.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, event.recorded_ts.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, event.source_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, event.source_ts.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 8, event.value);
    sqlite3_bind_text(stmt, 9, domain::to_string(event.currency).c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, event.tenor.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, event.instrument_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 12, event.overlay_set_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 13, event.metadata_json.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_finalize(stmt);
}

domain::MarketSnapshot SQLiteStorageBackend::load_market_snapshot_asof(
    const std::string& market_ts,
    const std::string& recorded_ts,
    const std::string& /*base_ccy*/,
    const std::string& /*overlay_set_id*/) {
    
    domain::MarketSnapshot snapshot;
    snapshot.valuation_date = market_ts;
    
    // Logic: latest quote event satisfying both cutoffs per quote_id.
    // Overlays can be implemented by ordering overlay_set_id matching.
    const char* sql = R"(
        WITH LatestEvents AS (
            SELECT quote_id, MAX(market_ts) as max_m_ts
            FROM market_quote_events
            WHERE market_ts <= ? AND recorded_ts <= ?
            GROUP BY quote_id
        )
        SELECT e.quote_id, e.value, e.currency, e.tenor, e.instrument_type, e.metadata_json
        FROM market_quote_events e
        JOIN LatestEvents le ON e.quote_id = le.quote_id AND e.market_ts = le.max_m_ts
        WHERE e.recorded_ts <= ?
    )";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, market_ts.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, recorded_ts.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, recorded_ts.c_str(), -1, SQLITE_TRANSIENT);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        domain::MarketQuote q;
        q.id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        q.value = sqlite3_column_double(stmt, 1);
        const char* ccy_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        if (ccy_ptr) q.currency = domain::from_string(ccy_ptr);
        q.tenor = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* inst_type_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        if (inst_type_ptr) {
            std::string s(inst_type_ptr);
            if (s == "Bond") q.instrument_type = domain::QuoteInstrumentType::Bond;
            else if (s == "CapFloorVol") q.instrument_type = domain::QuoteInstrumentType::CapFloorVol;
            else if (s == "CDS") q.instrument_type = domain::QuoteInstrumentType::CDS;
            else if (s == "Deposit") q.instrument_type = domain::QuoteInstrumentType::Deposit;
            else if (s == "FRA") q.instrument_type = domain::QuoteInstrumentType::FRA;
            else if (s == "Future") q.instrument_type = domain::QuoteInstrumentType::Future;
            else if (s == "IRS") q.instrument_type = domain::QuoteInstrumentType::IRS;
            else if (s == "OIS") q.instrument_type = domain::QuoteInstrumentType::OIS;
            else if (s == "SwaptionVol") q.instrument_type = domain::QuoteInstrumentType::SwaptionVol;
        }
        
        const char* meta = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        if (meta) {
            nlohmann::json j = nlohmann::json::parse(meta);
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
        if (!metadata_json_ptr) {
            sqlite3_finalize(stmt);
            throw std::runtime_error("Market quote is missing metadata_json: " + q.id);
        }

        nlohmann::json j = nlohmann::json::parse(metadata_json_ptr);
        q.instrument_type = j.at("instrument_type").get<domain::QuoteInstrumentType>();
        if (j.contains("tenor")) q.tenor = j["tenor"];
        if (j.contains("instrument_family")) q.instrument_family = j["instrument_family"];
        if (j.contains("index_family")) q.index_family = j["index_family"];
        if (j.contains("day_count")) q.day_count = j["day_count"];
        if (j.contains("calendar")) q.calendar = j["calendar"];
        if (j.contains("bdc")) q.bdc = j["bdc"];
        if (j.contains("settlement_days")) q.settlement_days = j["settlement_days"];
        snapshot.quotes.push_back(q);
    }
    sqlite3_finalize(stmt);
    
    return snapshot;
}

} // namespace qrp::persistence

