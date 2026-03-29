#pragma once

#include <spdlog/spdlog.h>
#include <string>

namespace qrp::util {

/**
 * @brief Centralized logger utility for the platform.
 * 
 * Provides static methods to initialize and configure logging
 * globally based on environment variables and defaults.
 */
class Logger {
public:
    /**
     * @brief Initializes global logging settings.
     * 
     * Reads QRP_LOG_LEVEL environment variable (trace, debug, info, warn, err, critical).
     * Defaults to 'info' if not set or invalid.
     */
    static void initialize();

    /**
     * @brief Gets the platform's primary logger.
     */
    static std::shared_ptr<spdlog::logger> get();
};

} // namespace qrp::util
