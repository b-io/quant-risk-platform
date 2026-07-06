// Verifies Monte Carlo reset semantics and path-level risk result calculations.

#include <qrp/analytics/monte_carlo_engine.hpp>
#include <qrp/domain/factors.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/portfolio.hpp>
#include <qrp/market/market_snapshot.hpp>

#include <gtest/gtest.h>

#include <memory>
#include <ql/math/matrix.hpp>
#include <stdexcept>

using namespace qrp::analytics;
using namespace qrp::domain;

namespace {

MarketSnapshot make_single_quote_market() {
    MarketSnapshot market;
    market.valuation_date = "2024-03-24";

    MarketQuote quote;
    quote.id = "USD_OIS_1M";
    quote.instrument_type = QuoteInstrumentType::OIS;
    quote.currency = Currency::USD;
    quote.tenor = "1M";
    quote.value = 0.03;
    market.quotes.push_back(quote);

    return market;
}

FactorDefinition make_rate_factor() {
    FactorDefinition factor;
    factor.factor_id = "RF:RATES:USD:OIS:1M";
    factor.factor_type = FactorType::RateZero;
    factor.currency = Currency::USD;
    factor.tenor = "1M";
    return factor;
}

FactorBinding make_rate_binding() {
    FactorBinding binding;
    binding.factor_id = "RF:RATES:USD:OIS:1M";
    binding.quote_id = "USD_OIS_1M";
    binding.shock_measure = ShockMeasure::Absolute;
    return binding;
}

} // namespace

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

TEST(MonteCarloTest, ReturnsEmptyResultWhenPathCountIsNonPositive) {
    MonteCarloConfig config;
    config.num_paths = 0;

    const auto result = MonteCarloEngine::run_simulation({}, {}, {}, {}, QuantLib::Matrix(), config);

    EXPECT_TRUE(result.portfolio_values.empty());
    EXPECT_TRUE(result.portfolio_pnls.empty());
    EXPECT_TRUE(result.traces.empty());
}

TEST(MonteCarloTest, ValidatesFactorsBindingsAndCovarianceShapeBeforeSimulation) {
    Portfolio portfolio;
    const auto market = make_single_quote_market();
    const auto factor = make_rate_factor();
    const auto binding = make_rate_binding();

    MonteCarloConfig config;
    config.num_paths = 1;

    EXPECT_THROW(
        { MonteCarloEngine::run_simulation(portfolio, market, {}, {binding}, QuantLib::Matrix(1, 1, 0.0), config); },
        std::runtime_error);

    EXPECT_THROW(
        { MonteCarloEngine::run_simulation(portfolio, market, {factor}, {}, QuantLib::Matrix(1, 1, 0.0), config); },
        std::runtime_error);

    config.require_bindings = false;
    EXPECT_THROW(
        { MonteCarloEngine::run_simulation(portfolio, market, {factor}, {}, QuantLib::Matrix(2, 2, 0.0), config); },
        std::runtime_error);
}

TEST(MonteCarloTest, RecordsUnsupportedTradesAndStillProducesPathDiagnostics) {
    Portfolio portfolio;
    auto unsupported_trade = std::make_shared<Trade>();
    unsupported_trade->id = "unsupported_trade";
    unsupported_trade->asset_class = "unknown";
    unsupported_trade->type = "unsupported";
    unsupported_trade->currency = "USD";
    portfolio.trades.push_back(unsupported_trade);

    MonteCarloConfig config;
    config.num_paths = 4;
    config.seed = 7;

    const auto result = MonteCarloEngine::run_simulation(portfolio,
                                                         make_single_quote_market(),
                                                         {make_rate_factor()},
                                                         {make_rate_binding()},
                                                         QuantLib::Matrix(1, 1, 0.0),
                                                         config);

    EXPECT_EQ(result.num_trades_total, 1);
    EXPECT_EQ(result.num_trades_priced_t0, 0);
    EXPECT_EQ(result.num_trades_failed_t0, 1);
    ASSERT_TRUE(result.construction_errors.contains("unsupported_trade"));
    EXPECT_EQ(result.portfolio_values.size(), 4U);
    EXPECT_EQ(result.portfolio_pnls.size(), 4U);
    ASSERT_EQ(result.traces.size(), 3U);
    EXPECT_EQ(result.traces.front().path_index, 0);
    EXPECT_EQ(result.traces.front().num_priced, 0);
    EXPECT_EQ(result.traces.front().num_failed, 0);
    EXPECT_TRUE(result.traces.front().quote_before_after.contains("USD_OIS_1M"));
}
