// Verifies the application facade against the checked-in demo workflow data.

#include "test_paths.hpp"

#include <qrp/app/quant_risk_platform.hpp>
#include <qrp/domain/portfolio.hpp>
#include <qrp/io/json_loader.hpp>
#include <qrp/persistence/sqlite_storage_backend.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sqlite3.h>
#include <stdexcept>
#include <string>

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

    const double value = sqlite3_column_double(stmt, 0);
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return value;
}

class AppWorkflowTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path = "test_app_workflows.db";
        storage = std::make_shared<persistence::SQLiteStorageBackend>(db_path);
    }

    void TearDown() override {
        storage.reset();

        // SQLite can keep Windows handles briefly; cleanup is best effort for local test runs.
        try {
            if (std::filesystem::exists(db_path)) {
                std::filesystem::remove(db_path);
            }
        } catch (...) {
        }
    }

    std::string db_path;
    std::shared_ptr<persistence::SQLiteStorageBackend> storage;
};

class TemporaryJsonFile {
public:
    explicit TemporaryJsonFile(const std::string& body) {
        const auto suffix = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        path_ = std::filesystem::current_path() / ("qrp_app_workflow_" + std::to_string(suffix) + ".json");
        std::ofstream out(path_);
        out << body;
    }

    ~TemporaryJsonFile() {
        std::error_code ec;
        std::filesystem::remove(path_, ec);
    }

    std::string string() const {
        return path_.string();
    }

private:
    std::filesystem::path path_;
};

} // namespace

TEST_F(AppWorkflowTest, RunsDemoImportAnalyticsAndReports) {
    app::QuantRiskPlatform platform(storage);
    platform.initialize();

    platform.import_market_snapshot(test::data_file({"market", "demo_market.json"}).string());
    platform.import_portfolio(test::data_file({"portfolios", "demo_portfolio.json"}).string());
    platform.import_scenario_set(test::data_file({"scenarios", "demo_scenarios.json"}).string());

    const auto valuation_run = platform.run_valuation("demo_portfolio", "SNAP:2026-03-24");
    const auto valuation_records = storage->get_valuation_results(valuation_run);
    ASSERT_EQ(valuation_records.size(), 32U);

    const auto risk_run = platform.run_risk("demo_portfolio", "SNAP:2026-03-24");
    EXPECT_FALSE(risk_run.empty());

    const auto var_run = platform.run_historical_var("demo_portfolio", "SNAP:2026-03-24", "demo_factor_scenarios");
    EXPECT_FALSE(var_run.empty());
    EXPECT_GT(QueryDouble(db_path, "SELECT count(*) FROM var_scenario_pnls WHERE run_id = '" + var_run + "';"), 0.0);
    EXPECT_GT(QueryDouble(db_path, "SELECT count(*) FROM var_contributions WHERE run_id = '" + var_run + "';"), 0.0);
    EXPECT_GT(QueryDouble(db_path,
                          "SELECT count(*) FROM var_contributions WHERE run_id = '" + var_run +
                              "' AND aggregation_type = 'risk_factor';"),
              0.0);

    EXPECT_NO_THROW(platform.compare_runs(valuation_run, valuation_run));
    EXPECT_NO_THROW(platform.get_run_report(valuation_run));
    EXPECT_NO_THROW(platform.get_run_report("missing_run"));
    EXPECT_NO_THROW(platform.list_data());
}

TEST_F(AppWorkflowTest, ReportsMixedCurrencyRunsAndComparesNewTrades) {
    app::QuantRiskPlatform platform(storage);
    platform.initialize();

    storage->store_portfolio("P_REPORT", "Reporting portfolio", "USD");
    storage->store_market_snapshot("S_REPORT", "2026-03-24", "USD");
    storage->store_analysis_run("RUN_REPORT_1", "VALUATION", "P_REPORT", "S_REPORT");
    storage->store_analysis_run("RUN_REPORT_2", "VALUATION", "P_REPORT", "S_REPORT");

    storage->store_valuation_result("RUN_REPORT_1",
                                    "T_USD",
                                    100.0,
                                    "USD",
                                    "SUCCESS",
                                    "",
                                    "rates",
                                    "deposit",
                                    "supported",
                                    "UnitModel",
                                    "");
    storage->store_valuation_result("RUN_REPORT_2",
                                    "T_USD",
                                    125.0,
                                    "USD",
                                    "SUCCESS",
                                    "",
                                    "rates",
                                    "deposit",
                                    "supported",
                                    "UnitModel",
                                    "");
    storage->store_valuation_result("RUN_REPORT_2",
                                    "T_EUR_NEW",
                                    75.0,
                                    "EUR",
                                    "SUCCESS",
                                    "",
                                    "fx",
                                    "fx_forward",
                                    "supported",
                                    "UnitModel",
                                    "");

    EXPECT_NO_THROW(platform.get_run_report("RUN_REPORT_2"));
    EXPECT_NO_THROW(platform.compare_runs("RUN_REPORT_1", "RUN_REPORT_2"));
    EXPECT_NO_THROW(platform.list_data());
}

TEST_F(AppWorkflowTest, RiskWorkflowsRejectMissingFactorConfiguration) {
    app::QuantRiskPlatform platform(storage);
    platform.initialize();

    platform.import_market_snapshot(test::data_file({"market", "demo_market.json"}).string());
    platform.import_portfolio(test::data_file({"portfolios", "demo_portfolio.json"}).string());

    EXPECT_THROW({ platform.run_risk("demo_portfolio", "SNAP:2026-03-24"); }, std::runtime_error);

    EXPECT_THROW(
        { platform.run_historical_var("demo_portfolio", "SNAP:2026-03-24", "missing_scenarios"); },
        std::runtime_error);
}

TEST_F(AppWorkflowTest, RunsPnlExplainWorkflowWithImportedFactors) {
    TemporaryJsonFile previous_market_file(R"json(
        {
          "valuation_date": "2026-03-24",
          "quotes": [
            {
              "id": "AAPL",
              "instrument_type": "EquitySpot",
              "currency": "USD",
              "tenor": "SPOT",
              "value": 100.0
            }
          ]
        }
    )json");
    TemporaryJsonFile current_market_file(R"json(
        {
          "valuation_date": "2026-03-25",
          "quotes": [
            {
              "id": "AAPL",
              "instrument_type": "EquitySpot",
              "currency": "USD",
              "tenor": "SPOT",
              "value": 110.0
            }
          ]
        }
    )json");
    TemporaryJsonFile portfolio_file(R"json(
        {
          "portfolio_id": "P_APP_PNL",
          "trades": [
            {
              "id": "equity_aapl",
              "asset_class": "equity",
              "type": "equity_spot",
              "currency": "USD",
              "direction": "long",
              "book": "BOOK:EQUITY",
              "strategy": "DELTA",
              "quantity": 10.0,
              "details": {
                "reference_price": 100.0,
                "underlier": "AAPL"
              }
            }
          ]
        }
    )json");
    TemporaryJsonFile scenario_file(R"json(
        {
          "scenario_set_id": "pnl_factor_set",
          "factors": [
            {
              "factor_id": "RF:EQ:AAPL:SPOT",
              "factor_type": "EquitySpot",
              "shock_measure": "Relative",
              "currency": "USD",
              "quote_ids": [
                "AAPL"
              ]
            }
          ],
          "bindings": [
            {
              "factor_id": "RF:EQ:AAPL:SPOT",
              "quote_id": "AAPL",
              "shock_measure": "Relative"
            }
          ],
          "scenarios": {}
        }
    )json");

    app::QuantRiskPlatform platform(storage);
    platform.initialize();
    platform.import_market_snapshot(previous_market_file.string());
    platform.import_market_snapshot(current_market_file.string());
    platform.import_portfolio(portfolio_file.string());
    platform.import_scenario_set(scenario_file.string());

    const auto run_id = platform.run_pnl_explain("P_APP_PNL", "SNAP:2026-03-24", "SNAP:2026-03-25");

    EXPECT_FALSE(run_id.empty());
    const auto runs = storage->list_runs("P_APP_PNL");
    EXPECT_NE(std::find(runs.begin(), runs.end(), run_id), runs.end());
}

TEST_F(AppWorkflowTest, RunsPnlExplainWorkflowWithoutImportedFactors) {
    TemporaryJsonFile previous_market_file(R"json(
        {
          "valuation_date": "2026-03-24",
          "quotes": [
            {
              "id": "AAPL",
              "instrument_type": "EquitySpot",
              "currency": "USD",
              "tenor": "SPOT",
              "value": 100.0
            }
          ]
        }
    )json");
    TemporaryJsonFile current_market_file(R"json(
        {
          "valuation_date": "2026-03-25",
          "quotes": [
            {
              "id": "AAPL",
              "instrument_type": "EquitySpot",
              "currency": "USD",
              "tenor": "SPOT",
              "value": 105.0
            }
          ]
        }
    )json");
    TemporaryJsonFile portfolio_file(R"json(
        {
          "portfolio_id": "P_APP_PNL_NO_FACTORS",
          "trades": [
            {
              "id": "equity_aapl",
              "asset_class": "equity",
              "type": "equity_spot",
              "currency": "USD",
              "direction": "long",
              "book": "BOOK:EQUITY",
              "strategy": "DELTA",
              "quantity": 10.0,
              "details": {
                "reference_price": 100.0,
                "underlier": "AAPL"
              }
            }
          ]
        }
    )json");

    app::QuantRiskPlatform platform(storage);
    platform.initialize();
    platform.import_market_snapshot(previous_market_file.string());
    platform.import_market_snapshot(current_market_file.string());
    platform.import_portfolio(portfolio_file.string());

    const auto run_id = platform.run_pnl_explain("P_APP_PNL_NO_FACTORS", "SNAP:2026-03-24", "SNAP:2026-03-25");

    EXPECT_FALSE(run_id.empty());
    const auto runs = storage->list_runs("P_APP_PNL_NO_FACTORS");
    EXPECT_NE(std::find(runs.begin(), runs.end(), run_id), runs.end());
}

TEST_F(AppWorkflowTest, ImportWorkflowsRejectMissingInputFiles) {
    app::QuantRiskPlatform platform(storage);
    platform.initialize();

    EXPECT_THROW({ platform.import_market_snapshot("missing_market_snapshot.json"); }, std::runtime_error);
    EXPECT_THROW({ platform.import_portfolio("missing_portfolio.json"); }, std::runtime_error);
    EXPECT_THROW({ platform.import_scenario_set("missing_scenario_set.json"); }, std::runtime_error);
}

TEST_F(AppWorkflowTest, JsonLoadersRejectMissingInputFiles) {
    EXPECT_THROW({ (void)io::load_market("missing_market_loader.json"); }, std::runtime_error);
    EXPECT_THROW({ (void)io::load_portfolio("missing_portfolio_loader.json"); }, std::runtime_error);
}

TEST_F(AppWorkflowTest, JsonLoadersParseValidInputsAndAttachDiagnostics) {
    TemporaryJsonFile market_file(R"json(
        {
          "valuation_date": "2026-03-24",
          "snapshot_id": "MKT_LOADER",
          "base_currency": "USD",
          "default_stale_after_days": 1,
          "quotes": [
            {
              "id": "USD_OIS_1Y",
              "instrument_type": "OIS",
              "currency": "USD",
              "tenor": "1Y",
              "value": 0.045,
              "source_name": "unit-test",
              "source_ts": "2026-03-20T17:00:00Z"
            }
          ],
          "curves": [
            {
              "id": {
                "currency": "USD",
                "family": "OIS"
              },
              "purpose": "OISDiscount",
              "quote_ids": [
                "USD_OIS_1Y"
              ],
              "day_count": "ACT360"
            }
          ],
          "diagnostics": [
            {
              "severity": "INFO",
              "code": "SOURCE_NOTE",
              "message": "Loaded by unit test",
              "quote_id": "USD_OIS_1Y"
            }
          ]
        }
    )json");

    const auto snapshot = io::load_market(market_file.string());

    EXPECT_EQ(snapshot.snapshot_id, "MKT_LOADER");
    EXPECT_EQ(snapshot.base_currency, domain::Currency::USD);
    ASSERT_EQ(snapshot.quotes.size(), 1U);
    EXPECT_EQ(snapshot.quotes.front().id, "USD_OIS_1Y");
    ASSERT_EQ(snapshot.curves.size(), 1U);
    EXPECT_EQ(snapshot.curves.front().id.family, "OIS");

    auto has_diagnostic = [&](const std::string& code) {
        return std::any_of(snapshot.diagnostics.begin(), snapshot.diagnostics.end(), [&](const auto& diagnostic) {
            return diagnostic.code == code;
        });
    };
    EXPECT_TRUE(has_diagnostic("SOURCE_NOTE"));
    EXPECT_TRUE(has_diagnostic("STALE_QUOTE"));

    TemporaryJsonFile portfolio_file(R"json(
        {
          "portfolio_id": "P_LOADER",
          "trades": [
            {
              "id": "loader_deposit",
              "asset_class": "rates",
              "type": "deposit",
              "currency": "USD",
              "direction": "lend",
              "book": "BOOK:UNIT:RATES",
              "strategy": "LIQUIDITY",
              "notional": 1000000.0,
              "start_date": "2026-03-24",
              "maturity_date": "2026-06-24",
              "details": {
                "rate": 0.0525
              }
            }
          ]
        }
    )json");

    const auto portfolio = io::load_portfolio(portfolio_file.string());

    EXPECT_EQ(portfolio.portfolio_id, "P_LOADER");
    ASSERT_EQ(portfolio.trades.size(), 1U);
    const auto deposit = std::dynamic_pointer_cast<domain::DepositTrade>(portfolio.trades.front());
    ASSERT_NE(deposit, nullptr);
    EXPECT_EQ(deposit->id, "loader_deposit");
    EXPECT_EQ(deposit->asset_class_type, domain::AssetClass::Rates);
    EXPECT_EQ(deposit->product_type, domain::ProductType::Deposit);
    EXPECT_DOUBLE_EQ(deposit->notional, 1000000.0);
    EXPECT_DOUBLE_EQ(deposit->deposit_rate, 0.0525);
}

TEST_F(AppWorkflowTest, ImportMarketSnapshotAppliesDefaultsAndQuoteMetadata) {
    TemporaryJsonFile market_file(R"json(
        {
          "valuation_date": "2026-03-24",
          "quotes": [
            {
              "id": "USD_OIS_2Y",
              "instrument_type": "OIS",
              "instrument_family": "ois",
              "index_family": "SOFR",
              "day_count": "ACT360",
              "calendar": "US",
              "bdc": "ModifiedFollowing",
              "settlement_days": 2,
              "tenor": "2Y",
              "value": 0.0415,
              "currency": "USD"
            }
          ]
        }
    )json");

    app::QuantRiskPlatform platform(storage);
    platform.initialize();
    platform.import_market_snapshot(market_file.string());

    const auto snapshots = storage->list_snapshots();
    ASSERT_EQ(snapshots.size(), 1U);
    EXPECT_EQ(snapshots.front(), "SNAP:2026-03-24");

    const auto snapshot = storage->load_market_snapshot("SNAP:2026-03-24");
    EXPECT_EQ(snapshot.base_currency, domain::Currency::USD);
    EXPECT_EQ(snapshot.curves.size(), 0U);
    ASSERT_EQ(snapshot.quotes.size(), 1U);

    const auto& quote = snapshot.quotes.front();
    EXPECT_EQ(quote.id, "USD_OIS_2Y");
    EXPECT_EQ(quote.instrument_type, domain::QuoteInstrumentType::OIS);
    EXPECT_EQ(quote.instrument_family, "ois");
    EXPECT_EQ(quote.index_family, "SOFR");
    EXPECT_EQ(quote.day_count, domain::DayCount::ACT360);
    EXPECT_EQ(quote.calendar, domain::BusinessCalendar::US);
    EXPECT_EQ(quote.bdc, domain::BusinessDayConvention::ModifiedFollowing);
    EXPECT_EQ(quote.settlement_days, 2);
    EXPECT_EQ(quote.tenor, "2Y");
    EXPECT_DOUBLE_EQ(quote.value, 0.0415);
}

TEST_F(AppWorkflowTest, ImportPortfolioAppliesDefaultsAndEconomicsFallbacks) {
    TemporaryJsonFile portfolio_file(R"json(
        {
          "portfolio_id": "P_DEFAULTS",
          "trades": [
            {
              "id": "deposit_defaults",
              "asset_class": "rates",
              "type": "deposit",
              "currency": "USD",
              "quantity": 1250000.0,
              "effective_date": "2026-03-24",
              "termination_date": "2026-06-24",
              "economics": {
                "rate": 0.0525
              }
            }
          ]
        }
    )json");

    app::QuantRiskPlatform platform(storage);
    platform.initialize();
    platform.import_portfolio(portfolio_file.string());

    const auto portfolios = storage->list_portfolios();
    ASSERT_EQ(portfolios.size(), 1U);
    EXPECT_EQ(portfolios.front(), "P_DEFAULTS");

    const auto trades = storage->load_trades("P_DEFAULTS");
    ASSERT_EQ(trades.size(), 1U);
    const auto deposit = std::dynamic_pointer_cast<domain::DepositTrade>(trades.front());
    ASSERT_NE(deposit, nullptr);
    EXPECT_EQ(deposit->id, "deposit_defaults");
    EXPECT_EQ(deposit->asset_class, "rates");
    EXPECT_EQ(deposit->type, "deposit");
    EXPECT_EQ(deposit->currency, "USD");
    EXPECT_EQ(deposit->direction, "unknown");
    EXPECT_EQ(deposit->start_date, "2026-03-24");
    EXPECT_EQ(deposit->maturity_date, "2026-06-24");
    EXPECT_DOUBLE_EQ(deposit->notional, 1250000.0);
    EXPECT_DOUBLE_EQ(deposit->deposit_rate, 0.0525);
}

TEST_F(AppWorkflowTest, ImportScenarioSetRejectsUndefinedScenarioFactorIds) {
    TemporaryJsonFile scenario_file(R"json(
        {
          "scenario_set_id": "bad_scenario_factor_set",
          "factors": [
            {
              "factor_id": "RF:EQ:AAPL:SPOT",
              "factor_type": "EquitySpot",
              "shock_measure": "Relative"
            }
          ],
          "bindings": [
            {
              "factor_id": "RF:EQ:AAPL:SPOT",
              "quote_id": "AAPL_SPOT",
              "shock_measure": "Relative"
            }
          ],
          "scenarios": {
            "BAD_FACTOR": {
              "factor_shocks": {
                "RF:EQ:MSFT:SPOT": -0.01
              }
            }
          }
        }
    )json");

    app::QuantRiskPlatform platform(storage);
    platform.initialize();

    EXPECT_THROW({ platform.import_scenario_set(scenario_file.string()); }, std::invalid_argument);
}

TEST_F(AppWorkflowTest, ImportScenarioSetPersistsDefinitionsBindingsAndShocks) {
    TemporaryJsonFile scenario_file(R"json(
        {
          "scenario_set_id": "mini_scenario_set",
          "name": "Mini Scenario Set",
          "factors": [
            {
              "factor_id": "RF:RATES:USD:OIS:2Y",
              "factor_type": "RateZero",
              "shock_measure": "BasisPoints",
              "currency": "USD",
              "curve_id": "USD_OIS",
              "tenor": "2Y",
              "quote_ids": [
                "USD_OIS_2Y"
              ],
              "description": "USD OIS two-year zero rate"
            }
          ],
          "bindings": [
            {
              "factor_id": "RF:RATES:USD:OIS:2Y",
              "quote_id": "USD_OIS_2Y",
              "shock_measure": "BasisPoints",
              "weight": 0.5,
              "transform": "rate",
              "selector_json": "{\"tenor\":\"2Y\"}"
            }
          ],
          "scenarios": {
            "USD_2Y_UP": {
              "factor_shocks": {
                "RF:RATES:USD:OIS:2Y": 10.0
              }
            }
          }
        }
    )json");

    app::QuantRiskPlatform platform(storage);
    platform.initialize();
    platform.import_scenario_set(scenario_file.string());

    const auto factors = storage->load_factor_definitions("unused_portfolio");
    ASSERT_EQ(factors.size(), 1U);
    EXPECT_EQ(factors.front().factor_id, "RF:RATES:USD:OIS:2Y");
    EXPECT_EQ(factors.front().factor_type, domain::FactorType::RateZero);
    EXPECT_EQ(factors.front().shock_measure, domain::ShockMeasure::BasisPoints);
    EXPECT_EQ(factors.front().currency, domain::Currency::USD);
    EXPECT_EQ(factors.front().curve_id, "USD_OIS");
    EXPECT_EQ(factors.front().tenor, "2Y");
    ASSERT_EQ(factors.front().quote_ids.size(), 1U);
    EXPECT_EQ(factors.front().quote_ids.front(), "USD_OIS_2Y");
    EXPECT_EQ(factors.front().description, "USD OIS two-year zero rate");

    const auto bindings = storage->load_factor_bindings({"RF:RATES:USD:OIS:2Y"});
    ASSERT_EQ(bindings.size(), 1U);
    EXPECT_EQ(bindings.front().factor_id, "RF:RATES:USD:OIS:2Y");
    EXPECT_EQ(bindings.front().quote_id, "USD_OIS_2Y");
    EXPECT_EQ(bindings.front().shock_measure, domain::ShockMeasure::BasisPoints);
    EXPECT_DOUBLE_EQ(bindings.front().weight, 0.5);
    EXPECT_EQ(bindings.front().transform, "rate");
    EXPECT_EQ(bindings.front().selector_json, "{\"tenor\":\"2Y\"}");

    const auto scenarios = storage->load_scenario_set("mini_scenario_set");
    ASSERT_EQ(scenarios.size(), 1U);
    EXPECT_DOUBLE_EQ(scenarios.at("USD_2Y_UP").at("RF:RATES:USD:OIS:2Y"), 10.0);
}

TEST_F(AppWorkflowTest, HistoricalVarRejectsImportedScenarioSetWithoutScenarios) {
    TemporaryJsonFile scenario_file(R"json(
        {
          "scenario_set_id": "empty_scenario_set",
          "factors": [
            {
              "factor_id": "RF:EQ:AAPL:SPOT",
              "factor_type": "EquitySpot",
              "shock_measure": "Relative",
              "currency": "USD",
              "quote_ids": [
                "AAPL_SPOT"
              ]
            }
          ],
          "bindings": [
            {
              "factor_id": "RF:EQ:AAPL:SPOT",
              "quote_id": "AAPL_SPOT",
              "shock_measure": "Relative"
            }
          ],
          "scenarios": {}
        }
    )json");

    app::QuantRiskPlatform platform(storage);
    platform.initialize();
    platform.import_market_snapshot(test::data_file({"market", "demo_market.json"}).string());
    platform.import_portfolio(test::data_file({"portfolios", "demo_portfolio.json"}).string());
    platform.import_scenario_set(scenario_file.string());

    EXPECT_THROW(
        { platform.run_historical_var("demo_portfolio", "SNAP:2026-03-24", "empty_scenario_set"); },
        std::runtime_error);
}

} // namespace qrp::testing
