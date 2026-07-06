// Exercises sample-data risk calculation through the public loading and analytics services.

#include "test_paths.hpp"

#include <qrp/analytics/risk_service.hpp>
#include <qrp/domain/factors.hpp>
#include <qrp/io/json_loader.hpp>
#include <qrp/market/market_snapshot.hpp>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

std::map<std::string, double> material_bucketed_risk(const std::map<std::string, double>& values, double tolerance) {
    std::map<std::string, double> material;
    for (const auto& [bucket, value] : values) {
        if (std::abs(value) > tolerance) {
            material[bucket] = value;
        }
    }
    return material;
}

std::set<std::string> json_object_keys(const nlohmann::json& object) {
    std::set<std::string> keys;
    for (auto it = object.begin(); it != object.end(); ++it) {
        keys.insert(it.key());
    }
    return keys;
}

std::set<std::string> map_keys(const std::map<std::string, double>& values) {
    std::set<std::string> keys;
    for (const auto& entry : values) {
        keys.insert(entry.first);
    }
    return keys;
}

void load_factor_configuration(const std::filesystem::path& scenario_path,
                               std::vector<qrp::domain::FactorDefinition>& factors,
                               std::vector<qrp::domain::FactorBinding>& bindings) {
    std::ifstream scenario_stream(scenario_path);
    ASSERT_TRUE(scenario_stream.is_open()) << "Unable to open " << scenario_path.string();

    nlohmann::json scenario_data;
    scenario_stream >> scenario_data;

    for (const auto& factor_json : scenario_data.at("factors")) {
        qrp::domain::FactorDefinition factor;
        factor.factor_id = factor_json.at("factor_id").get<std::string>();
        factor.factor_type = qrp::domain::parse_factor_type(factor_json.at("factor_type").get<std::string>());
        factor.shock_measure = qrp::domain::parse_shock_measure(factor_json.at("shock_measure").get<std::string>());
        factor.currency = qrp::domain::from_string(factor_json.value("currency", "UNKNOWN"));
        factor.curve_id = factor_json.value("curve_id", "");
        factor.tenor = factor_json.value("tenor", "");
        factor.description = factor_json.value("description", "");
        if (factor_json.contains("quote_ids")) {
            factor.quote_ids = factor_json.at("quote_ids").get<std::vector<std::string>>();
        }
        factors.push_back(factor);
    }

    for (const auto& binding_json : scenario_data.at("bindings")) {
        qrp::domain::FactorBinding binding;
        binding.factor_id = binding_json.at("factor_id").get<std::string>();
        binding.quote_id = binding_json.at("quote_id").get<std::string>();
        binding.shock_measure = qrp::domain::parse_shock_measure(binding_json.at("shock_measure").get<std::string>());
        binding.weight = binding_json.value("weight", 1.0);
        binding.transform = binding_json.value("transform", "");
        binding.selector_json = binding_json.value("selector_json", "");
        bindings.push_back(binding);
    }
}

} // namespace

TEST(RiskIntegrationTest, ComputeRiskSamplePortfolio) {
    const auto market_path = qrp::test::data_file({"market", "demo_market.json"});
    const auto portfolio_path = qrp::test::data_file({"portfolios", "demo_portfolio.json"});
    const auto golden_path = qrp::test::data_file({"regression", "demo_golden.json"});
    const auto scenario_path = qrp::test::data_file({"scenarios", "demo_scenarios.json"});

    auto market_dto = qrp::io::load_market(market_path.string());
    auto portfolio_dto = qrp::io::load_portfolio(portfolio_path.string());

    std::ifstream golden_stream(golden_path);
    ASSERT_TRUE(golden_stream.is_open()) << "Unable to open " << golden_path.string();

    nlohmann::json golden;
    golden_stream >> golden;

    std::vector<qrp::domain::FactorDefinition> factors;
    std::vector<qrp::domain::FactorBinding> bindings;
    load_factor_configuration(scenario_path, factors, bindings);

    auto risk_results = qrp::analytics::RiskService::compute_risk(portfolio_dto, market_dto, factors, bindings);

    const auto& expected_risk = golden.at("risk");
    const double tolerance = expected_risk.value("tolerance", 1.0);
    std::set<std::string> expected_trade_ids;
    for (auto it = expected_risk.begin(); it != expected_risk.end(); ++it) {
        if (it.key() != "tolerance") {
            expected_trade_ids.insert(it.key());
        }
    }

    ASSERT_EQ(risk_results.size(), expected_trade_ids.size());
    std::map<std::string, qrp::analytics::RiskResult> by_trade;
    for (const auto& res : risk_results) {
        EXPECT_FALSE(res.trade_id.empty());
        EXPECT_TRUE(expected_trade_ids.contains(res.trade_id)) << "Unexpected risk result for " << res.trade_id;
        by_trade[res.trade_id] = res;
    }

    for (const auto& [trade_id, expected] : expected_risk.items()) {
        if (trade_id == "tolerance") {
            continue;
        }
        ASSERT_TRUE(by_trade.contains(trade_id)) << "Missing risk result for " << trade_id;
        const auto& actual = by_trade.at(trade_id);

        EXPECT_NEAR(actual.cs01, expected.at("cs01").get<double>(), tolerance) << trade_id;
        EXPECT_NEAR(actual.fx_delta, expected.at("fx_delta").get<double>(), tolerance) << trade_id;
        EXPECT_NEAR(actual.fx_vega, expected.at("fx_vega").get<double>(), tolerance) << trade_id;
        EXPECT_NEAR(actual.pv01, expected.at("pv01").get<double>(), tolerance) << trade_id;

        const auto material_actual = material_bucketed_risk(actual.bucketed_risk, tolerance);
        const auto& expected_buckets = expected.at("bucketed_risk");
        EXPECT_EQ(map_keys(material_actual), json_object_keys(expected_buckets)) << trade_id;
        for (const auto& [bucket, expected_value] : expected_buckets.items()) {
            ASSERT_TRUE(material_actual.contains(bucket)) << trade_id << " missing bucket " << bucket;
            EXPECT_NEAR(material_actual.at(bucket), expected_value.get<double>(), tolerance)
                << trade_id << "/" << bucket;
        }
    }
}
