#pragma once

#include <filesystem>
#include <initializer_list>
#include <stdexcept>
#include <string>

namespace qrp::test {

inline std::filesystem::path configured_data_dir() {
#ifdef QRP_TEST_DATA_DIR
    return std::filesystem::path(QRP_TEST_DATA_DIR);
#else
    return {};
#endif
}

inline std::filesystem::path find_data_dir() {
    const auto configured = configured_data_dir();
    if (!configured.empty() && std::filesystem::exists(configured)) {
        return configured;
    }

    auto current = std::filesystem::current_path();
    while (true) {
        const auto candidate = current / "data";
        if (std::filesystem::exists(candidate / "market" / "demo_market.json") &&
            std::filesystem::exists(candidate / "portfolios" / "demo_portfolio.json")) {
            return candidate;
        }

        if (current == current.parent_path()) {
            break;
        }
        current = current.parent_path();
    }

    throw std::runtime_error("Test data directory was not found. Configure QRP_TEST_DATA_DIR or run from the repository tree.");
}

inline std::filesystem::path data_file(std::initializer_list<std::string> parts) {
    auto path = find_data_dir();
    for (const auto& part : parts) {
        path /= part;
    }
    return path;
}

} // namespace qrp::test
