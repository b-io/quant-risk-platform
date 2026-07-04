// Implements SQLite persistence for market snapshots, quotes, curve specs, and quote events.

#include <qrp/persistence/sqlite_storage_backend.hpp>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

namespace qrp::persistence {
namespace {

/**
 * @brief Reads a nullable SQLite text column into a safe std::string.
 */
std::string column_text(sqlite3_stmt* stmt, int column) {
    const auto* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, column));
    return text ? std::string(text) : std::string{};
}

/**
 * @brief Applies supplemental quote metadata stored as JSON onto a typed quote DTO.
 */
void apply_quote_metadata_json(const std::string& metadata_json, domain::MarketQuote& q) {
    if (metadata_json.empty()) {
        return;
    }

    const auto j = nlohmann::json::parse(metadata_json);
    if (j.contains("instrument_type")) q.instrument_type = j.at("instrument_type").get<domain::QuoteInstrumentType>();
    if (j.contains("risk_factor_id")) j.at("risk_factor_id").get_to(q.risk_factor_id);
    if (j.contains("quote_type")) q.quote_type = j.at("quote_type").get<domain::QuoteType>();
    if (j.contains("underlier")) j.at("underlier").get_to(q.underlier);
    if (j.contains("expiry")) j.at("expiry").get_to(q.expiry);
    if (j.contains("strike")) j.at("strike").get_to(q.strike);
    if (j.contains("tenor")) j.at("tenor").get_to(q.tenor);
    if (j.contains("instrument_family")) j.at("instrument_family").get_to(q.instrument_family);
    if (j.contains("index_family")) j.at("index_family").get_to(q.index_family);
    if (j.contains("day_count")) q.day_count = j.at("day_count").get<domain::DayCount>();
    if (j.contains("calendar")) q.calendar = j.at("calendar").get<domain::BusinessCalendar>();
    if (j.contains("bdc")) q.bdc = j.at("bdc").get<domain::BusinessDayConvention>();
    if (j.contains("settlement_days")) j.at("settlement_days").get_to(q.settlement_days);
    if (j.contains("market_ts")) j.at("market_ts").get_to(q.market_ts);
    if (j.contains("recorded_ts")) j.at("recorded_ts").get_to(q.recorded_ts);
    if (j.contains("source_name")) j.at("source_name").get_to(q.source_name);
    if (j.contains("source_ts")) j.at("source_ts").get_to(q.source_ts);
    if (j.contains("stale_after_days")) j.at("stale_after_days").get_to(q.stale_after_days);
}

} // namespace

/**
 * @brief Stores a bitemporal quote event for later as-of market reconstruction.
 */
void SQLiteStorageBackend::store_market_quote_event(const domain::MarketQuoteEvent& event) {
    const char* sql = R"(
        INSERT OR REPLACE INTO market_quote_events (
            event_id,
            quote_id,
            factor_id,
            market_ts,
            recorded_ts,
            source_name,
            source_ts,
            value,
            currency,
            tenor,
            instrument_type,
            overlay_set_id,
            metadata_json
        )
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;
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

/**
 * @brief Reconstructs the latest known quote set satisfying market-time and recorded-time cutoffs.
 */
domain::MarketSnapshot SQLiteStorageBackend::load_market_snapshot_asof(
    const std::string& market_ts,
    const std::string& recorded_ts,
    const std::string& base_ccy,
    const std::string& overlay_set_id) {

    domain::MarketSnapshot snapshot;
    snapshot.base_currency = domain::from_string(base_ccy);
    snapshot.recorded_ts = recorded_ts;
    snapshot.valuation_date = market_ts;

    const char* sql = R"(
        WITH ranked_events AS (
            SELECT
                e.*,
                ROW_NUMBER() OVER (
                    PARTITION BY e.quote_id
                    ORDER BY e.market_ts DESC, e.recorded_ts DESC
                ) AS row_num
            FROM market_quote_events e
            WHERE e.market_ts <= ?
              AND e.recorded_ts <= ?
              AND (? = '' OR e.overlay_set_id = '' OR e.overlay_set_id = ?)
        )
        SELECT
            quote_id,
            factor_id,
            value,
            currency,
            tenor,
            instrument_type,
            market_ts,
            recorded_ts,
            source_name,
            source_ts,
            metadata_json
        FROM ranked_events
        WHERE row_num = 1
        ORDER BY quote_id;
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }

    sqlite3_bind_text(stmt, 1, market_ts.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, recorded_ts.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, overlay_set_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, overlay_set_id.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        domain::MarketQuote q;
        q.id = column_text(stmt, 0);
        q.risk_factor_id = column_text(stmt, 1);
        q.value = sqlite3_column_double(stmt, 2);
        q.currency = domain::from_string(column_text(stmt, 3));
        q.tenor = column_text(stmt, 4);
        q.instrument_type = domain::parse_quote_instrument_type(column_text(stmt, 5));
        q.market_ts = column_text(stmt, 6);
        q.recorded_ts = column_text(stmt, 7);
        q.source_name = column_text(stmt, 8);
        q.source_ts = column_text(stmt, 9);

        apply_quote_metadata_json(column_text(stmt, 10), q);
        snapshot.quotes.push_back(q);
    }
    sqlite3_finalize(stmt);

    snapshot.diagnostics = domain::collect_market_snapshot_diagnostics(snapshot);
    return snapshot;
}

/**
 * @brief Stores market snapshot-level metadata and curve construction specifications.
 */
void SQLiteStorageBackend::store_market_snapshot(
    const std::string& snapshot_id,
    const std::string& as_of_date,
    const std::string& base_ccy,
    const std::string& curves_json) {

    const char* sql = R"(
        INSERT OR REPLACE INTO market_snapshots (
            snapshot_id,
            schema_version,
            as_of_date,
            base_currency,
            source_name,
            recorded_ts,
            default_stale_after_days,
            curves_json
        )
        VALUES (?, ?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }

    sqlite3_bind_text(stmt, 1, snapshot_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, 2);
    sqlite3_bind_text(stmt, 3, as_of_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, base_ccy.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, "SQLiteStorageBackend", -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, as_of_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, -1);
    sqlite3_bind_text(stmt, 8, curves_json.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_finalize(stmt);
}

/**
 * @brief Stores a typed market quote and selected metadata as indexed columns.
 */
void SQLiteStorageBackend::store_market_quote(
    const std::string& snapshot_id,
    const std::string& quote_id,
    double value,
    const std::string& ccy,
    const std::string& metadata_json) {

    domain::MarketQuote q;
    q.id = quote_id;
    q.value = value;
    q.currency = domain::from_string(ccy);
    apply_quote_metadata_json(metadata_json, q);

    const char* sql = R"(
        INSERT OR REPLACE INTO market_quotes (
            snapshot_id,
            quote_id,
            risk_factor_id,
            quote_type,
            underlier,
            expiry,
            strike,
            value,
            currency,
            tenor,
            instrument_type,
            market_ts,
            recorded_ts,
            source_name,
            source_ts,
            stale_after_days,
            metadata_json
        )
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }

    const auto instrument_type = domain::to_string(q.instrument_type);
    const auto quote_type = domain::to_string(q.quote_type);

    sqlite3_bind_text(stmt, 1, snapshot_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, q.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, q.risk_factor_id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, quote_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, q.underlier.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, q.expiry.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, q.strike.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 8, q.value);
    sqlite3_bind_text(stmt, 9, domain::to_string(q.currency).c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, q.tenor.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 11, instrument_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 12, q.market_ts.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 13, q.recorded_ts.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 14, q.source_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 15, q.source_ts.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 16, q.stale_after_days);
    sqlite3_bind_text(stmt, 17, metadata_json.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        throw std::runtime_error(fmt::format("Failed to execute statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_finalize(stmt);
}

/**
 * @brief Loads a persisted market snapshot, quotes, curve specs, and validation diagnostics.
 */
domain::MarketSnapshot SQLiteStorageBackend::load_market_snapshot(const std::string& snapshot_id) {
    domain::MarketSnapshot snapshot;

    const char* snapshot_sql = R"(
        SELECT
            snapshot_id,
            schema_version,
            as_of_date,
            base_currency,
            source_name,
            recorded_ts,
            default_stale_after_days,
            curves_json,
            diagnostics_json
        FROM market_snapshots
        WHERE snapshot_id = ?;
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, snapshot_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, snapshot_id.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        snapshot.snapshot_id = column_text(stmt, 0);
        snapshot.schema_version = sqlite3_column_int(stmt, 1);
        snapshot.valuation_date = column_text(stmt, 2);
        snapshot.base_currency = domain::from_string(column_text(stmt, 3));
        snapshot.source_name = column_text(stmt, 4);
        snapshot.recorded_ts = column_text(stmt, 5);
        snapshot.default_stale_after_days = sqlite3_column_int(stmt, 6);

        const auto curves_json = column_text(stmt, 7);
        if (!curves_json.empty()) {
            snapshot.curves = nlohmann::json::parse(curves_json).get<std::vector<domain::CurveSpec>>();
        }

        const auto diagnostics_json = column_text(stmt, 8);
        if (!diagnostics_json.empty()) {
            snapshot.diagnostics = nlohmann::json::parse(diagnostics_json).get<std::vector<domain::MarketDataDiagnostic>>();
        }
    } else {
        sqlite3_finalize(stmt);
        throw std::runtime_error("Snapshot not found: " + snapshot_id);
    }
    sqlite3_finalize(stmt);

    const char* quote_sql = R"(
        SELECT
            quote_id,
            risk_factor_id,
            quote_type,
            underlier,
            expiry,
            strike,
            value,
            currency,
            tenor,
            instrument_type,
            market_ts,
            recorded_ts,
            source_name,
            source_ts,
            stale_after_days,
            metadata_json
        FROM market_quotes
        WHERE snapshot_id = ?
        ORDER BY quote_id;
    )";

    if (sqlite3_prepare_v2(db_, quote_sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, snapshot_id.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        domain::MarketQuote q;
        q.id = column_text(stmt, 0);
        q.risk_factor_id = column_text(stmt, 1);
        q.quote_type = domain::parse_quote_type(column_text(stmt, 2));
        q.underlier = column_text(stmt, 3);
        q.expiry = column_text(stmt, 4);
        q.strike = column_text(stmt, 5);
        q.value = sqlite3_column_double(stmt, 6);
        q.currency = domain::from_string(column_text(stmt, 7));
        q.tenor = column_text(stmt, 8);
        q.instrument_type = domain::parse_quote_instrument_type(column_text(stmt, 9));
        q.market_ts = column_text(stmt, 10);
        q.recorded_ts = column_text(stmt, 11);
        q.source_name = column_text(stmt, 12);
        q.source_ts = column_text(stmt, 13);
        q.stale_after_days = sqlite3_column_int(stmt, 14);

        apply_quote_metadata_json(column_text(stmt, 15), q);
        snapshot.quotes.push_back(q);
    }
    sqlite3_finalize(stmt);

    snapshot.diagnostics = domain::collect_market_snapshot_diagnostics(snapshot);
    return snapshot;
}

} // namespace qrp::persistence
