// Implements convention-registry lookup and coordinates asset-class default registration.

#include <qrp/conventions/market_convention_registry.hpp>

#include <qrp/conventions/market_convention_defaults.hpp>

namespace qrp::conventions {

MarketConventionRegistry& MarketConventionRegistry::instance() {
    static MarketConventionRegistry inst;
    return inst;
}

MarketConventionRegistry::MarketConventionRegistry() {
    load_defaults();
}

void MarketConventionRegistry::register_rates_convention(const RatesConvention& conv) {
    rates_conventions_[{conv.currency, conv.index_family}] = conv;
}

RatesConvention MarketConventionRegistry::get_rates_convention(domain::Currency currency, const std::string& index_family) const {
    auto it = rates_conventions_.find({currency, index_family});
    if (it != rates_conventions_.end()) {
        return it->second;
    }

    // Use the currency OIS convention when a specific tenor/index family is not configured.
    it = rates_conventions_.find({currency, "OIS"});
    if (it != rates_conventions_.end()) {
        return it->second;
    }

    // Last-resort convention preserves the requested key and uses RatesConvention defaults.
    RatesConvention def;
    def.currency = currency;
    def.index_family = index_family;
    return def;
}

void MarketConventionRegistry::load_defaults() {
    register_default_rates_conventions(*this);
}

} // namespace qrp::conventions
