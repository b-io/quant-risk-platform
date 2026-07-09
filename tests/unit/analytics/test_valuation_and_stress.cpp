// Verifies valuation diagnostics, product support metadata, stress results, and PnL explain output.

#include <qrp/analytics/pnl_explain_service.hpp>
#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/product_pricing_registry.hpp>
#include <qrp/analytics/revaluation_session.hpp>
#include <qrp/analytics/risk_service.hpp>
#include <qrp/analytics/stress_engine.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/instruments/instrument_factory.hpp>
#include <qrp/io/json_loader.hpp>
#include <qrp/market/market_snapshot.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <test_paths.hpp>
#include <vector>

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

TEST(ValuationServiceTest, RegistryInstrumentBuildersRejectMismatchedConcreteTradeTypes) {
    qrp::domain::MarketSnapshot market_dto;
    market_dto.valuation_date = "2026-03-24";
    qrp::market::MarketSnapshot market(market_dto);
    qrp::analytics::PricingContext context(market.built_state());

    for (const auto product_type : qrp::domain::all_product_types()) {
        qrp::domain::Trade trade;
        trade.id = qrp::domain::to_string(product_type);
        trade.product_type = product_type;

        EXPECT_EQ(qrp::analytics::ProductPricingRegistry::create_instrument(trade, context), nullptr)
            << "Product type " << qrp::domain::to_string(product_type)
            << " should reject a non-concrete base Trade DTO";
    }
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

TEST(RevaluationSessionTest, RepricesFromQuoteUpdatesAndResets) {
    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "P1";
    portfolio.trades.push_back(make_equity_trade());

    qrp::domain::MarketSnapshot market_dto;
    market_dto.valuation_date = "2026-03-24";
    market_dto.quotes.push_back(make_equity_quote(100.0));

    qrp::analytics::RevaluationSession session(portfolio, market_dto);

    EXPECT_NEAR(session.total_npv(), 0.0, 1e-10);

    session.apply_quote_updates({{"AAPL", 110.0}});

    const auto updated_values = session.trade_values();
    ASSERT_TRUE(updated_values.contains("equity_aapl"));
    EXPECT_NEAR(updated_values.at("equity_aapl"), 100.0, 1e-10);
    EXPECT_NEAR(session.total_npv(), 100.0, 1e-10);

    session.reset();

    EXPECT_NEAR(session.total_npv(), 0.0, 1e-10);
}

TEST(RevaluationSessionTest, ReportsFailedCachedInstrumentConstruction) {
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

    qrp::analytics::RevaluationSession session(portfolio, market_dto);

    const auto results = session.price();

    ASSERT_EQ(results.size(), 1U);
    EXPECT_EQ(results[0].trade_id, "unsupported_trade");
    EXPECT_EQ(results[0].npv, 0.0);
    EXPECT_EQ(results[0].support_status, qrp::domain::SupportStatus::Unsupported);
    EXPECT_EQ(results[0].tags.at("error"), "Instrument construction failed");
    EXPECT_EQ(results[0].tags.at("status"), "unsupported");
}

TEST(RevaluationSessionTest, PreviewsQuoteUpdateImpactFromStructuralDependencies) {
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

    qrp::analytics::RevaluationSession session(portfolio, market_dto, {factor}, {binding});

    const auto preview = session.preview_quote_update_impact({{"AAPL", 110.0}});

    ASSERT_EQ(preview.updated_quote_ids, std::vector<std::string>({"AAPL"}));
    ASSERT_EQ(preview.potentially_affected_trade_count, 1U);
    ASSERT_EQ(preview.potentially_affected_trade_ids, std::vector<std::string>({"equity_aapl"}));
    ASSERT_EQ(preview.dependencies.size(), 1U);
    EXPECT_EQ(preview.dependencies[0].dependency_type, "market_quote_match");
    EXPECT_EQ(preview.dependencies[0].product_type, "equity_spot");
    EXPECT_EQ(preview.dependencies[0].quote_id, "AAPL");
    EXPECT_EQ(preview.dependencies[0].trade_id, "equity_aapl");
    EXPECT_NE(std::find(preview.dependencies[0].factor_ids.begin(),
                        preview.dependencies[0].factor_ids.end(),
                        "RF:EQ:AAPL:SPOT"),
              preview.dependencies[0].factor_ids.end());
    EXPECT_NEAR(session.total_npv(), 0.0, 1e-10);
}

TEST(RevaluationSessionTest, ExposesReadOnlyDependencyGraph) {
    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "P1";
    portfolio.trades.push_back(make_equity_trade());

    qrp::domain::MarketSnapshot market_dto;
    market_dto.valuation_date = "2026-03-24";
    market_dto.quotes.push_back(make_equity_quote(100.0));

    qrp::analytics::RevaluationSession session(portfolio, market_dto);

    const auto graph = session.dependency_graph();

    ASSERT_EQ(graph.dependency_count, 1U);
    EXPECT_EQ(graph.quote_count, 1U);
    EXPECT_EQ(graph.quote_ids, std::vector<std::string>({"AAPL"}));
    EXPECT_EQ(graph.trade_count, 1U);
    EXPECT_EQ(graph.trade_ids, std::vector<std::string>({"equity_aapl"}));
    ASSERT_EQ(graph.dependencies.size(), 1U);
    EXPECT_EQ(graph.dependencies[0].dependency_type, "market_quote_match");
    EXPECT_EQ(graph.dependencies[0].quote_id, "AAPL");
    EXPECT_EQ(graph.dependencies[0].trade_id, "equity_aapl");

    const auto quote_edges = session.dependencies_for_quote("AAPL");
    ASSERT_EQ(quote_edges.size(), 1U);
    EXPECT_EQ(quote_edges[0].trade_id, "equity_aapl");

    const auto trade_edges = session.dependencies_for_trade("equity_aapl");
    ASSERT_EQ(trade_edges.size(), 1U);
    EXPECT_EQ(trade_edges[0].quote_id, "AAPL");

    EXPECT_THROW(session.dependencies_for_quote("MISSING"), std::invalid_argument);
    EXPECT_THROW(session.dependencies_for_trade("missing_trade"), std::invalid_argument);
    EXPECT_NEAR(session.total_npv(), 0.0, 1e-10);
}

TEST(RevaluationSessionTest, RevaluesOnlyImpactedCandidatesAndRestoresState) {
    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "P1";
    portfolio.trades.push_back(make_equity_trade());

    qrp::domain::MarketSnapshot market_dto;
    market_dto.valuation_date = "2026-03-24";
    market_dto.quotes.push_back(make_equity_quote(100.0));

    qrp::analytics::RevaluationSession session(portfolio, market_dto);

    const auto report = session.revalue_quote_update_impact({{"AAPL", 110.0}}, 1e-10);

    ASSERT_EQ(report.updated_quote_ids, std::vector<std::string>({"AAPL"}));
    EXPECT_EQ(report.potentially_affected_trade_count, 1U);
    ASSERT_EQ(report.quote_moves.size(), 1U);
    EXPECT_EQ(report.quote_moves[0].quote_id, "AAPL");
    EXPECT_NEAR(report.quote_moves[0].before, 100.0, 1e-10);
    EXPECT_NEAR(report.quote_moves[0].after, 110.0, 1e-10);
    EXPECT_NEAR(report.quote_moves[0].restored, 100.0, 1e-10);
    EXPECT_NEAR(report.candidate_base_total_npv, 0.0, 1e-10);
    EXPECT_NEAR(report.candidate_shocked_total_npv, 100.0, 1e-10);
    EXPECT_NEAR(report.candidate_pnl, 100.0, 1e-10);
    EXPECT_NEAR(report.candidate_restored_total_npv, 0.0, 1e-10);
    ASSERT_EQ(report.trade_diffs.size(), 1U);
    EXPECT_EQ(report.trade_diffs[0].dependency_quote_ids, std::vector<std::string>({"AAPL"}));
    EXPECT_TRUE(report.trade_diffs[0].moved_above_tolerance);
    EXPECT_NEAR(report.trade_diffs[0].base_npv, 0.0, 1e-10);
    EXPECT_NEAR(report.trade_diffs[0].shocked_npv, 100.0, 1e-10);
    EXPECT_NEAR(report.trade_diffs[0].pnl, 100.0, 1e-10);
    EXPECT_NEAR(session.total_npv(), 0.0, 1e-10);
}

TEST(RevaluationSessionTest, DemoPortfolioImpactCoversMultiAssetDependencyTypes) {
    const auto market_path = qrp::test::data_file({"market", "demo_market.json"});
    const auto portfolio_path = qrp::test::data_file({"portfolios", "demo_portfolio.json"});
    auto market_dto = qrp::io::load_market(market_path.string());
    auto portfolio = qrp::io::load_portfolio(portfolio_path.string());

    std::map<std::string, double> quote_updates;
    for (const auto& quote : market_dto.quotes) {
        quote_updates[quote.id] = quote.value + 1.0e-4;
    }

    qrp::analytics::RevaluationSession session(portfolio, market_dto);

    const auto preview = session.preview_quote_update_impact(quote_updates);
    const auto cached_preview = session.preview_quote_update_impact(quote_updates);

    EXPECT_EQ(cached_preview.potentially_affected_trade_count, preview.potentially_affected_trade_count);
    EXPECT_EQ(preview.updated_quote_ids.size(), quote_updates.size());
    EXPECT_GT(preview.potentially_affected_trade_count, 25U);

    std::set<std::string> asset_classes;
    std::set<std::string> dependency_types;
    for (const auto& dependency : preview.dependencies) {
        asset_classes.insert(dependency.asset_class);
        dependency_types.insert(dependency.dependency_type);
    }

    EXPECT_TRUE(asset_classes.contains("commodity"));
    EXPECT_TRUE(asset_classes.contains("credit"));
    EXPECT_TRUE(asset_classes.contains("equity"));
    EXPECT_TRUE(asset_classes.contains("fx"));
    EXPECT_TRUE(asset_classes.contains("rates"));
    EXPECT_TRUE(dependency_types.contains("direct_quote"));
    EXPECT_TRUE(dependency_types.contains("discount_curve"));
    EXPECT_TRUE(dependency_types.contains("forecast_curve"));
    EXPECT_TRUE(dependency_types.contains("market_quote_match"));

    const auto report = session.revalue_quote_update_impact(quote_updates, 1.0e-10);

    EXPECT_EQ(report.updated_quote_ids.size(), quote_updates.size());
    EXPECT_EQ(report.potentially_affected_trade_count, preview.potentially_affected_trade_count);
    EXPECT_EQ(report.trade_diffs.size(), preview.potentially_affected_trade_count);
    EXPECT_NEAR(session.total_npv(), report.candidate_restored_total_npv, 1.0e-6);
    EXPECT_EQ(report.quote_moves.size(), quote_updates.size());
}

TEST(RevaluationSessionTest, RevaluesScenarioAndRestoresBaseState) {
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

    qrp::analytics::RevaluationSession session(portfolio, market_dto, {factor}, {binding});

    const auto report = session.revalue_scenario(scenario);

    EXPECT_EQ(report.scenario_name, "AAPL_UP_10_PERCENT");
    EXPECT_NEAR(report.base_total_npv, 0.0, 1e-10);
    EXPECT_NEAR(report.shocked_total_npv, 100.0, 1e-10);
    EXPECT_NEAR(report.scenario_pnl, 100.0, 1e-10);
    EXPECT_NEAR(report.restored_total_npv, 0.0, 1e-10);
    ASSERT_EQ(report.quote_moves.size(), 1U);
    EXPECT_EQ(report.quote_moves[0].quote_id, "AAPL");
    EXPECT_NEAR(report.quote_moves[0].before, 100.0, 1e-10);
    EXPECT_NEAR(report.quote_moves[0].after, 110.0, 1e-10);
    EXPECT_NEAR(report.quote_moves[0].restored, 100.0, 1e-10);
    EXPECT_NEAR(report.trade_pnls.at("equity_aapl"), 100.0, 1e-10);
    EXPECT_NEAR(session.total_npv(), 0.0, 1e-10);
}

TEST(RevaluationSessionTest, ResolvesScenarioImpactToTouchedQuoteDiffs) {
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

    qrp::analytics::RevaluationSession session(portfolio, market_dto, {factor}, {binding});

    const auto preview = session.preview_scenario_impact(scenario);
    ASSERT_EQ(preview.updated_quote_ids, std::vector<std::string>({"AAPL"}));
    ASSERT_EQ(preview.potentially_affected_trade_ids, std::vector<std::string>({"equity_aapl"}));

    const auto report = session.revalue_scenario_impact(scenario, 1e-10);

    EXPECT_EQ(report.scenario_name, "AAPL_UP_10_PERCENT");
    ASSERT_EQ(report.updated_quote_ids, std::vector<std::string>({"AAPL"}));
    ASSERT_EQ(report.trade_diffs.size(), 1U);
    EXPECT_EQ(report.trade_diffs[0].trade_id, "equity_aapl");
    EXPECT_TRUE(report.trade_diffs[0].moved_above_tolerance);
    EXPECT_NEAR(report.trade_diffs[0].pnl, 100.0, 1e-10);
    EXPECT_NEAR(report.candidate_pnl, 100.0, 1e-10);
    EXPECT_NEAR(session.total_npv(), 0.0, 1e-10);
}

TEST(RevaluationSessionTest, RejectsUnknownQuoteUpdatesAndMissingScenarioBindings) {
    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "P1";
    portfolio.trades.push_back(make_equity_trade());

    qrp::domain::MarketSnapshot market_dto;
    market_dto.valuation_date = "2026-03-24";
    market_dto.quotes.push_back(make_equity_quote(100.0));

    qrp::analytics::RevaluationSession session(portfolio, market_dto);

    EXPECT_THROW(session.apply_quote_updates({{"MISSING", 110.0}}), std::invalid_argument);

    qrp::market::ScenarioDefinition scenario;
    scenario.name = "NO_BINDINGS";
    scenario.factor_shocks["RF:EQ:AAPL:SPOT"] = 0.10;

    EXPECT_THROW(session.preview_quote_update_impact({{"MISSING", 110.0}}), std::invalid_argument);
    EXPECT_THROW(session.apply_scenario(scenario), std::runtime_error);
    EXPECT_THROW(session.revalue_scenario(scenario), std::runtime_error);
    EXPECT_THROW(session.preview_scenario_impact(scenario), std::runtime_error);
    EXPECT_THROW(session.revalue_scenario_impact(scenario), std::runtime_error);
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
    EXPECT_NEAR(result.residual_pnl, 0.0, 1e-10);
    EXPECT_TRUE(result.reconciliation_passed);
    EXPECT_FALSE(result.cashflow_extraction.extraction_supported);
    EXPECT_EQ(result.cashflow_extraction.support_status, qrp::domain::SupportStatus::Unsupported);
    EXPECT_EQ(result.diagnostics.at("cashflow_extraction_supported"), "false");
    EXPECT_EQ(result.diagnostics.at("cashflow_support_status"), "unsupported");

    std::map<std::string, qrp::analytics::PnlExplainComponent> components;
    for (const auto& component : result.components) {
        components[component.component_id] = component;
    }

    ASSERT_EQ(components.size(), 8);
    EXPECT_EQ(components.at("carry").component_type, qrp::analytics::PnlExplainComponentType::Carry);
    EXPECT_EQ(components.at("cash").component_type, qrp::analytics::PnlExplainComponentType::Cash);
    EXPECT_EQ(components.at("fx_translation").component_type, qrp::analytics::PnlExplainComponentType::FxTranslation);
    EXPECT_EQ(components.at("market_move").component_type, qrp::analytics::PnlExplainComponentType::MarketMove);
    EXPECT_EQ(components.at("model_change").component_type, qrp::analytics::PnlExplainComponentType::ModelChange);
    EXPECT_EQ(components.at("residual").component_type, qrp::analytics::PnlExplainComponentType::Residual);
    EXPECT_EQ(components.at("roll_down").component_type, qrp::analytics::PnlExplainComponentType::RollDown);
    EXPECT_EQ(components.at("trade_activity").component_type, qrp::analytics::PnlExplainComponentType::TradeActivity);
    EXPECT_NEAR(components.at("market_move").amount, 100.0, 1e-10);
    EXPECT_EQ(components.at("cash").tags.at("extraction_supported"), "false");
    EXPECT_EQ(components.at("residual").tags.at("reconciliation_passed"), "true");
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
