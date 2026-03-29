#include <qrp/util/logger.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/common.h>
#include <cstdlib>
#include <algorithm>
#include <cctype>

namespace qrp::util {

namespace {
    std::shared_ptr<spdlog::logger> g_logger;
}

void Logger::initialize() {
    if (g_logger) return;

    // Create the default console sink with color support
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    
    // Pattern: [Timestamp] [Level] [Thread ID] %v (message)
    // %^ and %$ mark the colored part of the log
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%f] [%^%l%$] [tid %t] %v");

    g_logger = std::make_shared<spdlog::logger>("qrp", console_sink);
    
    // Default log level
    spdlog::level::level_enum level = spdlog::level::info;

    // Check for environment variable
    const char* env_level = std::getenv("QRP_LOG_LEVEL");
    if (env_level) {
        std::string s_level(env_level);
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
    
    // Register as the global default logger so spdlog::info(...) works directly
    spdlog::set_default_logger(g_logger);
}

std::shared_ptr<spdlog::logger> Logger::get() {
    if (!g_logger) {
        initialize();
    }
    return g_logger;
}

} // namespace qrp::util
