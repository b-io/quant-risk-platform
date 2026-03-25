#pragma once
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/portfolio.hpp>
#include <string>

namespace qrp::io {

domain::MarketSnapshot load_market(const std::string& path);
domain::Portfolio load_portfolio(const std::string& path);

} // namespace qrp::io
