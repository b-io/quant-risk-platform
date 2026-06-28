// Verifies SQLite persistence round trips for portfolios, market data, factors, scenarios, and results.

#include <gtest/gtest.h>
#include <qrp/app/quant_risk_platform.hpp>
#include <qrp/persistence/sqlite_storage_backend.hpp>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <sqlite3.h>

namespace qrp::testing {

namespace {

double QueryDouble(const std::string& db_path, const std::string& sql) {
    sqlite3* db = nullptr;
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open test database");
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        throw std::runtime_error("Failed to prepare test query");
    }

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        throw std::runtime_error("Test query returned no rows");
    }

    double value = sqlite3_column_double(stmt, 0);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return value;
}

std::string QueryString(const std::string& db_path, const std::string& sql) {
    sqlite3* db = nullptr;
    if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open test database");
    }

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_close(db);
        throw std::runtime_error("Failed to prepare test query");
    }

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        throw std::runtime_error("Test query returned no rows");
    }

    const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    std::string value = text ? text : "";
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return value;
}

} // namespace

class PersistenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path = "test_qrp.db";
        storage = std::make_shared<persistence::SQLiteStorageBackend>(db_path);
        storage->initialize_schema();
    }

    void TearDown() override {
        storage.reset();
        // Try to remove, but don't fail if it's locked or already gone
        try {
            if (std::filesystem::exists(db_path)) {
                std::filesystem::remove(db_path);
            }
        } catch (...) {}
    }

    std::string db_path;
    std::shared_ptr<persistence::SQLiteStorageBackend> storage;
};

TEST_F(PersistenceTest, PortfolioIdempotency) {
    storage->store_portfolio("P1", "Portfolio 1", "USD");
    auto portfolios = storage->list_portfolios();
    EXPECT_EQ(portfolios.size(), 1);
    EXPECT_EQ(portfolios[0], "P1");

    // Idempotent call - same data
    storage->store_portfolio("P1", "Portfolio 1", "USD");
    portfolios = storage->list_portfolios();
    EXPECT_EQ(portfolios.size(), 1);

    // Update existing portfolio
    storage->store_portfolio("P1", "Portfolio 1 Updated", "EUR");
    portfolios = storage->list_portfolios();
    EXPECT_EQ(portfolios.size(), 1);
}

TEST_F(PersistenceTest, MarketSnapshotIdempotency) {
    storage->store_market_snapshot("S1", "2023-01-01", "USD", "[]");
    auto snapshots = storage->list_snapshots();
    EXPECT_EQ(snapshots.size(), 1);
    EXPECT_EQ(snapshots[0], "S1");

    // Idempotent call
    storage->store_market_snapshot("S1", "2023-01-01", "USD", "[]");
    snapshots = storage->list_snapshots();
    EXPECT_EQ(snapshots.size(), 1);
}

TEST_F(PersistenceTest, TradeStorageAndLoading) {
    storage->store_portfolio("P1", "P1", "USD");
    storage->store_book("B1", "P1", "Book 1");
    storage->store_trade("T1", "P1", "B1", "Rates", "vanilla_swap", "USD", 1000000.0, "2024-01-01", "2034-01-01", "Buy", "{\"fixed_rate\": 0.03, \"floating_index\": \"USD_LIBOR_3M\"}");

    auto trades = storage->load_trades("P1");
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0]->id, "T1");
    EXPECT_EQ(trades[0]->asset_class, "Rates");
    auto swap = std::dynamic_pointer_cast<domain::VanillaSwapTrade>(trades[0]);
    ASSERT_NE(swap, nullptr);
    EXPECT_EQ(swap->notional, 1000000.0);
}

TEST_F(PersistenceTest, MarketSnapshotQuotesRoundTrip) {
    storage->store_market_snapshot("S1", "2023-01-01", "USD", "[]");
    storage->store_market_quote("S1", "USD.SOFR.10Y", 0.035, "USD", "{\"instrument_type\": \"OIS\", \"tenor\": \"10Y\"}");
    storage->store_market_quote("S1", "USD.SOFR.5Y", 0.032, "USD", "{\"instrument_type\": \"OIS\", \"tenor\": \"5Y\"}");

    // Second snapshot with same quote IDs
    storage->store_market_snapshot("S2", "2023-01-02", "USD", "[]");
    storage->store_market_quote("S2", "USD.SOFR.10Y", 0.036, "USD", "{\"instrument_type\": \"OIS\", \"tenor\": \"10Y\"}");

    auto s1 = storage->load_market_snapshot("S1");
    EXPECT_EQ(s1.quotes.size(), 2);
    
    auto s2 = storage->load_market_snapshot("S2");
    EXPECT_EQ(s2.quotes.size(), 1);
    EXPECT_NEAR(s2.quotes[0].value, 0.036, 1e-10);
    EXPECT_EQ(s2.quotes[0].tenor, "10Y");
}

TEST_F(PersistenceTest, MarketSnapshotPreservesPhase2MarketFields) {
    storage->store_market_snapshot("S_PHASE2", "2026-03-24", "USD", R"json([
        {
          "id": { "currency": "USD", "family": "OIS" },
          "purpose": "OISDiscount",
          "construction_family": "RatesOIS",
          "quote_ids": ["USD_OIS_2Y"],
          "day_count": "ACT360",
          "calendar": "US",
          "interpolation": "LogLinear"
        }
    ])json");
    storage->store_market_quote("S_PHASE2", "EURUSD.SPOT", 1.085, "USD", R"json({
        "instrument_type": "FXSpot",
        "quote_type": "FXSpot",
        "risk_factor_id": "RF:FX:EURUSD:SPOT",
        "tenor": "SPOT",
        "underlier": "EURUSD",
        "source_name": "Composite",
        "source_ts": "2026-03-24T17:00:00Z",
        "stale_after_days": 1
    })json");
    storage->store_market_quote("S_PHASE2", "USD_OIS_2Y", 0.0538, "USD", R"json({
        "instrument_type": "OIS",
        "quote_type": "OIS",
        "risk_factor_id": "RF:RATES:USD:OIS:2Y",
        "tenor": "2Y",
        "source_name": "Composite",
        "source_ts": "2026-03-24T17:00:00Z",
        "stale_after_days": 1
    })json");

    auto snapshot = storage->load_market_snapshot("S_PHASE2");

    EXPECT_EQ(snapshot.snapshot_id, "S_PHASE2");
    EXPECT_EQ(snapshot.schema_version, 2);
    EXPECT_EQ(snapshot.base_currency, domain::Currency::USD);
    ASSERT_EQ(snapshot.curves.size(), 1U);
    EXPECT_EQ(snapshot.curves[0].purpose, domain::CurvePurpose::OISDiscount);
    EXPECT_EQ(snapshot.curves[0].construction_family, "RatesOIS");
    ASSERT_EQ(snapshot.quotes.size(), 2U);
    EXPECT_EQ(snapshot.quotes[0].id, "EURUSD.SPOT");
    EXPECT_EQ(snapshot.quotes[0].instrument_type, domain::QuoteInstrumentType::FXSpot);
    EXPECT_EQ(snapshot.quotes[0].quote_type, domain::QuoteType::FXSpot);
    EXPECT_EQ(snapshot.quotes[0].risk_factor_id, "RF:FX:EURUSD:SPOT");
    EXPECT_EQ(snapshot.quotes[0].underlier, "EURUSD");
    EXPECT_TRUE(snapshot.diagnostics.empty());
    EXPECT_EQ(
        QueryString(db_path, "SELECT quote_type FROM market_quotes WHERE snapshot_id = 'S_PHASE2' AND quote_id = 'EURUSD.SPOT';"),
        "FXSpot");
}

TEST_F(PersistenceTest, MarketQuoteEventsLoadLatestAsOfWithProvenance) {
    domain::MarketQuoteEvent old_event;
    old_event.event_id = "EVT_OLD";
    old_event.quote_id = "AAPL.SPOT";
    old_event.factor_id = "RF:EQ:AAPL:SPOT";
    old_event.market_ts = "2026-03-23";
    old_event.recorded_ts = "2026-03-23T18:00:00Z";
    old_event.source_name = "Composite";
    old_event.source_ts = "2026-03-23T17:00:00Z";
    old_event.value = 184.0;
    old_event.currency = domain::Currency::USD;
    old_event.tenor = "SPOT";
    old_event.instrument_type = "EquitySpot";
    old_event.metadata_json = R"json({
        "quote_type": "EquitySpot",
        "underlier": "AAPL",
        "stale_after_days": 2
    })json";
    storage->store_market_quote_event(old_event);

    domain::MarketQuoteEvent new_event = old_event;
    new_event.event_id = "EVT_NEW";
    new_event.market_ts = "2026-03-24";
    new_event.recorded_ts = "2026-03-24T18:00:00Z";
    new_event.source_ts = "2026-03-24T17:00:00Z";
    new_event.value = 185.5;
    storage->store_market_quote_event(new_event);

    auto snapshot = storage->load_market_snapshot_asof("2026-03-24", "2026-03-24T19:00:00Z", "USD");

    EXPECT_EQ(snapshot.valuation_date, "2026-03-24");
    EXPECT_EQ(snapshot.base_currency, domain::Currency::USD);
    ASSERT_EQ(snapshot.quotes.size(), 1U);
    EXPECT_EQ(snapshot.quotes[0].id, "AAPL.SPOT");
    EXPECT_EQ(snapshot.quotes[0].instrument_type, domain::QuoteInstrumentType::EquitySpot);
    EXPECT_EQ(snapshot.quotes[0].quote_type, domain::QuoteType::EquitySpot);
    EXPECT_EQ(snapshot.quotes[0].risk_factor_id, "RF:EQ:AAPL:SPOT");
    EXPECT_EQ(snapshot.quotes[0].source_name, "Composite");
    EXPECT_DOUBLE_EQ(snapshot.quotes[0].value, 185.5);
    EXPECT_TRUE(snapshot.diagnostics.empty());
}

TEST_F(PersistenceTest, ValuationResultsPersistence) {
    storage->store_portfolio("P1", "P1", "USD");
    storage->store_market_snapshot("S1", "2023-01-01", "USD", "[]");
    std::string run_id = "RUN1";
    storage->store_analysis_run(run_id, "VALUATION", "P1", "S1");
    storage->store_valuation_result(
        run_id,
        "T1",
        1234.56,
        "USD",
        "SUCCESS",
        "",
        "rates",
        "vanilla_swap",
        "supported",
        "QuantLib::VanillaSwap/DiscountingSwapEngine",
        "Vanilla fixed-float swap pricing is supported for configured rates curves");

    auto results = storage->get_valuation_results(run_id);
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].trade_id, "T1");
    EXPECT_NEAR(results[0].npv, 1234.56, 1e-10);
    EXPECT_EQ(results[0].status, "SUCCESS");
    EXPECT_EQ(results[0].asset_class, "rates");
    EXPECT_EQ(results[0].product_type, "vanilla_swap");
    EXPECT_EQ(results[0].support_status, "supported");
    EXPECT_EQ(results[0].model_name, "QuantLib::VanillaSwap/DiscountingSwapEngine");
    EXPECT_FALSE(results[0].status_message.empty());
    EXPECT_EQ(
        QueryString(db_path, "SELECT support_status FROM valuation_results WHERE run_id = 'RUN1' AND trade_id = 'T1';"),
        "supported");
    EXPECT_EQ(
        QueryString(db_path, "SELECT model_name FROM valuation_results WHERE run_id = 'RUN1' AND trade_id = 'T1';"),
        "QuantLib::VanillaSwap/DiscountingSwapEngine");
}

TEST_F(PersistenceTest, ScenarioAndVarResultsPersistence) {
    storage->store_portfolio("P1", "P1", "USD");
    storage->store_market_snapshot("S1", "2023-01-01", "USD", "[]");
    const std::string run_id = "RUN_HVAR_1";
    storage->store_analysis_run(run_id, "HISTORICAL_VAR", "P1", "S1");
    storage->store_scenario_result(run_id, "RISK_OFF", -1250.0);
    storage->store_scenario_result(run_id, "RATES_UP", 350.0);
    storage->store_var_result(run_id, "HISTORICAL", 0.95, 1250.0, 1250.0, 2);

    EXPECT_EQ(QueryDouble(db_path, "SELECT count(*) FROM scenario_results WHERE run_id = 'RUN_HVAR_1';"), 2.0);
    EXPECT_NEAR(QueryDouble(db_path, "SELECT portfolio_pnl FROM scenario_results WHERE scenario_name = 'RISK_OFF';"), -1250.0, 1e-10);
    EXPECT_NEAR(QueryDouble(db_path, "SELECT var_value FROM var_results WHERE run_id = 'RUN_HVAR_1';"), 1250.0, 1e-10);
    EXPECT_NEAR(QueryDouble(db_path, "SELECT expected_shortfall FROM var_results WHERE run_id = 'RUN_HVAR_1';"), 1250.0, 1e-10);
}

TEST_F(PersistenceTest, ScenarioImportRejectsNonCanonicalFactorIds) {
    const std::string scenario_path = "bad_factor_scenario.json";
    std::ofstream out(scenario_path);
    out << R"json(
        {
          "scenario_set_id": "bad_factor_set",
          "factors": [
            {
              "factor_id": "RF:RATES:USD::5Y",
              "factor_type": "RateZero",
              "shock_measure": "Absolute"
            }
          ],
          "bindings": [],
          "scenarios": {}
        }
    )json";
    out.close();

    app::QuantRiskPlatform platform(storage);
    EXPECT_THROW(platform.import_scenario_set(scenario_path), std::invalid_argument);
    std::filesystem::remove(scenario_path);
}

TEST_F(PersistenceTest, ScenarioImportRejectsUndefinedBindingFactorIds) {
    const std::string scenario_path = "bad_binding_scenario.json";
    std::ofstream out(scenario_path);
    out << R"json(
        {
          "scenario_set_id": "bad_binding_set",
          "factors": [
            {
              "factor_id": "RF:EQ:AAPL:SPOT",
              "factor_type": "EquitySpot",
              "shock_measure": "Relative"
            }
          ],
          "bindings": [
            {
              "factor_id": "RF:EQ:MSFT:SPOT",
              "quote_id": "MSFT",
              "shock_measure": "Relative"
            }
          ],
          "scenarios": {}
        }
    )json";
    out.close();

    app::QuantRiskPlatform platform(storage);
    EXPECT_THROW(platform.import_scenario_set(scenario_path), std::invalid_argument);
    std::filesystem::remove(scenario_path);
}

} // namespace qrp::testing
