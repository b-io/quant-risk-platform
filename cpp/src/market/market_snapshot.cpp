#include <qrp/market/market_snapshot.hpp>
#include <qrp/conventions/market_convention_registry.hpp>
#include <ql/currencies/america.hpp>
#include <ql/currencies/asia.hpp>
#include <ql/currencies/europe.hpp>
#include <ql/currencies/oceania.hpp>
#include <ql/indexes/ibor/chflibor.hpp>
#include <ql/indexes/ibor/estr.hpp>
#include <ql/indexes/ibor/euribor.hpp>
#include <ql/indexes/ibor/fedfunds.hpp>
#include <ql/indexes/ibor/gbplibor.hpp>
#include <ql/indexes/ibor/jpylibor.hpp>
#include <ql/indexes/ibor/saron.hpp>
#include <ql/indexes/ibor/sofr.hpp>
#include <ql/indexes/ibor/sonia.hpp>
#include <ql/indexes/ibor/tonar.hpp>
#include <ql/indexes/ibor/usdlibor.hpp>
#include <ql/settings.hpp>
#include <ql/termstructures/yield/piecewiseyieldcurve.hpp>
#include <ql/termstructures/yield/ratehelpers.hpp>
#include <ql/termstructures/yield/oisratehelper.hpp>
#include <ql/termstructures/iterativebootstrap.hpp>
#include <ql/termstructures/yield/discountcurve.hpp>
#include <ql/math/interpolations/loginterpolation.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/time/calendars/japan.hpp>
#include <ql/time/calendars/switzerland.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/time/calendars/unitedkingdom.hpp>
#include <ql/time/calendars/unitedstates.hpp>
#include <ql/time/daycounters/actual360.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/time/daycounters/actualactual.hpp>
#include <ql/time/daycounters/thirty360.hpp>
#include <fmt/format.h>

/*
Design note (see docs/design/CURVE_BOOTSTRAP_DESIGN.md):
- We build yield curves with QuantLib::PiecewiseYieldCurve<Discount, LogLinear>.
- Helpers are stored as QuantLib::ext::shared_ptr to be ABI-compatible with the QuantLib build in vcpkg
  (QuantLib uses ext::shared_ptr which maps to boost::shared_ptr or std::shared_ptr depending on config).
- Quotes are QuantLib::SimpleQuote handles owned by MarketState. Any bump on a handle propagates to the
  curve via QuantLib's observer mechanism (no rebuild), which is essential for fast risk/scenario runs.
- We support both OIS (discount) and IBOR/FRA/IRS helpers. The choice of helpers reflects market practice:
  discounting from OIS, forwarding from term indices. Interpolation on discount factors via LogLinear keeps
  positivity of discount factors and is an industry-standard choice for production curves.
*/
namespace qrp::market {

QuantLib::Date CurveBuilder::parse_date(const std::string& date_str) {
    int y = std::stoi(date_str.substr(0, 4));
    int m = std::stoi(date_str.substr(5, 2));
    int d = std::stoi(date_str.substr(8, 2));
    return QuantLib::Date(d, static_cast<QuantLib::Month>(m), y);
}

QuantLib::Period CurveBuilder::parse_tenor(const std::string& tenor) {
    if (tenor.empty()) return QuantLib::Period(0, QuantLib::Days);
    
    char unit = tenor.back();
    int value = std::stoi(tenor.substr(0, tenor.size() - 1));
    
    switch (unit) {
        case 'D': return QuantLib::Period(value, QuantLib::Days);
        case 'W': return QuantLib::Period(value, QuantLib::Weeks);
        case 'M': return QuantLib::Period(value, QuantLib::Months);
        case 'Y': return QuantLib::Period(value, QuantLib::Years);
        default: return QuantLib::Period(0, QuantLib::Days);
    }
}

QuantLib::DayCounter CurveBuilder::parse_day_count(domain::DayCount dc) {
    switch (dc) {
        case domain::DayCount::ACT360: return QuantLib::Actual360();
        case domain::DayCount::ACT365: 
        case domain::DayCount::ACT365F: return QuantLib::Actual365Fixed();
        case domain::DayCount::Thirty360: return QuantLib::Thirty360(QuantLib::Thirty360::BondBasis);
        case domain::DayCount::ACTACT:
        case domain::DayCount::ACTACT_ISDA: return QuantLib::ActualActual(QuantLib::ActualActual::ISDA);
        case domain::DayCount::ACTACT_ISMA: return QuantLib::ActualActual(QuantLib::ActualActual::ISMA);
        case domain::DayCount::ACTACT_AFB: return QuantLib::ActualActual(QuantLib::ActualActual::AFB);
        case domain::DayCount::ACTACT_EURO: return QuantLib::ActualActual(QuantLib::ActualActual::Euro);
        default: return QuantLib::Actual365Fixed();
    }
}

QuantLib::Calendar CurveBuilder::parse_calendar(domain::BusinessCalendar cal) {
    switch (cal) {
        case domain::BusinessCalendar::Target: return QuantLib::TARGET();
        case domain::BusinessCalendar::US: return QuantLib::UnitedStates(QuantLib::UnitedStates::GovernmentBond);
        case domain::BusinessCalendar::UK: return QuantLib::UnitedKingdom();
        case domain::BusinessCalendar::CHF: return QuantLib::Switzerland();
        case domain::BusinessCalendar::JP: return QuantLib::Japan();
        case domain::BusinessCalendar::WeekendsOnly: return QuantLib::NullCalendar();
        default: return QuantLib::TARGET();
    }
}

double CurveBuilder::tenor_to_years(const std::string& tenor) {
    if (tenor.empty()) return 0.0;
    QuantLib::Period p = parse_tenor(tenor);
    if (p.units() == QuantLib::Days) return p.length() / 365.25;
    if (p.units() == QuantLib::Weeks) return p.length() / 52.1786;
    if (p.units() == QuantLib::Months) return p.length() / 12.0;
    if (p.units() == QuantLib::Years) return static_cast<double>(p.length());
    return 0.0;
}

QuantLib::BusinessDayConvention CurveBuilder::parse_business_day_convention(domain::BusinessDayConvention bdc) {
    switch (bdc) {
        case domain::BusinessDayConvention::Following: return QuantLib::Following;
        case domain::BusinessDayConvention::ModifiedFollowing: return QuantLib::ModifiedFollowing;
        case domain::BusinessDayConvention::Preceding: return QuantLib::Preceding;
        case domain::BusinessDayConvention::ModifiedPreceding: return QuantLib::ModifiedPreceding;
        case domain::BusinessDayConvention::Unadjusted: return QuantLib::Unadjusted;
        case domain::BusinessDayConvention::HalfMonthModifiedFollowing: return QuantLib::HalfMonthModifiedFollowing;
        case domain::BusinessDayConvention::Nearest: return QuantLib::Nearest;
        default: return QuantLib::Following;
    }
}

QuantLib::Frequency CurveBuilder::parse_frequency(domain::Frequency freq) {
    switch (freq) {
        case domain::Frequency::Once: return QuantLib::Once;
        case domain::Frequency::Annual: return QuantLib::Annual;
        case domain::Frequency::Semiannual: return QuantLib::Semiannual;
        case domain::Frequency::EveryFourthMonth: return QuantLib::EveryFourthMonth;
        case domain::Frequency::Quarterly: return QuantLib::Quarterly;
        case domain::Frequency::Bimonthly: return QuantLib::Bimonthly;
        case domain::Frequency::Monthly: return QuantLib::Monthly;
        case domain::Frequency::EveryFourthWeek: return QuantLib::EveryFourthWeek;
        case domain::Frequency::Biweekly: return QuantLib::Biweekly;
        case domain::Frequency::Weekly: return QuantLib::Weekly;
        case domain::Frequency::Daily: return QuantLib::Daily;
        case domain::Frequency::OtherFrequency: return QuantLib::OtherFrequency;
        default: return QuantLib::Annual;
    }
}

QuantLib::DateGeneration::Rule CurveBuilder::parse_date_generation(domain::DateGeneration rule) {
    switch (rule) {
        case domain::DateGeneration::Backward: return QuantLib::DateGeneration::Backward;
        case domain::DateGeneration::Forward: return QuantLib::DateGeneration::Forward;
        case domain::DateGeneration::Zero: return QuantLib::DateGeneration::Zero;
        case domain::DateGeneration::ThirdWednesday: return QuantLib::DateGeneration::ThirdWednesday;
        case domain::DateGeneration::Twentieth: return QuantLib::DateGeneration::Twentieth;
        case domain::DateGeneration::TwentiethIMM: return QuantLib::DateGeneration::TwentiethIMM;
        case domain::DateGeneration::OldCDS: return QuantLib::DateGeneration::OldCDS;
        case domain::DateGeneration::CDS: return QuantLib::DateGeneration::CDS;
        case domain::DateGeneration::CDS2015: return QuantLib::DateGeneration::CDS2015;
        default: return QuantLib::DateGeneration::Forward;
    }
}

QuantLib::ext::shared_ptr<QuantLib::OvernightIndex> CurveBuilder::create_overnight_index(
    domain::Currency currency, 
    const QuantLib::Handle<QuantLib::YieldTermStructure>& h) {
    
    switch (currency) {
        case domain::Currency::USD: return QuantLib::ext::make_shared<QuantLib::Sofr>(h);
        case domain::Currency::EUR: return QuantLib::ext::make_shared<QuantLib::Estr>(h);
        case domain::Currency::GBP: return QuantLib::ext::make_shared<QuantLib::Sonia>(h);
        case domain::Currency::CHF: return QuantLib::ext::make_shared<QuantLib::Saron>(h);
        case domain::Currency::JPY: return QuantLib::ext::make_shared<QuantLib::Tonar>(h);
        default: return QuantLib::ext::make_shared<QuantLib::Sofr>(h);
    }
}

QuantLib::ext::shared_ptr<QuantLib::IborIndex> CurveBuilder::create_ibor_index(
    domain::Currency currency,
    const QuantLib::Period& tenor,
    const QuantLib::Handle<QuantLib::YieldTermStructure>& h) {
    
    switch (currency) {
        case domain::Currency::USD: return QuantLib::ext::make_shared<QuantLib::USDLibor>(tenor, h);
        case domain::Currency::EUR: return QuantLib::ext::make_shared<QuantLib::Euribor>(tenor, h);
        case domain::Currency::GBP: return QuantLib::ext::make_shared<QuantLib::GBPLibor>(tenor, h);
        case domain::Currency::CHF: return QuantLib::ext::make_shared<QuantLib::CHFLibor>(tenor, h);
        case domain::Currency::JPY: return QuantLib::ext::make_shared<QuantLib::JPYLibor>(tenor, h);
        default: return QuantLib::ext::make_shared<QuantLib::USDLibor>(tenor, h);
    }
}

QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> CurveBuilder::build_curve(
    const domain::CurveSpec& spec,
    const std::map<std::string, domain::MarketQuote>& quotes,
    const QuantLib::Date& valuation_date,
    std::shared_ptr<MarketState> state_ptr) {

    std::vector<QuantLib::ext::shared_ptr<QuantLib::RateHelper>> helpers;
    QuantLib::Handle<QuantLib::YieldTermStructure> dummy;
    
    for (const auto& q_id : spec.quote_ids) {
        if (!quotes.contains(q_id)) continue;
        const auto& q = quotes.at(q_id);
        
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> simple_quote;
        if (state_ptr && state_ptr->get_quote_handle(q_id)) {
            simple_quote = state_ptr->get_quote_handle(q_id);
        } else {
            simple_quote = QuantLib::ext::make_shared<QuantLib::SimpleQuote>(q.value);
            if (state_ptr) state_ptr->add_quote_handle(q_id, simple_quote);
        }
        
        auto quote_handle = QuantLib::Handle<QuantLib::Quote>(simple_quote);
        auto tenor = parse_tenor(q.tenor);

        // Resolve conventions (prefer per-quote index_family, fallback by type)
        std::string idx_family = q.index_family.empty()
            ? (q.instrument_type == domain::QuoteInstrumentType::OIS ? std::string("OIS") : std::string("IBOR_3M"))
            : q.index_family;
        auto& reg = qrp::conventions::MarketConventionRegistry::instance();
        auto conv = reg.get_rates_convention(q.currency, idx_family);

        auto cal = parse_calendar(q.calendar != domain::BusinessCalendar::UNKNOWN ? q.calendar : conv.calendar);
        auto bdc = parse_business_day_convention(q.bdc != domain::BusinessDayConvention::UNKNOWN ? q.bdc : conv.business_day_convention);
        auto dc = parse_day_count(q.day_count != domain::DayCount::UNKNOWN ? q.day_count : spec.day_count);
        int settlement_days = q.settlement_days >= 0 ? q.settlement_days : conv.settlement_days;
        
        if (q.instrument_type == domain::QuoteInstrumentType::Deposit) {
            helpers.push_back(QuantLib::ext::make_shared<QuantLib::DepositRateHelper>(
                quote_handle, tenor, settlement_days, cal, bdc, false, dc));
        } else if (q.instrument_type == domain::QuoteInstrumentType::OIS) {
            auto index = create_overnight_index(q.currency, dummy);
            helpers.push_back(QuantLib::ext::make_shared<QuantLib::OISRateHelper>(settlement_days, tenor, quote_handle, index));
        } else if (q.instrument_type == domain::QuoteInstrumentType::FRA) {
            auto index = create_ibor_index(q.currency, QuantLib::Period(3, QuantLib::Months), dummy); // default 3M unless extended
            helpers.push_back(QuantLib::ext::make_shared<QuantLib::FraRateHelper>(quote_handle, tenor, index));
        } else if (q.instrument_type == domain::QuoteInstrumentType::IRS) {
            auto index = create_ibor_index(q.currency, QuantLib::Period(3, QuantLib::Months), dummy); // default 3M unless extended
            auto fixed_freq = parse_frequency(conv.fixed_leg_frequency);
            auto fixed_dc = parse_day_count(conv.fixed_leg_day_count);
            helpers.push_back(QuantLib::ext::make_shared<QuantLib::SwapRateHelper>(
                quote_handle, tenor, cal, fixed_freq, bdc, fixed_dc, index));
        }
    }

    if (helpers.empty()) return nullptr;

    using CurveT = QuantLib::PiecewiseYieldCurve<QuantLib::Discount, QuantLib::LogLinear>;
    QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> curve = QuantLib::ext::make_shared<CurveT>(
        valuation_date, helpers, parse_day_count(spec.day_count));
    curve->enableExtrapolation();
    return curve;
}

MarketSnapshot::MarketSnapshot(const domain::MarketSnapshot& dto) {
    QuantLib::Date val_date = CurveBuilder::parse_date(dto.valuation_date);
    QuantLib::Settings::instance().evaluationDate() = val_date;
    state_ = std::make_shared<MarketState>(val_date);

    std::map<std::string, domain::MarketQuote> quote_map;
    for (const auto& q : dto.quotes) {
        quote_map[q.id] = q;
        state_->add_quote(q.id, q.value);
    }

    for (const auto& spec : dto.curves) {
        auto curve = CurveBuilder::build_curve(spec, quote_map, val_date, state_);
        if (curve) {
            state_->add_curve(spec.id, curve);
        }
    }
}

} // namespace qrp::market
