// Implements market snapshot validation and QuantLib market-state construction.

#include <qrp/conventions/market_convention_registry.hpp>
#include <qrp/market/market_snapshot.hpp>

#include <fmt/format.h>

#include <algorithm>
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
#include <ql/math/interpolations/loginterpolation.hpp>
#include <ql/settings.hpp>
#include <ql/termstructures/iterativebootstrap.hpp>
#include <ql/termstructures/yield/discountcurve.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/termstructures/yield/oisratehelper.hpp>
#include <ql/termstructures/yield/piecewiseyieldcurve.hpp>
#include <ql/termstructures/yield/ratehelpers.hpp>
#include <ql/time/calendars/japan.hpp>
#include <ql/time/calendars/switzerland.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/time/calendars/unitedkingdom.hpp>
#include <ql/time/calendars/unitedstates.hpp>
#include <ql/time/daycounters/actual360.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/time/daycounters/actualactual.hpp>
#include <ql/time/daycounters/thirty360.hpp>
#include <ql/time/imm.hpp>
#include <stdexcept>
#include <string>
#include <vector>

/*
Design note (see docs/architecture/CURVE_BOOTSTRAP_DESIGN.md):
- We build yield curves with
QuantLib::PiecewiseYieldCurve<Discount, LogLinear>.
- Helpers use QuantLib::ext::shared_ptr so their pointer type matches the QuantLib build in vcpkg

(QuantLib maps ext::shared_ptr to boost::shared_ptr or std::shared_ptr depending on config).
-
Quotes are QuantLib::SimpleQuote handles owned by MarketState. Any bump on a handle propagates to
the curve via QuantLib's observer mechanism (no rebuild), which is essential for fast risk/scenario
runs.
- We support both OIS (discount) and IBOR/FRA/IRS helpers. The choice of helpers reflects market
practice: discounting from OIS, forwarding from term indices. Interpolation on discount factors via
LogLinear keeps positivity of discount factors and is an industry-standard choice for production
curves.
*/
namespace qrp::market {
namespace {

/**
 * @brief Formats a curve id for market-builder diagnostics.
 */
std::string curve_id_to_string(const domain::CurveId& id) {
    return domain::to_string(id.currency) + ":" + id.family;
}

/**
 * @brief Returns whether a string looks like a canonical YYYY-MM-DD date.
 */
bool is_iso_date_string(const std::string& value) {
    if (value.size() != 10 || value[4] != '-' || value[7] != '-') {
        return false;
    }
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (i == 4 || i == 7) {
            continue;
        }
        if (value[i] < '0' || value[i] > '9') {
            return false;
        }
    }
    return true;
}

/**
 * @brief Resolves an interest-rate future expiry into the date consumed by QuantLib helpers.

 */
QuantLib::Date resolve_future_expiry_date(const domain::MarketQuote& quote,
                                          const QuantLib::Date& valuation_date,
                                          const QuantLib::Calendar& calendar) {
    if (!quote.expiry.empty() && is_iso_date_string(quote.expiry)) {
        return CurveBuilder::parse_date(quote.expiry);
    }

    const auto expiry_tenor = !quote.expiry.empty() ? quote.expiry : quote.tenor;
    const auto target_date = calendar.advance(valuation_date, CurveBuilder::parse_tenor(expiry_tenor));

    auto imm_date = QuantLib::IMM::nextDate(valuation_date, true);
    while (true) {
        const auto next_imm_date = QuantLib::IMM::nextDate(imm_date + 1, true);
        if (next_imm_date > target_date) {
            return imm_date;
        }
        imm_date = next_imm_date;
    }
}

/**
 * @brief Converts a futures quote into a QuantLib futures price.
 */
double futures_price_from_quote(double value) {
    return value > 1.0 ? value : 100.0 * (1.0 - value);
}

} // namespace

QuantLib::Date CurveBuilder::parse_date(const std::string& date_str) {
    auto is_digit = [](char c) { return c >= '0' && c <= '9'; };

    if (date_str.size() != 10 || date_str[4] != '-' || date_str[7] != '-' || !is_digit(date_str[0]) ||
        !is_digit(date_str[1]) || !is_digit(date_str[2]) || !is_digit(date_str[3]) || !is_digit(date_str[5]) ||
        !is_digit(date_str[6]) || !is_digit(date_str[8]) || !is_digit(date_str[9])) {
        throw std::invalid_argument("Invalid date format '" + date_str + "'; expected YYYY-MM-DD");
    }

    try {
        int y = std::stoi(date_str.substr(0, 4));
        int m = std::stoi(date_str.substr(5, 2));
        int d = std::stoi(date_str.substr(8, 2));
        return QuantLib::Date(d, static_cast<QuantLib::Month>(m), y);
    } catch (const std::exception& e) {
        throw std::invalid_argument(fmt::format("Invalid date '{}': {}", date_str, e.what()));
    }
}

QuantLib::Period CurveBuilder::parse_tenor(const std::string& tenor) {
    if (tenor.size() < 2) {
        throw std::invalid_argument("Invalid tenor '" + tenor + "'; expected positive tenor like 3M or 5Y");
    }

    try {
        char unit = tenor.back();
        std::string value_part = tenor.substr(0, tenor.size() - 1);
        for (char c : value_part) {
            if (c < '0' || c > '9') {
                throw std::invalid_argument("tenor value must be numeric");
            }
        }
        int value = std::stoi(value_part);
        if (value <= 0) {
            throw std::invalid_argument("tenor value must be positive");
        }

        switch (unit) {
            case 'D':
                return QuantLib::Period(value, QuantLib::Days);
            case 'W':
                return QuantLib::Period(value, QuantLib::Weeks);
            case 'M':
                return QuantLib::Period(value, QuantLib::Months);
            case 'Y':
                return QuantLib::Period(value, QuantLib::Years);
            default:
                throw std::invalid_argument("tenor unit must be one of D, W, M, Y");
        }
    } catch (const std::exception& e) {
        throw std::invalid_argument(fmt::format("Invalid tenor '{}': {}", tenor, e.what()));
    }
}

QuantLib::DayCounter CurveBuilder::parse_day_count(domain::DayCount dc) {
    switch (dc) {
        case domain::DayCount::ACT360:
            return QuantLib::Actual360();
        case domain::DayCount::ACT365:
        case domain::DayCount::ACT365F:
            return QuantLib::Actual365Fixed();
        case domain::DayCount::ACTACT:
        case domain::DayCount::ACTACT_AFB:
            return QuantLib::ActualActual(QuantLib::ActualActual::AFB);
        case domain::DayCount::ACTACT_EURO:
            return QuantLib::ActualActual(QuantLib::ActualActual::Euro);
        case domain::DayCount::ACTACT_ISDA:
            return QuantLib::ActualActual(QuantLib::ActualActual::ISDA);
        case domain::DayCount::ACTACT_ISMA:
            return QuantLib::ActualActual(QuantLib::ActualActual::ISMA);
        case domain::DayCount::Thirty360:
            return QuantLib::Thirty360(QuantLib::Thirty360::BondBasis);
        default:
            return QuantLib::Actual365Fixed();
    }
}

QuantLib::Calendar CurveBuilder::parse_calendar(domain::BusinessCalendar cal) {
    switch (cal) {
        case domain::BusinessCalendar::CHF:
            return QuantLib::Switzerland();
        case domain::BusinessCalendar::JP:
            return QuantLib::Japan();
        case domain::BusinessCalendar::Target:
            return QuantLib::TARGET();
        case domain::BusinessCalendar::UK:
            return QuantLib::UnitedKingdom();
        case domain::BusinessCalendar::US:
            return QuantLib::UnitedStates(QuantLib::UnitedStates::GovernmentBond);
        case domain::BusinessCalendar::WeekendsOnly:
            return QuantLib::NullCalendar();
        default:
            return QuantLib::TARGET();
    }
}

double CurveBuilder::tenor_to_years(const std::string& tenor) {
    if (tenor.empty())
        return 0.0;
    QuantLib::Period p = parse_tenor(tenor);
    if (p.units() == QuantLib::Days)
        return p.length() / 365.25;
    if (p.units() == QuantLib::Weeks)
        return p.length() / 52.1786;
    if (p.units() == QuantLib::Months)
        return p.length() / 12.0;
    if (p.units() == QuantLib::Years)
        return static_cast<double>(p.length());
    return 0.0;
}

QuantLib::BusinessDayConvention CurveBuilder::parse_business_day_convention(domain::BusinessDayConvention bdc) {
    switch (bdc) {
        case domain::BusinessDayConvention::Following:
            return QuantLib::Following;
        case domain::BusinessDayConvention::HalfMonthModifiedFollowing:
            return QuantLib::HalfMonthModifiedFollowing;
        case domain::BusinessDayConvention::ModifiedFollowing:
            return QuantLib::ModifiedFollowing;
        case domain::BusinessDayConvention::ModifiedPreceding:
            return QuantLib::ModifiedPreceding;
        case domain::BusinessDayConvention::Nearest:
            return QuantLib::Nearest;
        case domain::BusinessDayConvention::Preceding:
            return QuantLib::Preceding;
        case domain::BusinessDayConvention::Unadjusted:
            return QuantLib::Unadjusted;
        default:
            return QuantLib::Following;
    }
}

QuantLib::Frequency CurveBuilder::parse_frequency(domain::Frequency freq) {
    switch (freq) {
        case domain::Frequency::Annual:
            return QuantLib::Annual;
        case domain::Frequency::Bimonthly:
            return QuantLib::Bimonthly;
        case domain::Frequency::Biweekly:
            return QuantLib::Biweekly;
        case domain::Frequency::Daily:
            return QuantLib::Daily;
        case domain::Frequency::EveryFourthMonth:
            return QuantLib::EveryFourthMonth;
        case domain::Frequency::EveryFourthWeek:
            return QuantLib::EveryFourthWeek;
        case domain::Frequency::Monthly:
            return QuantLib::Monthly;
        case domain::Frequency::Once:
            return QuantLib::Once;
        case domain::Frequency::OtherFrequency:
            return QuantLib::OtherFrequency;
        case domain::Frequency::Quarterly:
            return QuantLib::Quarterly;
        case domain::Frequency::Semiannual:
            return QuantLib::Semiannual;
        case domain::Frequency::Weekly:
            return QuantLib::Weekly;
        default:
            return QuantLib::Annual;
    }
}

QuantLib::DateGeneration::Rule CurveBuilder::parse_date_generation(domain::DateGeneration rule) {
    switch (rule) {
        case domain::DateGeneration::Backward:
            return QuantLib::DateGeneration::Backward;
        case domain::DateGeneration::CDS:
            return QuantLib::DateGeneration::CDS;
        case domain::DateGeneration::CDS2015:
            return QuantLib::DateGeneration::CDS2015;
        case domain::DateGeneration::Forward:
            return QuantLib::DateGeneration::Forward;
        case domain::DateGeneration::OldCDS:
            return QuantLib::DateGeneration::OldCDS;
        case domain::DateGeneration::ThirdWednesday:
            return QuantLib::DateGeneration::ThirdWednesday;
        case domain::DateGeneration::Twentieth:
            return QuantLib::DateGeneration::Twentieth;
        case domain::DateGeneration::TwentiethIMM:
            return QuantLib::DateGeneration::TwentiethIMM;
        case domain::DateGeneration::Zero:
            return QuantLib::DateGeneration::Zero;
        default:
            return QuantLib::DateGeneration::Forward;
    }
}

QuantLib::ext::shared_ptr<QuantLib::OvernightIndex>
CurveBuilder::create_overnight_index(domain::Currency currency,
                                     const QuantLib::Handle<QuantLib::YieldTermStructure>& h) {

    switch (currency) {
        case domain::Currency::CHF:
            return QuantLib::ext::make_shared<QuantLib::Saron>(h);
        case domain::Currency::EUR:
            return QuantLib::ext::make_shared<QuantLib::Estr>(h);
        case domain::Currency::GBP:
            return QuantLib::ext::make_shared<QuantLib::Sonia>(h);
        case domain::Currency::JPY:
            return QuantLib::ext::make_shared<QuantLib::Tonar>(h);
        case domain::Currency::USD:
            return QuantLib::ext::make_shared<QuantLib::Sofr>(h);
        default:
            return QuantLib::ext::make_shared<QuantLib::Sofr>(h);
    }
}

QuantLib::ext::shared_ptr<QuantLib::IborIndex>
CurveBuilder::create_ibor_index(domain::Currency currency,
                                const QuantLib::Period& tenor,
                                const QuantLib::Handle<QuantLib::YieldTermStructure>& h) {

    switch (currency) {
        case domain::Currency::CHF:
            return QuantLib::ext::make_shared<QuantLib::CHFLibor>(tenor, h);
        case domain::Currency::EUR:
            return QuantLib::ext::make_shared<QuantLib::Euribor>(tenor, h);
        case domain::Currency::GBP:
            return QuantLib::ext::make_shared<QuantLib::GBPLibor>(tenor, h);
        case domain::Currency::JPY:
            return QuantLib::ext::make_shared<QuantLib::JPYLibor>(tenor, h);
        case domain::Currency::USD:
            return QuantLib::ext::make_shared<QuantLib::USDLibor>(tenor, h);
        default:
            return QuantLib::ext::make_shared<QuantLib::USDLibor>(tenor, h);
    }
}

bool CurveBuilder::supports_rates_curve_purpose(domain::CurvePurpose purpose) {
    switch (purpose) {
        case domain::CurvePurpose::Discount:
        case domain::CurvePurpose::Forward:
        case domain::CurvePurpose::Forward3M:
        case domain::CurvePurpose::Forward6M:
        case domain::CurvePurpose::OISDiscount:
            return true;
        default:
            return false;
    }
}

bool CurveBuilder::supports_rates_curve_quote(domain::QuoteInstrumentType type) {
    switch (type) {
        case domain::QuoteInstrumentType::Deposit:
        case domain::QuoteInstrumentType::FRA:
        case domain::QuoteInstrumentType::InterestRateFuture:
        case domain::QuoteInstrumentType::IRS:
        case domain::QuoteInstrumentType::OIS:
            return true;
        default:
            return false;
    }
}

bool CurveBuilder::supports_rates_vol_quote(domain::QuoteInstrumentType type) {
    switch (type) {
        case domain::QuoteInstrumentType::CapFloorVol:
        case domain::QuoteInstrumentType::SwaptionVol:
            return true;
        default:
            return false;
    }
}

QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure>
CurveBuilder::build_rate_curve(const domain::CurveSpec& spec,
                               const std::map<std::string, domain::MarketQuote>& quotes,
                               const QuantLib::Date& valuation_date,
                               std::shared_ptr<MarketState> state_ptr) {

    std::vector<QuantLib::ext::shared_ptr<QuantLib::RateHelper>> helpers;
    QuantLib::Handle<QuantLib::YieldTermStructure> empty_term_structure;

    for (const auto& q_id : spec.quote_ids) {
        if (!quotes.contains(q_id))
            continue;
        const auto& q = quotes.at(q_id);
        if (!supports_rates_curve_quote(q.instrument_type))
            continue;

        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> simple_quote;
        if (state_ptr && state_ptr->get_quote_handle(q_id)) {
            simple_quote = state_ptr->get_quote_handle(q_id);
        } else {
            if (state_ptr) {
                state_ptr->add_quote(q);
                simple_quote = state_ptr->get_quote_handle(q_id);
            } else {
                simple_quote = QuantLib::ext::make_shared<QuantLib::SimpleQuote>(q.value);
            }
        }

        auto quote_handle = QuantLib::Handle<QuantLib::Quote>(simple_quote);
        auto tenor = parse_tenor(q.tenor);

        // Resolve conventions (prefer per-quote index_family, fallback by type)
        std::string idx_family =
            q.index_family.empty()
                ? (q.instrument_type == domain::QuoteInstrumentType::OIS ? std::string("OIS") : std::string("IBOR_3M"))
                : q.index_family;
        auto& reg = qrp::conventions::MarketConventionRegistry::instance();
        auto conv = reg.get_rates_convention(q.currency, idx_family);

        auto cal = parse_calendar(q.calendar != domain::BusinessCalendar::UNKNOWN ? q.calendar : conv.calendar);
        auto bdc = parse_business_day_convention(
            q.bdc != domain::BusinessDayConvention::UNKNOWN ? q.bdc : conv.business_day_convention);
        auto dc = parse_day_count(q.day_count != domain::DayCount::UNKNOWN ? q.day_count : spec.day_count);
        int settlement_days = q.settlement_days >= 0 ? q.settlement_days : conv.settlement_days;

        if (q.instrument_type == domain::QuoteInstrumentType::Deposit) {
            helpers.push_back(QuantLib::ext::make_shared<QuantLib::DepositRateHelper>(quote_handle,
                                                                                      tenor,
                                                                                      settlement_days,
                                                                                      cal,
                                                                                      bdc,
                                                                                      false,
                                                                                      dc));
        } else if (q.instrument_type == domain::QuoteInstrumentType::FRA) {
            auto index = create_ibor_index(q.currency,
                                           QuantLib::Period(3, QuantLib::Months),
                                           empty_term_structure); // default 3M index convention
            helpers.push_back(QuantLib::ext::make_shared<QuantLib::FraRateHelper>(quote_handle, tenor, index));
        } else if (q.instrument_type == domain::QuoteInstrumentType::InterestRateFuture) {
            auto futures_quote_handle = quote_handle;
            if (q.value <= 1.0) {
                auto futures_price_quote =
                    QuantLib::ext::make_shared<QuantLib::SimpleQuote>(futures_price_from_quote(q.value));
                futures_quote_handle = QuantLib::Handle<QuantLib::Quote>(futures_price_quote);
            }
            const auto expiry_date = resolve_future_expiry_date(q, valuation_date, cal);
            auto index = create_ibor_index(q.currency, QuantLib::Period(3, QuantLib::Months), empty_term_structure);
            helpers.push_back(
                QuantLib::ext::make_shared<QuantLib::FuturesRateHelper>(futures_quote_handle, expiry_date, index));
        } else if (q.instrument_type == domain::QuoteInstrumentType::IRS) {
            auto index = create_ibor_index(q.currency,
                                           QuantLib::Period(3, QuantLib::Months),
                                           empty_term_structure); // default 3M index convention
            auto fixed_freq = parse_frequency(conv.fixed_leg_frequency);
            auto fixed_dc = parse_day_count(conv.fixed_leg_day_count);
            helpers.push_back(QuantLib::ext::make_shared<QuantLib::SwapRateHelper>(quote_handle,
                                                                                   tenor,
                                                                                   cal,
                                                                                   fixed_freq,
                                                                                   bdc,
                                                                                   fixed_dc,
                                                                                   index));
        } else if (q.instrument_type == domain::QuoteInstrumentType::OIS) {
            auto index = create_overnight_index(q.currency, empty_term_structure);
            helpers.push_back(
                QuantLib::ext::make_shared<QuantLib::OISRateHelper>(settlement_days, tenor, quote_handle, index));
        }
    }

    if (helpers.empty())
        return nullptr;

    using CurveT = QuantLib::PiecewiseYieldCurve<QuantLib::Discount, QuantLib::LogLinear>;
    QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> curve =
        QuantLib::ext::make_shared<CurveT>(valuation_date, helpers, parse_day_count(spec.day_count));
    curve->enableExtrapolation();
    if (state_ptr) {
        state_ptr->add_curve(spec.id, curve);
    }
    return curve;
}

RatesMarketBuildResult RatesMarketBuilder::build(const domain::MarketSnapshot& dto) {
    domain::throw_if_market_snapshot_not_ready(dto);

    QuantLib::Date val_date = CurveBuilder::parse_date(dto.valuation_date);
    QuantLib::Settings::instance().evaluationDate() = val_date;

    RatesMarketBuildResult result;
    result.state = std::make_shared<MarketState>(val_date);

    std::map<std::string, domain::MarketQuote> quote_map;
    for (const auto& q : dto.quotes) {
        quote_map[q.id] = q;
        result.state->add_quote(q);
        if (CurveBuilder::supports_rates_vol_quote(q.instrument_type)) {
            result.rates_vol_quote_ids.push_back(q.id);
        }
    }

    std::vector<std::string> failures;
    for (const auto& spec : dto.curves) {
        CurveBuildResult curve_result;
        curve_result.id = spec.id;
        curve_result.purpose = spec.purpose;
        curve_result.quote_ids = spec.quote_ids;

        if (!CurveBuilder::supports_rates_curve_purpose(spec.purpose)) {
            curve_result.status_message = "Skipped: curve purpose is not supported by the rates builder";
            result.curve_results.push_back(std::move(curve_result));
            continue;
        }

        try {
            auto curve = CurveBuilder::build_rate_curve(spec, quote_map, val_date, result.state);
            if (curve) {
                curve_result.built = true;
                curve_result.status_message = "Built";
            } else {
                curve_result.status_message = "No supported rates curve helper quotes were available";
                failures.push_back(curve_id_to_string(spec.id) + ": " + curve_result.status_message);
            }
        } catch (const std::exception& e) {
            curve_result.status_message = e.what();
            failures.push_back(curve_id_to_string(spec.id) + ": " + curve_result.status_message);
        }

        result.curve_results.push_back(std::move(curve_result));
    }

    for (const auto& [index_name, date_fixings] : dto.fixings) {
        for (const auto& [date_str, value] : date_fixings) {
            result.state->add_fixing(index_name, CurveBuilder::parse_date(date_str), value);
        }
    }

    if (!failures.empty()) {
        std::string message = "Rates market build failed";
        for (const auto& failure : failures) {
            message += "; " + failure;
        }
        throw std::runtime_error(message);
    }

    return result;
}

MarketSnapshot::MarketSnapshot(const domain::MarketSnapshot& dto) {
    auto result = RatesMarketBuilder::build(dto);
    state_ = std::move(result.state);
}

domain::MarketSnapshot MarketState::capture_snapshot() const {
    domain::MarketSnapshot snapshot;
    snapshot.valuation_date = fmt::format("{:4d}-{:02d}-{:02d}",
                                          (int)valuation_date_.year(),
                                          (int)valuation_date_.month(),
                                          (int)valuation_date_.dayOfMonth());

    for (const auto& [id, handle] : quote_handles_) {
        domain::MarketQuote q;
        if (auto it = quote_metadata_.find(id); it != quote_metadata_.end()) {
            q = it->second;
        } else {
            q.id = id;
        }
        q.value = handle->value();
        snapshot.quotes.push_back(std::move(q));
    }

    for (const auto& [index_name, date_map] : fixings_) {
        for (const auto& [date, value] : date_map) {
            std::string date_str =
                fmt::format("{:4d}-{:02d}-{:02d}", (int)date.year(), (int)date.month(), (int)date.dayOfMonth());
            snapshot.fixings[index_name][date_str] = value;
        }
    }

    return snapshot;
}

} // namespace qrp::market
