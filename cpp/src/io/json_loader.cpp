// Implements JSON loading helpers for portfolio and market snapshot inputs.

#include <qrp/io/json_loader.hpp>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace qrp::io {

domain::MarketSnapshot load_market(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Unable to open market JSON file: " + path);
    }
    nlohmann::json j;
    f >> j;
    auto snapshot = j.get<domain::MarketSnapshot>();
    snapshot.diagnostics = domain::collect_market_snapshot_diagnostics(snapshot);
    return snapshot;
}

domain::Portfolio load_portfolio(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        throw std::runtime_error("Unable to open portfolio JSON file: " + path);
    }
    nlohmann::json j;
    f >> j;
    return j.get<domain::Portfolio>();
}

} // namespace qrp::io
