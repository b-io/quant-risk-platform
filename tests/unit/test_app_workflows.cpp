// Verifies the application facade against the checked-in demo workflow data.

#include "test_paths.hpp"

#include <qrp/app/quant_risk_platform.hpp>
#include <qrp/persistence/sqlite_storage_backend.hpp>

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

namespace qrp::testing {
namespace {

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

} // namespace

TEST_F(AppWorkflowTest, RunsDemoImportAnalyticsAndReports) {
    app::QuantRiskPlatform platform(storage);
    platform.initialize();

    platform.import_market_snapshot(test::data_file({"market", "demo_market.json"}).string());
    platform.import_portfolio(test::data_file({"portfolios", "demo_portfolio.json"}).string());
    platform.import_scenario_set(test::data_file({"scenarios", "demo_scenarios.json"}).string());

    const auto valuation_run = platform.run_valuation("demo_portfolio", "SNAP:2026-03-24");
    const auto valuation_records = storage->get_valuation_results(valuation_run);
    ASSERT_EQ(valuation_records.size(), 31U);

    const auto risk_run = platform.run_risk("demo_portfolio", "SNAP:2026-03-24");
    EXPECT_FALSE(risk_run.empty());

    const auto var_run = platform.run_historical_var("demo_portfolio", "SNAP:2026-03-24", "demo_factor_scenarios");
    EXPECT_FALSE(var_run.empty());

    EXPECT_NO_THROW(platform.compare_runs(valuation_run, valuation_run));
    EXPECT_NO_THROW(platform.get_run_report(valuation_run));
    EXPECT_NO_THROW(platform.get_run_report("missing_run"));
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

} // namespace qrp::testing
