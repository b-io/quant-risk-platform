// Verifies PnL explain factor attribution, cash extraction, and residual reconciliation.

#include <qrp/analytics/pnl_explain_service.hpp>
#include <qrp/analytics/product_pricing_registry.hpp>
#include <qrp/domain/factors.hpp>

#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

qrp::domain::MarketQuote make_equity_quote(const std::string& quote_id, double value) {
    qrp::domain::MarketQuote quote;
    quote.id = quote_id;
    quote.instrument_type = qrp::domain::QuoteInstrumentType::EquitySpot;
    quote.quote_type = qrp::domain::QuoteType::EquitySpot;
    quote.currency = qrp::domain::Currency::USD;
    quote.tenor = "SPOT";
    quote.value = value;
    quote.underlier = quote_id;
    quote.risk_factor_id = qrp::domain::make_equity_spot_factor_id(quote_id);
    return quote;
}

std::shared_ptr<qrp::domain::EquitySpotTrade> make_equity_trade(const std::string& underlier) {
    auto trade = std::make_shared<qrp::domain::EquitySpotTrade>();
    trade->id = "equity_" + underlier;
    trade->asset_class = "equity";
    trade->asset_class_type = qrp::domain::AssetClass::Equity;
    trade->type = "equity_spot";
    trade->product_type = qrp::domain::ProductType::EquitySpot;
    trade->currency = "USD";
    trade->direction = "long";
    trade->book = "BOOK:EQUITY";
    trade->strategy = "DELTA";
    trade->quantity = 10.0;
    trade->reference_price = 100.0;
    trade->underlier = underlier;
    return trade;
}

qrp::domain::FactorDefinition make_equity_factor(const std::string& underlier) {
    qrp::domain::FactorDefinition factor;
    factor.factor_id = qrp::domain::make_equity_spot_factor_id(underlier);
    factor.factor_type = qrp::domain::FactorType::EquitySpot;
    factor.shock_measure = qrp::domain::ShockMeasure::Relative;
    factor.currency = qrp::domain::Currency::USD;
    factor.quote_ids = {underlier};
    return factor;
}

qrp::domain::FactorDefinition
make_factor(std::string factor_id, qrp::domain::FactorType factor_type, std::vector<std::string> quote_ids = {}) {
    qrp::domain::FactorDefinition factor;
    factor.factor_id = std::move(factor_id);
    factor.factor_type = factor_type;
    factor.shock_measure = qrp::domain::ShockMeasure::Relative;
    factor.currency = qrp::domain::Currency::USD;
    factor.quote_ids = std::move(quote_ids);
    return factor;
}

qrp::domain::FactorBinding make_binding(std::string factor_id, std::string quote_id) {
    qrp::domain::FactorBinding binding;
    binding.factor_id = std::move(factor_id);
    binding.quote_id = std::move(quote_id);
    binding.shock_measure = qrp::domain::ShockMeasure::Relative;
    binding.weight = 1.0;
    return binding;
}

qrp::domain::FactorBinding make_equity_binding(const qrp::domain::FactorDefinition& factor,
                                               const std::string& quote_id) {
    qrp::domain::FactorBinding binding;
    binding.factor_id = factor.factor_id;
    binding.quote_id = quote_id;
    binding.shock_measure = qrp::domain::ShockMeasure::Relative;
    binding.weight = 1.0;
    return binding;
}

std::map<std::string, qrp::analytics::PnlExplainComponent>
components_by_id(const qrp::analytics::PnlExplainResult& result) {
    std::map<std::string, qrp::analytics::PnlExplainComponent> components;
    for (const auto& component : result.components) {
        components[component.component_id] = component;
    }
    return components;
}

} // namespace

TEST(PnlExplainServiceTest, TagsDefinedFactorGroupsAcrossAssetClasses) {
    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "P_GROUPS";
    portfolio.trades.push_back(make_equity_trade("AAPL"));

    qrp::domain::MarketSnapshot previous_market;
    previous_market.valuation_date = "2026-03-24";
    previous_market.quotes.push_back(make_equity_quote("AAPL", 100.0));

    qrp::domain::MarketSnapshot current_market = previous_market;
    current_market.valuation_date = "2026-03-25";

    const std::vector<qrp::domain::FactorDefinition> factors = {
        make_factor("CUSTOM:MACRO", qrp::domain::FactorType::Custom, {"AAPL"}),
        make_factor("RF:COM:WTI:SPOT", qrp::domain::FactorType::CommoditySpot, {"AAPL"}),
        make_factor("RF:CREDIT:ACME:SPREAD:5Y", qrp::domain::FactorType::CreditSpread, {"AAPL"}),
        make_factor("RF:EQVOL:AAPL:1Y:ATM", qrp::domain::FactorType::Volatility, {"AAPL"}),
        make_factor("RF:FX:EURUSD:SPOT", qrp::domain::FactorType::FXSpot, {"AAPL"}),
        make_factor("RF:RATES:USD:OIS:1Y", qrp::domain::FactorType::RateZero, {"AAPL"}),
    };
    std::vector<qrp::domain::FactorBinding> bindings;
    for (const auto& factor : factors) {
        bindings.push_back(make_binding(factor.factor_id, "AAPL"));
    }

    const auto results =
        qrp::analytics::PnlExplainService::explain_pnl(portfolio, previous_market, current_market, factors, bindings);

    ASSERT_EQ(results.size(), 1U);
    EXPECT_TRUE(results.front().reconciliation_passed);

    const auto components = components_by_id(results.front());
    EXPECT_EQ(components.at("market_move:CUSTOM:MACRO").risk_factor_group, "custom");
    EXPECT_EQ(components.at("market_move:RF:COM:WTI:SPOT").risk_factor_group, "commodity");
    EXPECT_EQ(components.at("market_move:RF:CREDIT:ACME:SPREAD:5Y").risk_factor_group, "credit");
    EXPECT_EQ(components.at("market_move:RF:EQVOL:AAPL:1Y:ATM").risk_factor_group, "volatility");
    EXPECT_EQ(components.at("market_move:RF:FX:EURUSD:SPOT").risk_factor_group, "fx");
    EXPECT_EQ(components.at("market_move:RF:RATES:USD:OIS:1Y").risk_factor_group, "rates");
}

TEST(PnlExplainServiceTest, AttributesMarketMoveBySequentialFactorRevaluation) {
    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "P_FACTOR";
    portfolio.trades.push_back(make_equity_trade("AAPL"));

    qrp::domain::MarketSnapshot previous_market;
    previous_market.valuation_date = "2026-03-24";
    previous_market.quotes.push_back(make_equity_quote("AAPL", 100.0));

    qrp::domain::MarketSnapshot current_market = previous_market;
    current_market.valuation_date = "2026-03-25";
    current_market.quotes[0].value = 110.0;

    const auto factor = make_equity_factor("AAPL");
    const auto binding = make_equity_binding(factor, "AAPL");
    const auto results =
        qrp::analytics::PnlExplainService::explain_pnl(portfolio, previous_market, current_market, {factor}, {binding});

    ASSERT_EQ(results.size(), 1U);
    const auto& result = results[0];
    EXPECT_EQ(result.asset_class, "equity");
    EXPECT_EQ(result.book, "BOOK:EQUITY");
    EXPECT_EQ(result.product_type, "equity_spot");
    EXPECT_EQ(result.diagnostics.at("factor_explain_enabled"), "true");
    EXPECT_TRUE(result.reconciliation_passed);
    EXPECT_NEAR(result.prev_npv, 0.0, 1.0e-10);
    EXPECT_NEAR(result.curr_npv, 100.0, 1.0e-10);
    EXPECT_NEAR(result.total_pnl, 100.0, 1.0e-10);
    EXPECT_NEAR(result.market_move_pnl, 100.0, 1.0e-10);
    EXPECT_NEAR(result.residual_pnl, 0.0, 1.0e-10);

    const auto components = components_by_id(result);
    const auto factor_component_id = "market_move:" + factor.factor_id;
    ASSERT_TRUE(components.contains(factor_component_id));
    EXPECT_EQ(components.at(factor_component_id).component_type, qrp::analytics::PnlExplainComponentType::MarketMove);
    EXPECT_EQ(components.at(factor_component_id).factor_id, factor.factor_id);
    EXPECT_EQ(components.at(factor_component_id).risk_factor_group, "equity");
    EXPECT_EQ(components.at(factor_component_id).tags.at("factor_explain_method"), "sequential_full_revaluation");
    EXPECT_NEAR(components.at(factor_component_id).amount, 100.0, 1.0e-10);
}

TEST(PnlExplainServiceTest, FallsBackToFactorIdGroupAndReportsMissingBoundQuotes) {
    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "P_FALLBACK";
    portfolio.trades.push_back(make_equity_trade("AAPL"));

    qrp::domain::MarketSnapshot previous_market;
    previous_market.valuation_date = "2026-03-24";
    previous_market.quotes.push_back(make_equity_quote("AAPL", 100.0));

    qrp::domain::MarketSnapshot current_market = previous_market;
    current_market.valuation_date = "2026-03-25";

    const std::vector<qrp::domain::FactorDefinition> factors = {
        make_factor("DEFINED:BUT:UNBOUND", qrp::domain::FactorType::Custom),
    };
    const std::vector<qrp::domain::FactorBinding> bindings = {
        make_binding("CUSTOM:UNKNOWN", "AAPL"),
        make_binding("RF:COM:BRENT:SPOT", "AAPL"),
        make_binding("RF:COM:BRENT:SPOT", "MISSING_A"),
        make_binding("RF:COM:BRENT:SPOT", "MISSING_B"),
        make_binding("RF:CREDIT:ACME:RECOVERY:SPOT", "AAPL"),
        make_binding("RF:EQ:MSFT:SPOT", "AAPL"),
        make_binding("RF:FX:EURUSD:SPOT", "AAPL"),
        make_binding("RF:RATES:USD:SOFR:5Y", "AAPL"),
        make_binding("SYNTH_VOL_FACTOR", "AAPL"),
    };

    const auto results =
        qrp::analytics::PnlExplainService::explain_pnl(portfolio, previous_market, current_market, factors, bindings);

    ASSERT_EQ(results.size(), 1U);
    const auto components = components_by_id(results.front());
    EXPECT_EQ(components.at("market_move:CUSTOM:UNKNOWN").risk_factor_group, "custom");
    EXPECT_EQ(components.at("market_move:RF:COM:BRENT:SPOT").risk_factor_group, "commodity");
    EXPECT_EQ(components.at("market_move:RF:CREDIT:ACME:RECOVERY:SPOT").risk_factor_group, "credit");
    EXPECT_EQ(components.at("market_move:RF:EQ:MSFT:SPOT").risk_factor_group, "equity");
    EXPECT_EQ(components.at("market_move:RF:FX:EURUSD:SPOT").risk_factor_group, "fx");
    EXPECT_EQ(components.at("market_move:RF:RATES:USD:SOFR:5Y").risk_factor_group, "rates");
    EXPECT_EQ(components.at("market_move:SYNTH_VOL_FACTOR").risk_factor_group, "volatility");

    const auto& partial_component = components.at("market_move:RF:COM:BRENT:SPOT");
    EXPECT_EQ(partial_component.support_status, qrp::domain::SupportStatus::PartiallySupported);
    EXPECT_EQ(partial_component.tags.at("applied_quote_count"), "1");
    EXPECT_EQ(partial_component.tags.at("missing_quote_ids"), "MISSING_A,MISSING_B");
}

TEST(PnlExplainServiceTest, ReportsResidualWhenObservedQuoteMoveIsUnbound) {
    qrp::domain::Portfolio portfolio;
    portfolio.portfolio_id = "P_RESIDUAL";
    portfolio.trades.push_back(make_equity_trade("AAPL"));

    qrp::domain::MarketSnapshot previous_market;
    previous_market.valuation_date = "2026-03-24";
    previous_market.quotes.push_back(make_equity_quote("AAPL", 100.0));
    previous_market.quotes.push_back(make_equity_quote("MSFT", 200.0));

    qrp::domain::MarketSnapshot current_market = previous_market;
    current_market.valuation_date = "2026-03-25";
    current_market.quotes[0].value = 110.0;

    const auto unrelated_factor = make_equity_factor("MSFT");
    const auto unrelated_binding = make_equity_binding(unrelated_factor, "MSFT");
    const auto results = qrp::analytics::PnlExplainService::explain_pnl(portfolio,
                                                                        previous_market,
                                                                        current_market,
                                                                        {unrelated_factor},
                                                                        {unrelated_binding});

    ASSERT_EQ(results.size(), 1U);
    const auto& result = results[0];
    EXPECT_TRUE(result.reconciliation_passed);
    EXPECT_NEAR(result.market_move_pnl, 0.0, 1.0e-10);
    EXPECT_NEAR(result.residual_pnl, 100.0, 1.0e-10);

    const auto components = components_by_id(result);
    ASSERT_TRUE(components.contains("residual"));
    EXPECT_EQ(components.at("residual").risk_factor_group, "unexplained");
    EXPECT_EQ(components.at("residual").tags.at("asset_class"), "equity");
    EXPECT_EQ(components.at("residual").tags.at("book"), "BOOK:EQUITY");
    EXPECT_EQ(components.at("residual").tags.at("currency"), "USD");
}

TEST(PnlExplainCashflowExtractorTest, RealizesDepositMaturityCashBetweenSnapshots) {
    qrp::domain::DepositTrade deposit;
    deposit.id = "deposit_usd";
    deposit.asset_class = "rates";
    deposit.asset_class_type = qrp::domain::AssetClass::Rates;
    deposit.product_type = qrp::domain::ProductType::Deposit;
    deposit.currency = "USD";
    deposit.direction = "lend";
    deposit.notional = 1'000'000.0;
    deposit.start_date = "2026-03-24";
    deposit.maturity_date = "2026-03-25";
    deposit.deposit_rate = 0.0360;

    qrp::domain::MarketSnapshot previous_market;
    previous_market.valuation_date = "2026-03-24";
    qrp::domain::MarketSnapshot current_market;
    current_market.valuation_date = "2026-03-25";

    const auto result =
        qrp::analytics::ProductPricingRegistry::extract_realized_cashflows(deposit, previous_market, current_market);

    EXPECT_TRUE(result.extraction_supported);
    EXPECT_EQ(result.support_status, qrp::domain::SupportStatus::Supported);
    EXPECT_EQ(result.model_name, "QRP::DepositCashflowExtractor/ACT360");
    EXPECT_EQ(result.tags.at("event_count"), "1");
    EXPECT_NEAR(result.realized_cash_pnl, 1'000'100.0, 1.0e-10);
}

TEST(PnlExplainCashflowExtractorTest, ReportsNoDepositCashflowWhenMaturityIsOutsideInterval) {
    qrp::domain::DepositTrade deposit;
    deposit.id = "deposit_usd";
    deposit.product_type = qrp::domain::ProductType::Deposit;
    deposit.currency = "USD";
    deposit.direction = "lend";
    deposit.notional = 1'000'000.0;
    deposit.start_date = "2026-03-24";
    deposit.maturity_date = "2026-03-26";
    deposit.deposit_rate = 0.0360;

    qrp::domain::MarketSnapshot previous_market;
    previous_market.valuation_date = "2026-03-24";
    qrp::domain::MarketSnapshot current_market;
    current_market.valuation_date = "2026-03-25";

    const auto result =
        qrp::analytics::ProductPricingRegistry::extract_realized_cashflows(deposit, previous_market, current_market);

    EXPECT_TRUE(result.extraction_supported);
    EXPECT_EQ(result.support_status, qrp::domain::SupportStatus::Supported);
    EXPECT_EQ(result.tags.at("event_count"), "0");
    EXPECT_NEAR(result.realized_cash_pnl, 0.0, 1.0e-10);
}

TEST(PnlExplainCashflowExtractorTest, FailsDepositCashflowExtractionForNonDepositTrade) {
    qrp::domain::Trade trade;
    trade.id = "misclassified_trade";
    trade.product_type = qrp::domain::ProductType::Deposit;

    qrp::domain::MarketSnapshot previous_market;
    previous_market.valuation_date = "2026-03-24";
    qrp::domain::MarketSnapshot current_market;
    current_market.valuation_date = "2026-03-25";

    const auto result =
        qrp::analytics::ProductPricingRegistry::extract_realized_cashflows(trade, previous_market, current_market);

    EXPECT_TRUE(result.extraction_supported);
    EXPECT_EQ(result.support_status, qrp::domain::SupportStatus::Failed);
    EXPECT_EQ(result.status_message, "Deposit cashflow extractor received a non-deposit trade");
}
