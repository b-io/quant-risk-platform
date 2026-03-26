#pragma once
#include <string>
#include <map>
#include <vector>

namespace qrp::domain {

enum class Currency {
    USD,
    EUR,
    GBP,
    CHF,
    JPY,
    UNKNOWN
};

inline std::string to_string(Currency cc) {
    switch (cc) {
        case Currency::USD: return "USD";
        case Currency::EUR: return "EUR";
        case Currency::GBP: return "GBP";
        case Currency::CHF: return "CHF";
        case Currency::JPY: return "JPY";
        default: return "UNKNOWN";
    }
}

inline Currency from_string(const std::string& s) {
    if (s == "USD") return Currency::USD;
    if (s == "EUR") return Currency::EUR;
    if (s == "GBP") return Currency::GBP;
    if (s == "CHF") return Currency::CHF;
    if (s == "JPY") return Currency::JPY;
    return Currency::UNKNOWN;
}

enum class DayCount {
    ACT360,
    ACT365,
    Thirty360,
    ACTACT,
    ACT365F,
    ACTACT_ISDA,
    ACTACT_ISMA,
    ACTACT_AFB,
    ACTACT_EURO,
    UNKNOWN
};

enum class BusinessCalendar {
    Target,
    US,
    UK,
    CHF,
    JP,
    WeekendsOnly,
    UNKNOWN
};

enum class CurvePurpose {
    Discount,
    Forward,
    Forward3M,
    Forward6M,
    Credit,
    Volatility,
    UNKNOWN
};

enum class QuoteInstrumentType {
    Deposit,
    OIS,
    IRS,
    FRA,
    Future,
    Bond,
    CDS,
    CapFloorVol,
    SwaptionVol,
    UNKNOWN
};

enum class InterpolationType {
    CubicSpline,
    LogLinear,
    Linear,
    UNKNOWN
};

enum class BusinessDayConvention {
    Following,
    ModifiedFollowing,
    Preceding,
    ModifiedPreceding,
    Unadjusted,
    HalfMonthModifiedFollowing,
    Nearest,
    UNKNOWN
};

enum class Frequency {
    Once,
    Annual,
    Semiannual,
    EveryFourthMonth,
    Quarterly,
    Bimonthly,
    Monthly,
    EveryFourthWeek,
    Biweekly,
    Weekly,
    Daily,
    OtherFrequency,
    UNKNOWN
};

enum class DateGeneration {
    Backward,
    Forward,
    Zero,
    ThirdWednesday,
    Twentieth,
    TwentiethIMM,
    OldCDS,
    CDS,
    CDS2015,
    UNKNOWN
};

enum class QuoteType {
    Deposit,
    OIS,
    IRS,
    FRA,
    Future,
    Swap,
    BasisSwap,
    BondYield,
    CreditSpread,
    UNKNOWN
};

struct MarketQuote {
    std::string id;
    QuoteInstrumentType instrument_type = QuoteInstrumentType::UNKNOWN;
    Currency currency = Currency::UNKNOWN;
    std::string tenor;
    double value = 0.0;
    
    // Enriching schema
    std::string instrument_family; // ois, ibor_swap, etc.
    std::string index_family;      // IBOR_3M, etc.
    DayCount day_count = DayCount::UNKNOWN;
    BusinessCalendar calendar = BusinessCalendar::UNKNOWN;
    BusinessDayConvention bdc = BusinessDayConvention::UNKNOWN;
    int settlement_days = -1;
};

struct CurveId {
    Currency currency;
    std::string family; // e.g., "OIS", "IBOR_3M"
    
    bool operator<(const CurveId& other) const {
        if (currency != other.currency) return currency < other.currency;
        return family < other.family;
    }
};

struct CurveSpec {
    CurveId id;
    CurvePurpose purpose = CurvePurpose::UNKNOWN;
    std::vector<std::string> quote_ids;
    DayCount day_count = DayCount::UNKNOWN;
    BusinessCalendar calendar = BusinessCalendar::UNKNOWN;
    InterpolationType interpolation = InterpolationType::UNKNOWN;
};

} // namespace qrp::domain
