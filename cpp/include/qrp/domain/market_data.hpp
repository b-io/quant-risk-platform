#pragma once
#include <qrp/domain/types.hpp>
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

namespace qrp::domain {

inline void from_json(const nlohmann::json& j, Currency& c) {
    c = from_string(j.get<std::string>());
}

inline void from_json(const nlohmann::json& j, QuoteType& q) {
    std::string s = j.get<std::string>();
    if (s == "Deposit") q = QuoteType::Deposit;
    else if (s == "OIS") q = QuoteType::OIS;
    else if (s == "IRS") q = QuoteType::IRS;
    else if (s == "FRA") q = QuoteType::FRA;
    else if (s == "Future") q = QuoteType::Future;
    else if (s == "Swap") q = QuoteType::Swap;
    else if (s == "BasisSwap") q = QuoteType::BasisSwap;
    else if (s == "BondYield") q = QuoteType::BondYield;
    else if (s == "CreditSpread") q = QuoteType::CreditSpread;
    else q = QuoteType::UNKNOWN;
}

inline void from_json(const nlohmann::json& j, DayCount& d) {
    std::string s = j.get<std::string>();
    if (s == "ACT360") d = DayCount::ACT360;
    else if (s == "ACT365") d = DayCount::ACT365;
    else if (s == "ACT365F") d = DayCount::ACT365F;
    else if (s == "Thirty360") d = DayCount::Thirty360;
    else if (s == "ACTACT") d = DayCount::ACTACT;
    else if (s == "ACTACT_ISDA") d = DayCount::ACTACT_ISDA;
    else if (s == "ACTACT_ISMA") d = DayCount::ACTACT_ISMA;
    else if (s == "ACTACT_AFB") d = DayCount::ACTACT_AFB;
    else if (s == "ACTACT_EURO") d = DayCount::ACTACT_EURO;
    else d = DayCount::UNKNOWN;
}

inline void from_json(const nlohmann::json& j, BusinessCalendar& b) {
    std::string s = j.get<std::string>();
    if (s == "Target") b = BusinessCalendar::Target;
    else if (s == "US" || s == "USA") b = BusinessCalendar::US;
    else if (s == "UK" || s == "GBR") b = BusinessCalendar::UK;
    else if (s == "CHF" || s == "CHE") b = BusinessCalendar::CHF;
    else if (s == "JP" || s == "JPN") b = BusinessCalendar::JP;
    else if (s == "WeekendsOnly") b = BusinessCalendar::WeekendsOnly;
    else b = BusinessCalendar::UNKNOWN;
}

inline void from_json(const nlohmann::json& j, BusinessDayConvention& b) {
    std::string s = j.get<std::string>();
    if (s == "Following") b = BusinessDayConvention::Following;
    else if (s == "ModifiedFollowing") b = BusinessDayConvention::ModifiedFollowing;
    else if (s == "Preceding") b = BusinessDayConvention::Preceding;
    else if (s == "ModifiedPreceding") b = BusinessDayConvention::ModifiedPreceding;
    else if (s == "Unadjusted") b = BusinessDayConvention::Unadjusted;
    else if (s == "HalfMonthModifiedFollowing") b = BusinessDayConvention::HalfMonthModifiedFollowing;
    else if (s == "Nearest") b = BusinessDayConvention::Nearest;
    else b = BusinessDayConvention::UNKNOWN;
}

inline void from_json(const nlohmann::json& j, Frequency& f) {
    std::string s = j.get<std::string>();
    if (s == "Annual") f = Frequency::Annual;
    else if (s == "Semiannual") f = Frequency::Semiannual;
    else if (s == "Quarterly") f = Frequency::Quarterly;
    else if (s == "Monthly") f = Frequency::Monthly;
    else if (s == "Weekly") f = Frequency::Weekly;
    else if (s == "Daily") f = Frequency::Daily;
    else if (s == "Once") f = Frequency::Once;
    else f = Frequency::UNKNOWN;
}

inline void from_json(const nlohmann::json& j, DateGeneration& d) {
    std::string s = j.get<std::string>();
    if (s == "Backward") d = DateGeneration::Backward;
    else if (s == "Forward") d = DateGeneration::Forward;
    else if (s == "Zero") d = DateGeneration::Zero;
    else if (s == "ThirdWednesday") d = DateGeneration::ThirdWednesday;
    else if (s == "Twentieth") d = DateGeneration::Twentieth;
    else if (s == "TwentiethIMM") d = DateGeneration::TwentiethIMM;
    else if (s == "OldCDS") d = DateGeneration::OldCDS;
    else if (s == "CDS") d = DateGeneration::CDS;
    else if (s == "CDS2015") d = DateGeneration::CDS2015;
    else d = DateGeneration::UNKNOWN;
}

inline void from_json(const nlohmann::json& j, InterpolationType& i) {
    std::string s = j.get<std::string>();
    if (s == "Linear") i = InterpolationType::Linear;
    else if (s == "LogLinear") i = InterpolationType::LogLinear;
    else if (s == "CubicSpline") i = InterpolationType::CubicSpline;
    else i = InterpolationType::UNKNOWN;
}

inline void from_json(const nlohmann::json& j, MarketQuote& q) {
    j.at("id").get_to(q.id);
    j.at("type").get_to(q.type);
    j.at("currency").get_to(q.currency);
    j.at("tenor").get_to(q.tenor);
    j.at("value").get_to(q.value);
    
    if (j.contains("instrument_family")) j.at("instrument_family").get_to(q.instrument_family);
    if (j.contains("index_family")) j.at("index_family").get_to(q.index_family);
    if (j.contains("day_count")) j.at("day_count").get_to(q.day_count);
    if (j.contains("calendar")) j.at("calendar").get_to(q.calendar);
    if (j.contains("bdc")) j.at("bdc").get_to(q.bdc);
    if (j.contains("settlement_days")) j.at("settlement_days").get_to(q.settlement_days);
}

inline void from_json(const nlohmann::json& j, CurveId& id) {
    j.at("currency").get_to(id.currency);
    j.at("family").get_to(id.family);
}

inline void from_json(const nlohmann::json& j, CurveSpec& s) {
    j.at("id").get_to(s.id);
    j.at("quote_ids").get_to(s.quote_ids);
    j.at("day_count").get_to(s.day_count);
    j.at("calendar").get_to(s.calendar);
    j.at("interpolation").get_to(s.interpolation);
}

struct MarketSnapshot {
    std::string valuation_date;
    std::vector<MarketQuote> quotes;
    std::vector<CurveSpec> curves;
};

inline void from_json(const nlohmann::json& j, MarketSnapshot& m) {
    j.at("valuation_date").get_to(m.valuation_date);
    j.at("quotes").get_to(m.quotes);
    j.at("curves").get_to(m.curves);
}

} // namespace qrp::domain
