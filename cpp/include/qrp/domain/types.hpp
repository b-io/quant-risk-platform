#pragma once
#include <string>
#include <map>
#include <vector>

namespace qrp::domain {

enum class Currency {
    CHF,
    EUR,
    GBP,
    JPY,
    USD,
    UNKNOWN
};

inline std::string to_string(Currency cc) {
    switch (cc) {
        case Currency::CHF: return "CHF";
        case Currency::EUR: return "EUR";
        case Currency::GBP: return "GBP";
        case Currency::JPY: return "JPY";
        case Currency::USD: return "USD";
        default: return "UNKNOWN";
    }
}

inline Currency from_string(const std::string& s) {
    if (s == "CHF") return Currency::CHF;
    if (s == "EUR") return Currency::EUR;
    if (s == "GBP") return Currency::GBP;
    if (s == "JPY") return Currency::JPY;
    if (s == "USD") return Currency::USD;
    return Currency::UNKNOWN;
}

enum class DayCount {
    ACT360,
    ACT365,
    ACT365F,
    ACTACT,
    ACTACT_AFB,
    ACTACT_EURO,
    ACTACT_ISDA,
    ACTACT_ISMA,
    Thirty360,
    UNKNOWN
};

enum class BusinessCalendar {
    CHF,
    JP,
    Target,
    UK,
    US,
    WeekendsOnly,
    UNKNOWN
};

enum class CurvePurpose {
    Credit,
    Discount,
    Forward,
    Forward3M,
    Forward6M,
    Volatility,
    UNKNOWN
};

enum class QuoteInstrumentType {
    Bond,
    CapFloorVol,
    CDS,
    Deposit,
    FRA,
    Future,
    IRS,
    OIS,
    SwaptionVol,
    UNKNOWN
};

enum class InterpolationType {
    CubicSpline,
    Linear,
    LogLinear,
    UNKNOWN
};

enum class BusinessDayConvention {
    Following,
    HalfMonthModifiedFollowing,
    ModifiedFollowing,
    ModifiedPreceding,
    Nearest,
    Preceding,
    Unadjusted,
    UNKNOWN
};

enum class Frequency {
    Annual,
    Bimonthly,
    Biweekly,
    Daily,
    EveryFourthMonth,
    EveryFourthWeek,
    Monthly,
    Once,
    OtherFrequency,
    Quarterly,
    Semiannual,
    Weekly,
    UNKNOWN
};

enum class DateGeneration {
    Backward,
    CDS,
    CDS2015,
    Forward,
    OldCDS,
    ThirdWednesday,
    Twentieth,
    TwentiethIMM,
    Zero,
    UNKNOWN
};

enum class QuoteType {
    BasisSwap,
    BondYield,
    CreditSpread,
    Deposit,
    FRA,
    Future,
    IRS,
    OIS,
    Swap,
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
