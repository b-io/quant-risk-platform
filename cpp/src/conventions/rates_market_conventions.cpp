// Registers built-in rates-market conventions by currency and index family.

#include <qrp/conventions/market_convention_defaults.hpp>

namespace qrp::conventions {
namespace {

/**
 * @brief Creates the common OIS convention skeleton for a currency.
 */
RatesConvention make_ois_convention(domain::Currency currency,
                                    domain::BusinessCalendar calendar,
                                    int settlement_days,
                                    domain::DayCount day_count) {
    RatesConvention convention;
    convention.currency = currency;
    convention.index_family = "OIS";
    convention.calendar = calendar;
    convention.settlement_days = settlement_days;
    convention.business_day_convention = domain::BusinessDayConvention::ModifiedFollowing;
    convention.day_count = day_count;
    convention.fixed_leg_frequency = domain::Frequency::Annual;
    convention.fixed_leg_day_count = day_count;
    convention.fixed_leg_bdc = domain::BusinessDayConvention::ModifiedFollowing;
    convention.floating_leg_frequency = domain::Frequency::Annual;
    convention.floating_leg_day_count = day_count;
    convention.floating_leg_bdc = domain::BusinessDayConvention::ModifiedFollowing;
    convention.date_generation = domain::DateGeneration::Forward;
    return convention;
}

/**
 * @brief Creates the USD 3M IBOR convention used by demo curves and swap products.
 */
RatesConvention make_usd_ibor_3m_convention() {
    RatesConvention convention;
    convention.currency = domain::Currency::USD;
    convention.index_family = "IBOR_3M";
    convention.calendar = domain::BusinessCalendar::US;
    convention.settlement_days = 2;
    convention.business_day_convention = domain::BusinessDayConvention::ModifiedFollowing;
    convention.day_count = domain::DayCount::ACT360;
    convention.fixed_leg_frequency = domain::Frequency::Semiannual;
    convention.fixed_leg_day_count = domain::DayCount::Thirty360;
    convention.fixed_leg_bdc = domain::BusinessDayConvention::ModifiedFollowing;
    convention.floating_leg_frequency = domain::Frequency::Quarterly;
    convention.floating_leg_day_count = domain::DayCount::ACT360;
    convention.floating_leg_bdc = domain::BusinessDayConvention::ModifiedFollowing;
    convention.date_generation = domain::DateGeneration::Forward;
    return convention;
}

} // namespace

void register_default_rates_conventions(MarketConventionRegistry& registry) {
    registry.register_rates_convention(
        make_ois_convention(domain::Currency::USD, domain::BusinessCalendar::US, 2, domain::DayCount::ACT360));
    registry.register_rates_convention(make_usd_ibor_3m_convention());
    registry.register_rates_convention(
        make_ois_convention(domain::Currency::EUR, domain::BusinessCalendar::Target, 2, domain::DayCount::ACT360));
    registry.register_rates_convention(
        make_ois_convention(domain::Currency::GBP, domain::BusinessCalendar::UK, 0, domain::DayCount::ACT365));
    registry.register_rates_convention(
        make_ois_convention(domain::Currency::CHF, domain::BusinessCalendar::CHF, 2, domain::DayCount::ACT360));
}

} // namespace qrp::conventions
