#include <qrp/io/json_loader.hpp>
#include <fstream>
#include <nlohmann/json.hpp>

namespace qrp::io {

domain::MarketSnapshot load_market(const std::string& path) {
    std::ifstream f(path);
    nlohmann::json j;
    f >> j;
    return j.get<domain::MarketSnapshot>();
}

domain::Portfolio load_portfolio(const std::string& path) {
    std::ifstream f(path);
    nlohmann::json j;
    f >> j;
    return j.get<domain::Portfolio>();
}

} // namespace qrp::io
