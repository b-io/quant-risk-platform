#pragma once

// Declares market snapshot validation and QuantLib market-object construction services.

#include <ql/time/date.hpp>
#include <ql/time/period.hpp>
#include <ql/termstructures/yieldtermstructure.hpp>
#include <ql/time/dategenerationrule.hpp>
#include <ql/indexes/iborindex.hpp>
#include <ql/time/daycounter.hpp>
#include <ql/time/calendar.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/market/market_state.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace qrp::market {

/**
 * @brief Result of attempting to build one market curve into a MarketState.
 */
struct CurveBuildResult {
    domain::CurveId id;
    domain::CurvePurpose purpose = domain::CurvePurpose::UNKNOWN;
    std::vector<std::string> quote_ids;
    bool built = false;
    std::string status_message;
};

/**
 * @brief Output from building the supported rates market objects in a snapshot.
 */
struct RatesMarketBuildResult {
    std::shared_ptr<MarketState> state;
    std::vector<CurveBuildResult> curve_results;
    std::vector<std::string> rates_vol_quote_ids;
};

/**
 * @brief Builds QuantLib rates curves and rates-vol quote handles from normalized market data.
 */
class CurveBuilder {
public:
    /**
     * @brief Parses a canonical YYYY-MM-DD date into a QuantLib date.
     */
    static QuantLib::Date parse_date(const std::string& date_str);

    /**
     * @brief Parses a tenor such as 3M or 5Y into a QuantLib period.
     */
    static QuantLib::Period parse_tenor(const std::string& tenor);

    /**
     * @brief Maps a domain day-count convention into QuantLib.
     */
    static QuantLib::DayCounter parse_day_count(domain::DayCount dc);

    /**
     * @brief Maps a domain business calendar into QuantLib.
     */
    static QuantLib::Calendar parse_calendar(domain::BusinessCalendar cal);

    /**
     * @brief Converts a tenor into an approximate year fraction for ordering and diagnostics.
     */
    static double tenor_to_years(const std::string& tenor);

    /**
     * @brief Maps a domain business-day convention into QuantLib.
     */
    static QuantLib::BusinessDayConvention parse_business_day_convention(domain::BusinessDayConvention bdc);

    /**
     * @brief Maps a domain schedule frequency into QuantLib.
     */
    static QuantLib::Frequency parse_frequency(domain::Frequency freq);

    /**
     * @brief Maps a domain date-generation rule into QuantLib.
     */
    static QuantLib::DateGeneration::Rule parse_date_generation(domain::DateGeneration rule);

    /**
     * @brief Returns whether a curve purpose is supported by the rates builder.
     */
    static bool supports_rates_curve_purpose(domain::CurvePurpose purpose);

    /**
     * @brief Returns whether a raw quote can be consumed by the rates curve builder.
     */
    static bool supports_rates_curve_quote(domain::QuoteInstrumentType type);

    /**
     * @brief Returns whether a raw quote belongs to a rates volatility surface input.
     */
    static bool supports_rates_vol_quote(domain::QuoteInstrumentType type);

    /**
     * @brief Creates the standard overnight index for a currency.
     */
    static QuantLib::ext::shared_ptr<QuantLib::OvernightIndex> create_overnight_index(
        domain::Currency currency, 
        const QuantLib::Handle<QuantLib::YieldTermStructure>& h);

    /**
     * @brief Builds a supported rates yield curve from a spec and quotes.
     * Rationale: We use PiecewiseYieldCurve with LogLinear on discounts to ensure
     * positive forward rates and industry-standard interpolation.
     * The state_ptr allows us to reuse SimpleQuote handles for reactive risk.
     */
    static QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> build_rate_curve(
        const domain::CurveSpec& spec,
        const std::map<std::string, domain::MarketQuote>& quotes,
        const QuantLib::Date& valuation_date,
        std::shared_ptr<MarketState> state_ptr = nullptr);

    /**
     * @brief Creates the standard IBOR index for a currency and tenor.
     */
    static QuantLib::ext::shared_ptr<QuantLib::IborIndex> create_ibor_index(
        domain::Currency currency,
        const QuantLib::Period& tenor,
        const QuantLib::Handle<QuantLib::YieldTermStructure>& h);
};

/**
 * @brief Orchestrates construction of all supported rates market objects in a snapshot.
 */
class RatesMarketBuilder {
public:
    /**
     * @brief Builds quote handles, rates curves, and rates-vol quote catalogs into a MarketState.
     */
    static RatesMarketBuildResult build(const domain::MarketSnapshot& dto);
};

/**
 * @brief Runtime market snapshot with validated QuantLib market state.
 */
class MarketSnapshot {
public:
    /**
     * @brief Validates and builds the supported runtime market objects from a domain DTO.
     */
    explicit MarketSnapshot(const domain::MarketSnapshot& dto);

    /**
     * @brief Returns the built market state used by pricing and risk workflows.
     */
    std::shared_ptr<MarketState> built_state() const { return state_; }

private:
    std::shared_ptr<MarketState> state_;
};

} // namespace qrp::market
