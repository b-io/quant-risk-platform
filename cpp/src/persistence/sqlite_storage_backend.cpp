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
            // Rough mapping or just store as string
            if (s == "Deposit") q.instrument_type = domain::QuoteInstrumentType::Deposit;
            else if (s == "OIS") q.instrument_type = domain::QuoteInstrumentType::OIS;
            else if (s == "IRS") q.instrument_type = domain::QuoteInstrumentType::IRS;
            else if (s == "FRA") q.instrument_type = domain::QuoteInstrumentType::FRA;
            else if (s == "Future") q.instrument_type = domain::QuoteInstrumentType::Future;
            else if (s == "Bond") q.instrument_type = domain::QuoteInstrumentType::Bond;
            else if (s == "CDS") q.instrument_type = domain::QuoteInstrumentType::CDS;
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

void SQLiteStorageBackend::store_factor_definition(const domain::FactorDefinition& factor) {
    const char* sql = "INSERT OR REPLACE INTO factor_definitions (factor_id, factor_type, shock_measure, currency, curve_id, tenor, quote_ids_json, description) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, factor.factor_id.c_str(), -1, SQLITE_TRANSIENT);
    // FactorType mapping (simplistic string conversion for storage)
    std::string type_str = "Custom";
    switch(factor.factor_type) {
        case domain::FactorType::FXSpot: type_str = "FXSpot"; break;
        case domain::FactorType::RateZero: type_str = "RateZero"; break;
        case domain::FactorType::RateForward: type_str = "RateForward"; break;
        case domain::FactorType::CreditSpread: type_str = "CreditSpread"; break;
        case domain::FactorType::HazardRate: type_str = "HazardRate"; break;
        case domain::FactorType::Volatility: type_str = "Volatility"; break;
        case domain::FactorType::EquitySpot: type_str = "EquitySpot"; break;
        case domain::FactorType::CommodityForward: type_str = "CommodityForward"; break;
        case domain::FactorType::BasisSpread: type_str = "BasisSpread"; break;
        default: break;
    }
    sqlite3_bind_text(stmt, 2, type_str.c_str(), -1, SQLITE_TRANSIENT);
    
    std::string measure_str = "Absolute";
    switch(factor.shock_measure) {
        case domain::ShockMeasure::Relative: measure_str = "Relative"; break;
        case domain::ShockMeasure::LogReturn: measure_str = "LogReturn"; break;
        case domain::ShockMeasure::BasisPoints: measure_str = "BasisPoints"; break;
        case domain::ShockMeasure::VolPoints: measure_str = "VolPoints"; break;
        default: break;
    }
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
    // For now, load all factors (mapping to portfolio can be done via another table if needed)
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
        if (type_s == "FXSpot") f.factor_type = domain::FactorType::FXSpot;
        else if (type_s == "RateZero") f.factor_type = domain::FactorType::RateZero;
        else if (type_s == "RateForward") f.factor_type = domain::FactorType::RateForward;
        else if (type_s == "CreditSpread") f.factor_type = domain::FactorType::CreditSpread;
        else if (type_s == "HazardRate") f.factor_type = domain::FactorType::HazardRate;
        else if (type_s == "Volatility") f.factor_type = domain::FactorType::Volatility;
        else if (type_s == "EquitySpot") f.factor_type = domain::FactorType::EquitySpot;
        else if (type_s == "CommodityForward") f.factor_type = domain::FactorType::CommodityForward;
        else if (type_s == "BasisSpread") f.factor_type = domain::FactorType::BasisSpread;
        
        std::string measure_s = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        if (measure_s == "Relative") f.shock_measure = domain::ShockMeasure::Relative;
        else if (measure_s == "LogReturn") f.shock_measure = domain::ShockMeasure::LogReturn;
        else if (measure_s == "BasisPoints") f.shock_measure = domain::ShockMeasure::BasisPoints;
        else if (measure_s == "VolPoints") f.shock_measure = domain::ShockMeasure::VolPoints;
        
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
    
    std::string measure_str = "Absolute";
    switch(binding.shock_measure) {
        case domain::ShockMeasure::Relative: measure_str = "Relative"; break;
        case domain::ShockMeasure::LogReturn: measure_str = "LogReturn"; break;
        case domain::ShockMeasure::BasisPoints: measure_str = "BasisPoints"; break;
        case domain::ShockMeasure::VolPoints: measure_str = "VolPoints"; break;
        default: break;
    }
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
        if (measure_s == "Relative") b.shock_measure = domain::ShockMeasure::Relative;
        else if (measure_s == "LogReturn") b.shock_measure = domain::ShockMeasure::LogReturn;
        else if (measure_s == "BasisPoints") b.shock_measure = domain::ShockMeasure::BasisPoints;
        else if (measure_s == "VolPoints") b.shock_measure = domain::ShockMeasure::VolPoints;
        else b.shock_measure = domain::ShockMeasure::Absolute;

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

std::vector<std::shared_ptr<domain::Trade>> SQLiteStorageBackend::load_trades(const std::string& portfolio_id) {
    std::vector<std::shared_ptr<domain::Trade>> trades;
    const char* sql = "SELECT trade_id, asset_class, product_type, currency, notional, start_date, maturity_date, direction, economics_json FROM trades WHERE portfolio_id = ?;";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error(fmt::format("Failed to prepare statement: {}", sqlite3_errmsg(db_)));
    }
    sqlite3_bind_text(stmt, 1, portfolio_id.c_str(), -1, SQLITE_TRANSIENT);
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        std::shared_ptr<domain::Trade> t;
        if (type == "vanilla_swap") {
            t = std::make_shared<domain::VanillaSwapTrade>();
        } else if (type == "fixed_rate_bond") {
            t = std::make_shared<domain::FixedRateBondTrade>();
        } else if (type == "equity_spot") {
            t = std::make_shared<domain::EquitySpotTrade>();
        } else if (type == "fx_forward") {
            t = std::make_shared<domain::FxForwardTrade>();
        } else {
            // Fallback for unknown types if we want to be robust but strict
            t = std::make_shared<domain::Trade>();
        }

        t->id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        t->asset_class = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        t->type = type;
        t->currency = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        t->direction = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        t->book = ""; // Database schema might need book/strategy in separate columns or economics
        t->strategy = "";

        const char* econ_json_ptr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        if (econ_json_ptr) {
            nlohmann::json econ = nlohmann::json::parse(econ_json_ptr);
            // Construct a temporary full JSON to use the from_json logic
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
