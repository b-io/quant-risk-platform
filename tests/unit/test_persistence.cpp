#include <gtest/gtest.h>
#include <qrp/persistence/sqlite_storage_backend.hpp>
#include <filesystem>
#include <sqlite3.h>

namespace qrp::testing {

class PersistenceTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path = "test_qrp.db";
        storage = std::make_unique<persistence::SQLiteStorageBackend>(db_path);
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
    std::unique_ptr<persistence::SQLiteStorageBackend> storage;
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
    storage->store_trade("T1", "P1", "B1", "Rates", "Swap", "USD", 1000000.0, "2024-01-01", "2034-01-01", "Buy", "{\"notional\": 1000000}");

    auto trades = storage->load_trades("P1");
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].id, "T1");
    EXPECT_EQ(trades[0].asset_class, "Rates");
    EXPECT_EQ(trades[0].notional, 1000000.0);
}

TEST_F(PersistenceTest, MarketSnapshotQuotesRoundTrip) {
    storage->store_market_snapshot("S1", "2023-01-01", "USD", "[]");
    storage->store_market_quote("S1", "USD.SOFR.10Y", 0.035, "USD", "{\"tenor\": \"10Y\"}");
    storage->store_market_quote("S1", "USD.SOFR.5Y", 0.032, "USD", "{\"tenor\": \"5Y\"}");

    // Second snapshot with same quote IDs
    storage->store_market_snapshot("S2", "2023-01-02", "USD", "[]");
    storage->store_market_quote("S2", "USD.SOFR.10Y", 0.036, "USD", "{\"tenor\": \"10Y\"}");

    auto s1 = storage->load_market_snapshot("S1");
    EXPECT_EQ(s1.quotes.size(), 2);
    
    auto s2 = storage->load_market_snapshot("S2");
    EXPECT_EQ(s2.quotes.size(), 1);
    EXPECT_NEAR(s2.quotes[0].value, 0.036, 1e-10);
    EXPECT_EQ(s2.quotes[0].tenor, "10Y");
}

TEST_F(PersistenceTest, ValuationResultsPersistence) {
    storage->store_portfolio("P1", "P1", "USD");
    storage->store_market_snapshot("S1", "2023-01-01", "USD", "[]");
    std::string run_id = "RUN1";
    storage->store_analysis_run(run_id, "VALUATION", "P1", "S1");
    storage->store_valuation_result(run_id, "T1", 1234.56, "USD", "SUCCESS", "");

    auto results = storage->get_valuation_results(run_id);
    EXPECT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].trade_id, "T1");
    EXPECT_NEAR(results[0].npv, 1234.56, 1e-10);
    EXPECT_EQ(results[0].status, "SUCCESS");
}

} // namespace qrp::testing
