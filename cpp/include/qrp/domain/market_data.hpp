#pragma once
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

namespace qrp::domain {

struct CurveNode {
    std::string tenor;
    double value;
};

inline void from_json(const nlohmann::json& j, CurveNode& n) {
    j.at("tenor").get_to(n.tenor);
    j.at("value").get_to(n.value);
}

struct CurveDefinition {
    std::string type;
    std::string currency;
    std::vector<CurveNode> nodes;
};

inline void from_json(const nlohmann::json& j, CurveDefinition& c) {
    j.at("type").get_to(c.type);
    j.at("currency").get_to(c.currency);
    j.at("nodes").get_to(c.nodes);
}

struct MarketSnapshot {
    std::string valuation_date;
    std::map<std::string, std::map<std::string, CurveDefinition>> markets;
};

inline void from_json(const nlohmann::json& j, MarketSnapshot& m) {
    j.at("valuation_date").get_to(m.valuation_date);
    j.at("markets").get_to(m.markets);
}

} // namespace qrp::domain
