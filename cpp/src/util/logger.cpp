// Implements the platform logger bootstrap and shared sink configuration.

#include <qrp/util/logger.hpp>

#include <spdlog/common.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>

namespace qrp::util {

namespace {

std::shared_ptr<spdlog::logger> g_logger;

std::optional<std::string> env_value(const char* name) {
#ifdef _MSC_VER
    char* raw_value = nullptr;
    std::size_t value_size = 0;
    if (_dupenv_s(&raw_value, &value_size, name) != 0) {
        std::free(raw_value);
        return std::nullopt;
    }

    if (raw_value == nullptr || *raw_value == '\0') {
        std::free(raw_value);
        return std::nullopt;
    }

    std::string value(raw_value);
    std::free(raw_value);
    return value;
#else
    const char* raw_value = std::getenv(name);
    if (raw_value == nullptr || *raw_value == '\0') {
        return std::nullopt;
    }
    return std::string(raw_value);
#endif
}

} // namespace

void Logger::initialize() {
    if (g_logger) return;

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%^%l%$] [tid %t] %v");

    g_logger = std::make_shared<spdlog::logger>("qrp", console_sink);

    spdlog::level::level_enum level = spdlog::level::info;

    const auto env_level = env_value("QRP_LOG_LEVEL");
    if (env_level) {
        std::string s_level = *env_level;
        std::transform(s_level.begin(), s_level.end(), s_level.begin(),
                       [](unsigned char c){ return std::tolower(c); });

        if (s_level == "trace") level = spdlog::level::trace;
        else if (s_level == "debug") level = spdlog::level::debug;
        else if (s_level == "info") level = spdlog::level::info;
        else if (s_level == "warn") level = spdlog::level::warn;
        else if (s_level == "err") level = spdlog::level::err;
        else if (s_level == "critical") level = spdlog::level::critical;
        else if (s_level == "off") level = spdlog::level::off;
    }

    g_logger->set_level(level);
    spdlog::set_default_logger(g_logger);
}

std::shared_ptr<spdlog::logger> Logger::get() {
    if (!g_logger) {
        initialize();
    }
    return g_logger;
}

} // namespace qrp::util
