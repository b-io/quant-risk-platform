#pragma once

// Declares convention lookup and override APIs for market construction and pricing.

#include <qrp/domain/types.hpp>

#include <map>
#include <string>

namespace qrp::conventions {

/**
 * @brief Rates-market conventions used for curve helpers, swaps, and schedules.
 */
struct RatesConvention {
    /** @brief Currency whose calendar and schedule conventions are described. */
    domain::Currency currency;
    /** @brief Curve or index family such as OIS, IBOR_3M, or IBOR_6M. */
    std::string index_family;
    /** @brief Business calendar used for helper dates, schedules, and settlement. */
    domain::BusinessCalendar calendar = domain::BusinessCalendar::Target;
    /** @brief Spot settlement lag in business days. */
    int settlement_days = 2;
    /** @brief Default business-day adjustment for helpers and schedules. */
    domain::BusinessDayConvention business_day_convention = domain::BusinessDayConvention::ModifiedFollowing;
    /** @brief Default accrual day-count convention for simple rate instruments. */
    domain::DayCount day_count = domain::DayCount::ACT360;
    /** @brief Fixed-leg coupon frequency for swap-style products. */
    domain::Frequency fixed_leg_frequency = domain::Frequency::Annual;
    /** @brief Fixed-leg accrual day-count convention for swap-style products. */
    domain::DayCount fixed_leg_day_count = domain::DayCount::Thirty360;
    /** @brief Fixed-leg business-day adjustment for swap-style products. */
    domain::BusinessDayConvention fixed_leg_bdc = domain::BusinessDayConvention::ModifiedFollowing;
    /** @brief Floating-leg reset frequency for IBOR or overnight indexed products. */
    domain::Frequency floating_leg_frequency = domain::Frequency::Quarterly;
    /** @brief Floating-leg accrual day-count convention. */
    domain::DayCount floating_leg_day_count = domain::DayCount::ACT360;
    /** @brief Floating-leg business-day adjustment. */
    domain::BusinessDayConvention floating_leg_bdc = domain::BusinessDayConvention::ModifiedFollowing;
    /** @brief Schedule generation rule used for standard schedules. */
    domain::DateGeneration date_generation = domain::DateGeneration::Forward;
};

/**
 * @brief In-memory registry for default and user-provided market conventions.
 */
class MarketConventionRegistry {
public:
    /**
     * @brief Returns the process-wide convention registry.
     */
    static MarketConventionRegistry& instance();

    /**
     * @brief Adds or replaces a rates convention keyed by currency and index family.
     */
    void register_rates_convention(const RatesConvention& conv);

    /**
     * @brief Resolves a rates convention, falling back to sensible defaults when absent.
     */
    RatesConvention get_rates_convention(domain::Currency currency, const std::string& index_family) const;

private:
    /**
     * @brief Creates a registry seeded with platform defaults.
     */
    MarketConventionRegistry();

    /**
     * @brief Registers the built-in rates conventions.
     */
    void load_defaults();

    std::map<std::pair<domain::Currency, std::string>, RatesConvention> rates_conventions_; // Rates conventions keyed by currency and index family.
};

} // namespace qrp::conventions
