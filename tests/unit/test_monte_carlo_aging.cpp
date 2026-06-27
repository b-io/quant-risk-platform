#include <gtest/gtest.h>
#include <qrp/analytics/monte_carlo_engine.hpp>
#include <qrp/domain/portfolio.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/factors.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <ql/math/matrix.hpp>
#include <ql/settings.hpp>
#include <fmt/format.h>

using namespace qrp::analytics;
using namespace qrp::domain;

class MonteCarloAgingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a simple swap trade
        trade = std::make_shared<VanillaSwapTrade>();
        trade->id = "SWAP_1";
        trade->asset_class = "Rates";
        trade->type = "vanilla_swap";
        trade->currency = "USD";
        trade->notional = 1000000.0;
        trade->start_date = "2024-03-20";
        trade->maturity_date = "2029-03-20";
        trade->direction = "pay_fixed";
        trade->fixed_rate = 0.03;
        trade->floating_index = "USD_LIBOR_3M";
        portfolio.trades.push_back(trade);

        // Setup base market
        base_market.valuation_date = "2024-03-24";

        base_market.fixings["USD_LIBOR_3M"]["2024-03-18"] = 0.031;
        base_market.fixings["USD_LIBOR_3M"]["2024-03-20"] = 0.032;
        // 2024-03-24 is a Sunday, removed invalid fixing.
        
        MarketQuote q1;
        q1.id = "USD_OIS_1M";
        q1.instrument_type = QuoteInstrumentType::IRS; // Changed to IRS for better testing of swaps
        q1.currency = Currency::USD;
        q1.tenor = "1M";
        q1.value = 0.03;
        base_market.quotes.push_back(q1);

        MarketQuote q2;
        q2.id = "USD_OIS_5Y";
        q2.instrument_type = QuoteInstrumentType::IRS;
        q2.currency = Currency::USD;
        q2.tenor = "5Y";
        q2.value = 0.035;
        base_market.quotes.push_back(q2);

        CurveSpec spec;
        spec.id = {Currency::USD, "OIS"};
        spec.purpose = CurvePurpose::OISDiscount;
        spec.quote_ids = {"USD_OIS_1M", "USD_OIS_5Y"};
        base_market.curves.push_back(spec);
        
        // Also add IBOR curve for forecasting (same as OIS for simplicity)
        CurveSpec spec_ibor;
        spec_ibor.id = {Currency::USD, "IBOR_3M"};
        spec_ibor.purpose = CurvePurpose::Forward3M;
        spec_ibor.quote_ids = {"USD_OIS_1M", "USD_OIS_5Y"};
        base_market.curves.push_back(spec_ibor);

        // Map quotes for easier access if needed
        for (const auto& q : base_market.quotes) {
            quote_map[q.id] = q;
        }

        // Factors and bindings
        FactorDefinition f1;
        f1.factor_id = "USD_IR_OIS";
        factors.push_back(f1);

        FactorBinding b1;
        b1.factor_id = "USD_IR_OIS";
        b1.quote_id = "USD_OIS_1M";
        b1.shock_measure = ShockMeasure::Absolute;
        bindings.push_back(b1);

        covariance = QuantLib::Matrix(1, 1, 0.0001);
    }

    Portfolio portfolio;
    std::shared_ptr<VanillaSwapTrade> trade;
    MarketSnapshot base_market;
    std::map<std::string, MarketQuote> quote_map;
    std::vector<FactorDefinition> factors;
    std::vector<FactorBinding> bindings;
    QuantLib::Matrix covariance;

    void PrintResults(const SimulationResult& result) {
        fmt::print("Simulation Result:\n");
        fmt::print("  Base PV: {}\n", result.base_portfolio_value);
        fmt::print("  Total Trades: {}\n", result.num_trades_total);
        fmt::print("  Priced t0: {}\n", result.num_trades_priced_t0);
        fmt::print("  Failed t0: {}\n", result.num_trades_failed_t0);
        fmt::print("  Priced tH: {}\n", result.num_trades_priced_tH);
        fmt::print("  Expired tH: {}\n", result.num_trades_expired_tH);
        fmt::print("  Failed tH: {}\n", result.num_trades_failed_tH);
        
        if (!result.construction_errors.empty()) {
            fmt::print("  Errors:\n");
            for (auto const& [id, err] : result.construction_errors) {
                fmt::print("    {}: {}\n", id, err);
            }
        }
    }
};

TEST_F(MonteCarloAgingTest, AgedHorizonProducesNonZeroPV) {
    MonteCarloConfig config;
    config.num_paths = 10;
    config.horizon_days = 30; // 30 days into the future
    config.mode = MonteCarloMode::AgedHorizonRevaluation;
    config.seed = 42;

    MonteCarloEngine engine;
    SimulationResult result = engine.run_simulation(portfolio, base_market, factors, bindings, covariance, config);

    PrintResults(result);

    // Verify that base PV is not zero
    EXPECT_GT(std::abs(result.base_portfolio_value), 1.0) << "Base portfolio value should be non-zero";

    // Verify that the frozen-aged PV is not zero (this is what is reportedly failing)
    // We check the first trace if available
    if (!result.traces.empty()) {
        EXPECT_GT(std::abs(result.traces[0].portfolio_value_frozen_aged), 1.0) 
            << "Frozen-aged portfolio value should be non-zero at tH";
    }

    // Verify that the final portfolio values in simulation are non-zero
    for (size_t i = 0; i < result.portfolio_values.size(); ++i) {
        EXPECT_GT(std::abs(result.portfolio_values[i]), 1.0) 
            << "Portfolio value at path " << i << " should be non-zero";
    }
}
