#include <qrp/conventions/market_convention_registry.hpp>

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
    
    // Fallback to OIS if specific index not found
    it = rates_conventions_.find({currency, "OIS"});
    if (it != rates_conventions_.end()) {
        return it->second;
    }

    // Ultimate fallback (USD OIS style)
    RatesConvention def;
    def.currency = currency;
    def.index_family = index_family;
    return def;
}

void MarketConventionRegistry::load_defaults() {
    // USD SOFR / OIS
    RatesConvention usd_ois;
    usd_ois.currency = domain::Currency::USD;
    usd_ois.index_family = "OIS";
    usd_ois.calendar = domain::BusinessCalendar::US;
    usd_ois.settlement_days = 2;
    usd_ois.business_day_convention = domain::BusinessDayConvention::ModifiedFollowing;
    usd_ois.day_count = domain::DayCount::ACT360;
    usd_ois.fixed_leg_frequency = domain::Frequency::Annual;
    usd_ois.fixed_leg_day_count = domain::DayCount::ACT360;
    register_rates_convention(usd_ois);

    // USD LIBOR 3M
    RatesConvention usd_l3m;
    usd_l3m.currency = domain::Currency::USD;
    usd_l3m.index_family = "IBOR_3M";
    usd_l3m.calendar = domain::BusinessCalendar::US;
    usd_l3m.settlement_days = 2;
    usd_l3m.business_day_convention = domain::BusinessDayConvention::ModifiedFollowing;
    usd_l3m.day_count = domain::DayCount::ACT360;
    usd_l3m.fixed_leg_frequency = domain::Frequency::Semiannual;
    usd_l3m.fixed_leg_day_count = domain::DayCount::Thirty360;
    usd_l3m.floating_leg_frequency = domain::Frequency::Quarterly;
    register_rates_convention(usd_l3m);

    // EUR ESTR / OIS
    RatesConvention eur_ois;
    eur_ois.currency = domain::Currency::EUR;
    eur_ois.index_family = "OIS";
    eur_ois.calendar = domain::BusinessCalendar::Target;
    eur_ois.settlement_days = 2;
    eur_ois.business_day_convention = domain::BusinessDayConvention::ModifiedFollowing;
    eur_ois.day_count = domain::DayCount::ACT360;
    register_rates_convention(eur_ois);
}

} // namespace qrp::conventions
