#include <gtest/gtest.h>
#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/stress_engine.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <memory>

namespace {

std::shared_ptr<qrp::domain::EquitySpotTrade> make_equity_trade() {
    auto trade = std::make_shared<qrp::domain::EquitySpotTrade>();
    trade->id = "equity_aapl";
    trade->asset_class = "equity";
    trade->type = "equity_spot";
    trade->currency = "USD";
    trade->direction = "long";
    trade->book = "BOOK:EQUITY";
    trade->strategy = "DELTA";
    trade->quantity = 10.0;
    trade->reference_price = 100.0;
    trade->underlier = "AAPL";
    return trade;
}

} // namespace

TEST(ValuationServiceTest, ReportsFailedInstrumentConstruction) {
    auto trade = std::make_shared<qrp::domain::Trade>();
    trade->id = "unsupported_trade";
    trade->asset_class = "unknown";
    trade->type = "unsupported";
    trade->currency = "USD";
    trade->direction = "long";
    trade->book = "BOOK";
    trade->strategy = "TEST";

    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "P1";
    portfolio.trades.push_back(trade);

    qrp::domain::MarketSnapshot market_dto;
    market_dto.valuation_date = "2026-03-24";
    qrp::market::MarketSnapshot market(market_dto);
    qrp::analytics::PricingContext context(market.built_state());

    auto results = qrp::analytics::ValuationService::price_portfolio(portfolio, context);

    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].trade_id, "unsupported_trade");
    EXPECT_EQ(results[0].npv, 0.0);
    EXPECT_EQ(results[0].tags.at("status"), "failed");
    EXPECT_EQ(results[0].tags.at("error"), "Instrument construction failed");
}

TEST(StressEngineTest, UsesAdjustedTradeNpvForEquitySpot) {
    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "P1";
    portfolio.trades.push_back(make_equity_trade());

    qrp::domain::MarketSnapshot market_dto;
    market_dto.valuation_date = "2026-03-24";
    market_dto.quotes.push_back({"AAPL", qrp::domain::QuoteInstrumentType::Future, qrp::domain::Currency::USD, "SPOT", 100.0});

    qrp::domain::FactorDefinition factor;
    factor.factor_id = "RF:EQ:AAPL:SPOT";
    factor.factor_type = qrp::domain::FactorType::EquitySpot;
    factor.shock_measure = qrp::domain::ShockMeasure::Relative;
    factor.currency = qrp::domain::Currency::USD;

    qrp::domain::FactorBinding binding;
    binding.factor_id = factor.factor_id;
    binding.quote_id = "AAPL";
    binding.shock_measure = qrp::domain::ShockMeasure::Relative;
    binding.weight = 1.0;

    qrp::market::ScenarioDefinition scenario;
    scenario.name = "AAPL_UP_10_PERCENT";
    scenario.factor_shocks[factor.factor_id] = 0.10;

    auto results = qrp::analytics::StressEngine::run_historical_stress(
        portfolio,
        market_dto,
        {scenario},
        {factor},
        {binding});

    ASSERT_EQ(results.size(), 1);
    EXPECT_NEAR(results[0].trade_pnls.at("equity_aapl"), 100.0, 1e-10);
    EXPECT_NEAR(results[0].total_pnl, 100.0, 1e-10);
}
