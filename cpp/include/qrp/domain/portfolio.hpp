#pragma once
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

namespace qrp::domain {

struct Trade {
    std::string id;
    std::string asset_class;
    std::string type;
    std::string currency;
    double notional;
    std::string start_date;
    std::string maturity_date;
    std::string direction;
    std::string book;
    std::string strategy;
    nlohmann::json details;
};

inline void from_json(const nlohmann::json& j, Trade& t) {
    j.at("id").get_to(t.id);
    j.at("asset_class").get_to(t.asset_class);
    j.at("type").get_to(t.type);
    j.at("currency").get_to(t.currency);
    j.at("notional").get_to(t.notional);
    j.at("start_date").get_to(t.start_date);
    j.at("maturity_date").get_to(t.maturity_date);
    j.at("direction").get_to(t.direction);
    j.at("book").get_to(t.book);
    j.at("strategy").get_to(t.strategy);
    if (j.contains("details")) {
        t.details = j.at("details");
    }
}

struct Portfolio {
    std::string portfolio_id;
    std::vector<Trade> trades;
};

inline void from_json(const nlohmann::json& j, Portfolio& p) {
    j.at("portfolio_id").get_to(p.portfolio_id);
    j.at("trades").get_to(p.trades);
}

} // namespace qrp::domain
