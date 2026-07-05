#pragma once

// Declares internal asset-class default convention registration hooks.
// The public registry API remains in market_convention_registry.hpp.

#include <qrp/conventions/market_convention_registry.hpp>

namespace qrp::conventions {

/**
 * @brief Registers built-in rates-market conventions with the supplied registry.
 */
void register_default_rates_conventions(MarketConventionRegistry& registry);

} // namespace qrp::conventions
