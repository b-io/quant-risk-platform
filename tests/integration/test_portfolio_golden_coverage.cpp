// Verifies portfolio-backed golden coverage across the supported product set.

#include "test_paths.hpp"

#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/io/json_loader.hpp>
#include <qrp/market/market_snapshot.hpp>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

std::set<std::string> json_string_set(const nlohmann::json& values) {
    std::set<std::string> result;
    for (const auto& value : values) {
        result.insert(value.get<std::string>());
    }
    return result;
}

std::set<std::string> json_object_key_set(const nlohmann::json& object) {
    std::set<std::string> result;
    for (const auto& [key, value] : object.items()) {
        static_cast<void>(value);
        result.insert(key);
    }
    return result;
}

void verify_portfolio_fixture(const std::filesystem::path& manifest_path) {
    std::ifstream manifest_stream(manifest_path);
    ASSERT_TRUE(manifest_stream.is_open()) << "Unable to open " << manifest_path.string();

    nlohmann::json manifest;
    manifest_stream >> manifest;

    const auto data_root = qrp::test::find_data_dir();
    const auto market_path = data_root / std::filesystem::path(manifest.at("market_path").get<std::string>());
    const auto portfolio_path = data_root / std::filesystem::path(manifest.at("portfolio_path").get<std::string>());

    auto market_dto = qrp::io::load_market(market_path.string());
    qrp::market::MarketSnapshot market(market_dto);
    qrp::analytics::PricingContext context(market.built_state());
    auto portfolio = qrp::io::load_portfolio(portfolio_path.string());

    ASSERT_EQ(portfolio.portfolio_id, manifest.at("portfolio_id").get<std::string>());

    std::map<std::string, std::set<std::string>> actual_books;
    for (const auto& trade : portfolio.trades) {
        actual_books[trade->book].insert(trade->id);
    }

    std::ifstream portfolio_stream(portfolio_path);
    ASSERT_TRUE(portfolio_stream.is_open()) << "Unable to open " << portfolio_path.string();
    nlohmann::json portfolio_json;
    portfolio_stream >> portfolio_json;
    ASSERT_TRUE(portfolio_json.contains("books")) << portfolio_path.string();

    std::map<std::string, std::set<std::string>> declared_books;
    for (const auto& book : portfolio_json.at("books")) {
        declared_books[book.at("book_id").get<std::string>()] = json_string_set(book.at("trades"));
    }
    EXPECT_EQ(declared_books, actual_books) << portfolio_path.string();

    for (const auto& [book, expected_trade_ids] : manifest.at("expected_books").items()) {
        ASSERT_TRUE(actual_books.contains(book)) << "Missing book " << book;
        EXPECT_EQ(actual_books.at(book), json_string_set(expected_trade_ids)) << book;
    }

    auto valuation_results = qrp::analytics::ValuationService::price_portfolio(portfolio, context);
    const auto& expected_trades = manifest.at("trades");
    ASSERT_EQ(valuation_results.size(), expected_trades.size());

    std::set<std::string> seen_trade_ids;
    for (const auto& result : valuation_results) {
        ASSERT_TRUE(expected_trades.contains(result.trade_id)) << "Unexpected trade " << result.trade_id;
        const auto& expected = expected_trades.at(result.trade_id);

        EXPECT_TRUE(std::isfinite(result.npv)) << result.trade_id;
        EXPECT_EQ(qrp::domain::to_string(result.asset_class), expected.at("asset_class").get<std::string>())
            << result.trade_id;
        EXPECT_EQ(qrp::domain::to_string(result.product_type), expected.at("product_type").get<std::string>())
            << result.trade_id;
        EXPECT_EQ(qrp::domain::to_string(result.support_status), expected.at("support_status").get<std::string>())
            << result.trade_id;
        EXPECT_EQ(result.tags.at("status"), expected.at("support_status").get<std::string>()) << result.trade_id;

        seen_trade_ids.insert(result.trade_id);
    }

    EXPECT_EQ(seen_trade_ids, json_object_key_set(expected_trades)) << "Trade coverage drifted";
}

} // namespace

TEST(PortfolioGoldenCoverageTest, PortfolioFixturesPriceExpectedBooksAndProducts) {
    const auto fixture_dir = qrp::test::data_file({"regression", "portfolio_fixtures"});
    ASSERT_TRUE(std::filesystem::exists(fixture_dir)) << fixture_dir.string();

    std::vector<std::filesystem::path> manifests;
    for (const auto& entry : std::filesystem::directory_iterator(fixture_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            manifests.push_back(entry.path());
        }
    }
    std::sort(manifests.begin(), manifests.end());
    ASSERT_FALSE(manifests.empty()) << fixture_dir.string();

    for (const auto& manifest_path : manifests) {
        SCOPED_TRACE(manifest_path.filename().string());
        verify_portfolio_fixture(manifest_path);
    }
}
