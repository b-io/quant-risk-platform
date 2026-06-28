#pragma once

// Declares JSON file loading helpers for canonical market and portfolio DTOs.

#include <qrp/domain/market_data.hpp>
#include <qrp/domain/portfolio.hpp>
#include <string>

namespace qrp::io {

/**
 * @brief Loads a market snapshot JSON file and attaches computed diagnostics.
 */
domain::MarketSnapshot load_market(const std::string& path);

/**
 * @brief Loads a portfolio JSON file into the canonical trade model.
 */
domain::Portfolio load_portfolio(const std::string& path);

} // namespace qrp::io
