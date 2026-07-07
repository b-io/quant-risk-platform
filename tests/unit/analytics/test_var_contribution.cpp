// Verifies historical VaR and Expected Shortfall contribution analytics.

#include <qrp/analytics/var_contribution_service.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>

namespace qrp::testing {
namespace {

std::shared_ptr<domain::Trade> make_trade(const std::string& trade_id,
                                          const std::string& book,
                                          const std::string& strategy,
                                          const std::string& currency,
                                          const std::string& asset_class) {
    auto trade = std::make_shared<domain::Trade>();
    trade->id = trade_id;
    trade->book = book;
    trade->strategy = strategy;
    trade->currency = currency;
    trade->asset_class = asset_class;
    trade->type = "unit_test_trade";
    return trade;
}

const analytics::VarContribution& find_contribution(const analytics::VarContributionReport& report,
                                                    const std::string& aggregation_type,
                                                    const std::string& aggregation_key) {
    const auto it = std::find_if(report.contributions.begin(), report.contributions.end(), [&](const auto& row) {
        return row.aggregation_type == aggregation_type && row.aggregation_key == aggregation_key;
    });
    if (it == report.contributions.end()) {
        throw std::runtime_error("Missing contribution row");
    }
    return *it;
}

} // namespace

TEST(VarContributionServiceTest, CalculatesHistoricalContributionsAcrossReportingGroups) {
    domain::Portfolio portfolio;
    portfolio.portfolio_id = "P_VAR";
    portfolio.trades = {
        make_trade("T_RATES", "BOOK:RATES", "CARRY", "USD", "rates"),
        make_trade("T_EQUITY", "BOOK:EQUITY", "DELTA", "USD", "equity"),
    };

    const std::vector<analytics::HistoricalScenarioPnl> scenarios = {
        {"RISK_OFF",
         -100.0,
         {{"T_RATES", -70.0}, {"T_EQUITY", -30.0}},
         {{"RF:RATES:USD:OIS:2Y", -0.01}, {"RF:EQ:AAPL:SPOT", -0.02}}},
        {"RATES_DRIFT", -40.0, {{"T_RATES", -10.0}, {"T_EQUITY", -30.0}}, {{"RF:RATES:USD:OIS:2Y", -0.005}}},
        {"RELIEF_RALLY", 20.0, {{"T_RATES", 15.0}, {"T_EQUITY", 5.0}}, {{"RF:EQ:AAPL:SPOT", 0.01}}},
    };

    const auto report = analytics::VarContributionService::calculate_historical_contributions(portfolio, scenarios);

    EXPECT_EQ(report.method, "HISTORICAL");
    EXPECT_EQ(report.scenario_count, 3);
    EXPECT_EQ(report.tail_scenario_count, 1);
    EXPECT_EQ(report.var_scenario_name, "RISK_OFF");
    EXPECT_DOUBLE_EQ(report.var_value, 100.0);
    EXPECT_DOUBLE_EQ(report.expected_shortfall, 100.0);

    const auto& rates_trade = find_contribution(report, "trade", "T_RATES");
    EXPECT_DOUBLE_EQ(rates_trade.var_contribution, 70.0);
    EXPECT_DOUBLE_EQ(rates_trade.expected_shortfall_contribution, 70.0);
    EXPECT_DOUBLE_EQ(rates_trade.portfolio_var_share, 0.7);
    EXPECT_DOUBLE_EQ(rates_trade.standalone_var, 70.0);
    EXPECT_DOUBLE_EQ(rates_trade.incremental_var, 70.0);
    EXPECT_EQ(rates_trade.sign_convention, "positive_loss");

    const auto& equity_book = find_contribution(report, "book", "BOOK:EQUITY");
    EXPECT_DOUBLE_EQ(equity_book.var_contribution, 30.0);
    EXPECT_DOUBLE_EQ(equity_book.incremental_expected_shortfall, 30.0);

    const auto& rates_factor = find_contribution(report, "risk_factor", "RF:RATES:USD:OIS:2Y");
    EXPECT_NEAR(rates_factor.var_contribution, 100.0 / 3.0, 1e-10);
    EXPECT_NEAR(rates_factor.expected_shortfall_contribution, 100.0 / 3.0, 1e-10);

    const auto& equity_factor = find_contribution(report, "risk_factor", "RF:EQ:AAPL:SPOT");
    EXPECT_NEAR(equity_factor.var_contribution, 200.0 / 3.0, 1e-10);
    EXPECT_NEAR(equity_factor.portfolio_var_share, 2.0 / 3.0, 1e-10);

    EXPECT_NEAR(report.var_component_residuals.at("trade"), 0.0, 1e-10);
    EXPECT_NEAR(report.expected_shortfall_component_residuals.at("book"), 0.0, 1e-10);
    EXPECT_NEAR(report.var_component_residuals.at("risk_factor"), 0.0, 1e-10);

    const auto top_trades = analytics::VarContributionService::top_var_contributors(report, "trade", 1);
    ASSERT_EQ(top_trades.size(), 1U);
    EXPECT_EQ(top_trades.front().aggregation_key, "T_RATES");

    const auto top_es = analytics::VarContributionService::top_expected_shortfall_contributors(report, "trade", 2);
    ASSERT_EQ(top_es.size(), 2U);
    EXPECT_EQ(top_es.front().aggregation_key, "T_RATES");
}

TEST(VarContributionServiceTest, ValidatesHistoricalContributionInputs) {
    const domain::Portfolio portfolio;

    EXPECT_THROW(
        { analytics::VarContributionService::calculate_historical_contributions(portfolio, {}); },
        std::runtime_error);

    const std::vector<analytics::HistoricalScenarioPnl> scenarios = {
        {"ONLY_PATH", -1.0, {}, {}},
    };
    EXPECT_THROW(
        { analytics::VarContributionService::calculate_historical_contributions(portfolio, scenarios, 1.0); },
        std::runtime_error);
    EXPECT_THROW(
        { analytics::VarContributionService::calculate_historical_contributions(portfolio, scenarios, 0.0); },
        std::runtime_error);
}

} // namespace qrp::testing
