// Verifies valuation diagnostics, product support metadata, stress results, and PnL explain output.

#include <qrp/analytics/pnl_explain_service.hpp>
#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/product_pricing_registry.hpp>
#include <qrp/analytics/risk_service.hpp>
#include <qrp/analytics/stress_engine.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/instruments/instrument_factory.hpp>
#include <qrp/market/market_snapshot.hpp>

#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

qrp::domain::MarketQuote make_equity_quote(double value) {
    qrp::domain::MarketQuote quote;
    quote.id = "AAPL";
    quote.instrument_type = qrp::domain::QuoteInstrumentType::Future;
    quote.currency = qrp::domain::Currency::USD;
    quote.tenor = "SPOT";
    quote.value = value;
    quote.underlier = "AAPL";
    return quote;
}

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
    EXPECT_EQ(results[0].support_status, qrp::domain::SupportStatus::Unsupported);
    EXPECT_EQ(results[0].product_type, qrp::domain::ProductType::Unknown);
    EXPECT_EQ(results[0].tags.at("status"), "unsupported");
    EXPECT_EQ(results[0].tags.at("error"), "Instrument construction failed");
}

TEST(ValuationServiceTest, ReportsProductSupportMetadataForPricedTrades) {
    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "P1";
    portfolio.trades.push_back(make_equity_trade());

    qrp::domain::MarketSnapshot market_dto;
    market_dto.valuation_date = "2026-03-24";
    market_dto.quotes.push_back(make_equity_quote(110.0));

    qrp::market::MarketSnapshot market(market_dto);
    qrp::analytics::PricingContext context(market.built_state());

    auto results = qrp::analytics::ValuationService::price_portfolio(portfolio, context);

    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].asset_class, qrp::domain::AssetClass::Equity);
    EXPECT_EQ(results[0].product_type, qrp::domain::ProductType::EquitySpot);
    EXPECT_EQ(results[0].support_status, qrp::domain::SupportStatus::Supported);
    EXPECT_EQ(results[0].tags.at("product_type"), "equity_spot");
    EXPECT_EQ(results[0].tags.at("status"), "supported");
    EXPECT_NE(results[0].model_name.find("QuantLib::Stock"), std::string::npos);
}

TEST(ValuationServiceTest, EquityFactoryReturnsNullWhenUnderlierQuoteIsMissing) {
    qrp::domain::MarketSnapshot market_dto;
    market_dto.valuation_date = "2026-03-24";
    qrp::market::MarketSnapshot market(market_dto);
    qrp::analytics::PricingContext context(market.built_state());

    qrp::domain::EquitySpotTrade trade;
    trade.underlier = "AAPL";

    EXPECT_FALSE(qrp::instruments::EquityInstrumentFactory::create_equity_spot(trade, context));
}

TEST(ValuationServiceTest, DeclaresSupportProfileForEveryProductType) {
    for (const auto product_type : qrp::domain::all_product_types()) {
        qrp::domain::Trade trade;
        trade.id = qrp::domain::to_string(product_type);
        trade.product_type = product_type;

        const auto support = qrp::analytics::ValuationService::support_profile(trade);

        EXPECT_EQ(support.product_type, product_type);
        EXPECT_NE(support.status, qrp::domain::SupportStatus::Failed);
        EXPECT_FALSE(support.model_name.empty());
        EXPECT_FALSE(support.reason.empty());
        if (product_type == qrp::domain::ProductType::Unknown) {
            EXPECT_EQ(support.asset_class, qrp::domain::AssetClass::Unknown);
        } else {
            EXPECT_NE(support.asset_class, qrp::domain::AssetClass::Unknown);
        }
    }
}

TEST(ValuationServiceTest, RegistryFallbacksUseTradeAndProductTaxonomy) {
    qrp::domain::Trade explicit_asset_class;
    explicit_asset_class.id = "explicit_asset_class";
    explicit_asset_class.asset_class = "unknown";
    explicit_asset_class.asset_class_type = qrp::domain::AssetClass::Rates;
    explicit_asset_class.product_type = qrp::domain::ProductType::Deposit;

    const auto explicit_support = qrp::analytics::ProductPricingRegistry::support_profile(explicit_asset_class);

    EXPECT_EQ(explicit_support.asset_class, qrp::domain::AssetClass::Rates);
    EXPECT_EQ(explicit_support.product_type, qrp::domain::ProductType::Deposit);

    qrp::domain::Trade parsed_asset_class;
    parsed_asset_class.id = "parsed_asset_class";
    parsed_asset_class.asset_class = "FX";
    parsed_asset_class.product_type = qrp::domain::ProductType::FxForward;

    const auto parsed_support = qrp::analytics::ProductPricingRegistry::support_profile(parsed_asset_class);

    EXPECT_EQ(parsed_support.asset_class, qrp::domain::AssetClass::FX);
    EXPECT_EQ(parsed_support.product_type, qrp::domain::ProductType::FxForward);

    qrp::domain::Trade product_inferred_asset_class;
    product_inferred_asset_class.id = "product_inferred_asset_class";
    product_inferred_asset_class.product_type = qrp::domain::ProductType::Cds;

    const auto inferred_support = qrp::analytics::ProductPricingRegistry::support_profile(product_inferred_asset_class);

    EXPECT_EQ(inferred_support.asset_class, qrp::domain::AssetClass::Credit);
    EXPECT_EQ(inferred_support.product_type, qrp::domain::ProductType::Cds);

    qrp::domain::Trade trade;
    trade.id = "fallback_trade";
    trade.asset_class = "credit";
    trade.product_type = qrp::domain::ProductType::Unknown;
    trade.trade_type = qrp::domain::TradeType::CreditBond;

    const auto support = qrp::analytics::ProductPricingRegistry::support_profile(trade);

    EXPECT_EQ(support.asset_class, qrp::domain::AssetClass::Credit);
    EXPECT_EQ(support.product_type, qrp::domain::ProductType::CreditBond);
    EXPECT_EQ(support.status, qrp::domain::SupportStatus::Supported);

    qrp::domain::MarketSnapshot previous_market;
    previous_market.valuation_date = "2026-03-24";
    qrp::domain::MarketSnapshot current_market = previous_market;
    current_market.valuation_date = "2026-03-25";

    const auto cashflows =
        qrp::analytics::ProductPricingRegistry::extract_realized_cashflows(trade, previous_market, current_market);

    EXPECT_FALSE(cashflows.extraction_supported);
    EXPECT_FALSE(cashflows.model_name.empty());
    EXPECT_EQ(cashflows.tags.at("product_support_status"), "supported");

    qrp::domain::FxOptionTrade written_option;
    written_option.direction = "WRITTEN";

    const auto pricing_profile = qrp::analytics::ProductPricingRegistry::pricing_profile(written_option);

    EXPECT_DOUBLE_EQ(pricing_profile.multiplier, -1.0);
}

TEST(StressEngineTest, UsesAdjustedTradeNpvForEquitySpot) {
    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "P1";
    portfolio.trades.push_back(make_equity_trade());

    qrp::domain::MarketSnapshot market_dto;
    market_dto.valuation_date = "2026-03-24";
    market_dto.quotes.push_back(make_equity_quote(100.0));

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

    auto results =
        qrp::analytics::StressEngine::run_historical_stress(portfolio, market_dto, {scenario}, {factor}, {binding});

    ASSERT_EQ(results.size(), 1);
    EXPECT_NEAR(results[0].trade_pnls.at("equity_aapl"), 100.0, 1e-10);
    EXPECT_NEAR(results[0].total_pnl, 100.0, 1e-10);
}

TEST(PnlExplainServiceTest, PreservesValuationDiagnosticsAndComponentLines) {
    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "P1";
    portfolio.trades.push_back(make_equity_trade());

    qrp::domain::MarketSnapshot previous_market;
    previous_market.valuation_date = "2026-03-24";
    previous_market.quotes.push_back(make_equity_quote(100.0));

    qrp::domain::MarketSnapshot current_market = previous_market;
    current_market.valuation_date = "2026-03-25";
    current_market.quotes[0].value = 110.0;

    const auto results = qrp::analytics::PnlExplainService::explain_pnl(portfolio, previous_market, current_market);

    ASSERT_EQ(results.size(), 1);
    const auto& result = results[0];
    EXPECT_EQ(result.trade_id, "equity_aapl");
    EXPECT_TRUE(result.prev_valuation_available);
    EXPECT_TRUE(result.curr_valuation_available);
    EXPECT_TRUE(result.rolled_valuation_available);
    EXPECT_EQ(result.prev_valuation.trade_id, "equity_aapl");
    EXPECT_EQ(result.curr_valuation.support_status, qrp::domain::SupportStatus::Supported);
    EXPECT_EQ(result.rolled_valuation.model_name, result.prev_valuation.model_name);
    EXPECT_NEAR(result.prev_npv, 0.0, 1e-10);
    EXPECT_NEAR(result.curr_npv, 100.0, 1e-10);
    EXPECT_NEAR(result.total_pnl, 100.0, 1e-10);
    EXPECT_NEAR(result.carry_pnl, 0.0, 1e-10);
    EXPECT_NEAR(result.cash_pnl, 0.0, 1e-10);
    EXPECT_NEAR(result.market_move_pnl, 100.0, 1e-10);
    EXPECT_NEAR(result.residual, 0.0, 1e-10);
    EXPECT_FALSE(result.cashflow_extraction.extraction_supported);
    EXPECT_EQ(result.cashflow_extraction.support_status, qrp::domain::SupportStatus::Unsupported);
    EXPECT_EQ(result.diagnostics.at("cashflow_extraction_supported"), "false");
    EXPECT_EQ(result.diagnostics.at("cashflow_support_status"), "unsupported");

    std::map<std::string, qrp::analytics::PnlExplainComponent> components;
    for (const auto& component : result.components) {
        components[component.component_id] = component;
    }

    ASSERT_EQ(components.size(), 4);
    EXPECT_EQ(components.at("carry").component_type, qrp::analytics::PnlExplainComponentType::Carry);
    EXPECT_EQ(components.at("cash").component_type, qrp::analytics::PnlExplainComponentType::Cash);
    EXPECT_EQ(components.at("market_move").component_type, qrp::analytics::PnlExplainComponentType::MarketMove);
    EXPECT_EQ(components.at("residual").component_type, qrp::analytics::PnlExplainComponentType::Residual);
    EXPECT_NEAR(components.at("market_move").amount, 100.0, 1e-10);
    EXPECT_EQ(components.at("cash").tags.at("extraction_supported"), "false");
}

TEST(ValuationServiceTest, RegistryPricingProfilesCoverDirectionalMultipliers) {
    qrp::domain::InterestRateFutureTrade future;
    future.direction = "sold";
    future.quantity = 2.0;
    future.contract_size = 2500.0;
    future.reference_price = 95.0;

    const auto future_profile = qrp::analytics::ProductPricingRegistry::pricing_profile(future);

    EXPECT_DOUBLE_EQ(future_profile.multiplier, -5000.0);
    EXPECT_DOUBLE_EQ(future_profile.additive_npv, 475000.0);

    qrp::domain::FxOptionTrade long_option;
    long_option.direction = "long";
    EXPECT_DOUBLE_EQ(qrp::analytics::ProductPricingRegistry::pricing_profile(long_option).multiplier, 1.0);
}

TEST(RiskServiceTest, RejectsMissingFactorConfiguration) {
    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "P1";
    portfolio.trades.push_back(make_equity_trade());

    qrp::domain::MarketSnapshot market_dto;
    market_dto.valuation_date = "2026-03-24";
    market_dto.quotes.push_back(make_equity_quote(110.0));

    EXPECT_THROW({ qrp::analytics::RiskService::compute_risk(portfolio, market_dto, {}, {}); }, std::runtime_error);
}

TEST(StressEngineTest, RejectsMissingFactorConfiguration) {
    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "P1";
    portfolio.trades.push_back(make_equity_trade());

    qrp::domain::MarketSnapshot market_dto;
    market_dto.valuation_date = "2026-03-24";
    market_dto.quotes.push_back(make_equity_quote(110.0));

    qrp::market::ScenarioDefinition scenario;
    scenario.name = "NO_FACTORS";

    EXPECT_THROW(
        { qrp::analytics::StressEngine::run_historical_stress(portfolio, market_dto, {scenario}, {}, {}); },
        std::runtime_error);
}
