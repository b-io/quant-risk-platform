#pragma once
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

class CurveBuilder {
public:
    static QuantLib::Date parse_date(const std::string& date_str);
    static QuantLib::Period parse_tenor(const std::string& tenor);
    static QuantLib::DayCounter parse_day_count(domain::DayCount dc);
    static QuantLib::Calendar parse_calendar(domain::BusinessCalendar cal);
    static double tenor_to_years(const std::string& tenor);

    static QuantLib::BusinessDayConvention parse_business_day_convention(domain::BusinessDayConvention bdc);
    static QuantLib::Frequency parse_frequency(domain::Frequency freq);
    // Added missing DateGeneration header dependency might be needed in some TUs
    static QuantLib::DateGeneration::Rule parse_date_generation(domain::DateGeneration rule);

    static QuantLib::ext::shared_ptr<QuantLib::OvernightIndex> create_overnight_index(
        domain::Currency currency, 
        const QuantLib::Handle<QuantLib::YieldTermStructure>& h);

    static QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> build_curve(
        const domain::CurveSpec& spec,
        const std::map<std::string, domain::MarketQuote>& quotes,
        const QuantLib::Date& valuation_date,
        std::shared_ptr<MarketState> state_ptr = nullptr);

    static QuantLib::ext::shared_ptr<QuantLib::IborIndex> create_ibor_index(
        domain::Currency currency,
        const QuantLib::Period& tenor,
        const QuantLib::Handle<QuantLib::YieldTermStructure>& h);
};

class MarketSnapshot {
public:
    explicit MarketSnapshot(const domain::MarketSnapshot& dto);
    
    std::shared_ptr<MarketState> built_state() const { return state_; }

private:
    std::shared_ptr<MarketState> state_;
};

} // namespace qrp::market
