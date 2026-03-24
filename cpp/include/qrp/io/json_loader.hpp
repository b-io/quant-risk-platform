#pragma once
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/portfolio.hpp>
#include <fstream>

namespace qrp::io {

inline domain::MarketSnapshot load_market(const std::string& path) {
    std::ifstream f(path);
    nlohmann::json j;
    f >> j;
    return j.get<domain::MarketSnapshot>();
}

inline domain::Portfolio load_portfolio(const std::string& path) {
    std::ifstream f(path);
    nlohmann::json j;
    f >> j;
    return j.get<domain::Portfolio>();
}

} // namespace qrp::io
