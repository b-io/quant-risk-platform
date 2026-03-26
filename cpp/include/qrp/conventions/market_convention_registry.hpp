#pragma once
#include <qrp/domain/types.hpp>
#include <map>
#include <string>

namespace qrp::conventions {

struct RatesConvention {
    domain::Currency currency;
    std::string index_family; // OIS, IBOR_3M, IBOR_6M, etc.
    
    domain::BusinessCalendar calendar = domain::BusinessCalendar::Target;
    int settlement_days = 2;
    domain::BusinessDayConvention business_day_convention = domain::BusinessDayConvention::ModifiedFollowing;
    domain::DayCount day_count = domain::DayCount::ACT360;
    
    // For Swaps
    domain::Frequency fixed_leg_frequency = domain::Frequency::Annual;
    domain::DayCount fixed_leg_day_count = domain::DayCount::Thirty360;
    domain::BusinessDayConvention fixed_leg_bdc = domain::BusinessDayConvention::ModifiedFollowing;
    
    // For IBOR
    domain::Frequency floating_leg_frequency = domain::Frequency::Quarterly;
    domain::DayCount floating_leg_day_count = domain::DayCount::ACT360;
    domain::BusinessDayConvention floating_leg_bdc = domain::BusinessDayConvention::ModifiedFollowing;
    
    domain::DateGeneration date_generation = domain::DateGeneration::Forward;
};

class MarketConventionRegistry {
public:
    static MarketConventionRegistry& instance();

    void register_rates_convention(const RatesConvention& conv);
    RatesConvention get_rates_convention(domain::Currency currency, const std::string& index_family) const;

private:
    MarketConventionRegistry();
    void load_defaults();

    std::map<std::pair<domain::Currency, std::string>, RatesConvention> rates_conventions_;
};

} // namespace qrp::conventions
