// Verifies Monte Carlo reset semantics and path-level risk result calculations.

#include <qrp/analytics/monte_carlo_engine.hpp>
#include <qrp/domain/factors.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/portfolio.hpp>
#include <qrp/market/market_snapshot.hpp>

#include <gtest/gtest.h>
#include <ql/math/matrix.hpp>

using namespace qrp::analytics;
using namespace qrp::domain;

TEST(MonteCarloTest, TestAgedHorizonResetSemantics) {
    Portfolio portfolio;
    auto trade = std::make_shared<VanillaSwapTrade>();
    trade->id = "T1";
    trade->asset_class = "IR";
    trade->type = "vanilla_swap";
    trade->currency = "USD";
    trade->notional = 1000000.0;
    trade->start_date = "2024-03-24";
    trade->maturity_date = "2029-03-24";
    trade->fixed_rate = 0.03;
    trade->floating_index = "USD_SOFR_3M";
    portfolio.trades.push_back(trade);

    MarketSnapshot base_dto;
    base_dto.valuation_date = "2024-03-24";

    // Add fixing for the floating index (2 business days before start_date)
    base_dto.fixings["USD_SOFR_3M"]["2024-03-21"] = 0.03;
    base_dto.fixings["IBOR_3M"]["2024-03-21"] = 0.03;
    base_dto.fixings["USD_LIBOR_3M"]["2024-03-21"] = 0.03;

    MarketQuote q1;
    q1.id = "USD_OIS_1M";
    q1.instrument_type = QuoteInstrumentType::OIS;
    q1.currency = Currency::USD;
    q1.tenor = "1M";
    q1.value = 0.03;
    base_dto.quotes.push_back(q1);

    CurveSpec spec;
    spec.id = {Currency::USD, "OIS"};
    spec.purpose = CurvePurpose::OISDiscount;
    spec.quote_ids = {"USD_OIS_1M"};
    base_dto.curves.push_back(spec);

    std::vector<FactorDefinition> factors;
    FactorDefinition f1;
    f1.factor_id = "F1";
    factors.push_back(f1);

    std::vector<FactorBinding> bindings;
    FactorBinding b1;
    b1.factor_id = "F1";
    b1.quote_id = "USD_OIS_1M";
    b1.shock_measure = ShockMeasure::Absolute;
    bindings.push_back(b1);

    QuantLib::Matrix cov(1, 1, 0.0001); // 100 bps vol

    MonteCarloConfig config;
    config.num_paths = 2;
    config.horizon_days = 10;
    config.mode = MonteCarloMode::AgedHorizonRevaluation;
    config.seed = 42;

    MonteCarloEngine engine;
    auto result = engine.run_simulation(portfolio, base_dto, factors, bindings, cov, config);

    ASSERT_EQ(result.portfolio_values.size(), 2);

    // Both paths should reset to the same frozen-aged baseline before path shocks.
    ASSERT_GE(result.traces.size(), 2);

    double val_before_path0 = result.traces[0].quote_before_after.at("USD_OIS_1M").first;
    double val_before_path1 = result.traces[1].quote_before_after.at("USD_OIS_1M").first;

    EXPECT_NEAR(val_before_path0, val_before_path1, 1e-10);
}
