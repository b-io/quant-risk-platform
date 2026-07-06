#pragma once

// Defines shared domain enums, canonical strings, quote metadata, and curve identifiers.

#include <string>
#include <vector>

namespace qrp::domain {

/**
 * @brief ISO-style currency identifiers supported by the platform core.
 */
enum class Currency {
    CHF,
    EUR,
    GBP,
    JPY,
    USD,
    UNKNOWN
};

/**
 * @brief Converts a currency enum to its canonical external string.
 */
inline std::string to_string(Currency cc) {
    switch (cc) {
        case Currency::CHF:
            return "CHF";
        case Currency::EUR:
            return "EUR";
        case Currency::GBP:
            return "GBP";
        case Currency::JPY:
            return "JPY";
        case Currency::USD:
            return "USD";
        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Parses a canonical currency string, returning UNKNOWN for unsupported values.
 */
inline Currency from_string(const std::string& s) {
    if (s == "CHF")
        return Currency::CHF;
    if (s == "EUR")
        return Currency::EUR;
    if (s == "GBP")
        return Currency::GBP;
    if (s == "JPY")
        return Currency::JPY;
    if (s == "USD")
        return Currency::USD;
    return Currency::UNKNOWN;
}

/**
 * @brief Day-count conventions used by schedules, curves, and cashflows.
 */
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

/**
 * @brief Business-calendar families used by convention resolution.
 */
enum class BusinessCalendar {
    CHF,
    JP,
    Target,
    UK,
    US,
    WeekendsOnly,
    UNKNOWN
};

/**
 * @brief Business purpose of a curve or surface in the market snapshot.
 */
enum class CurvePurpose {
    // Commodity
    CommodityForward,
    CommodityVolatility,

    // Credit
    Credit,
    CreditSpread,
    Hazard,
    Recovery,

    // Equity
    EquityBorrow,
    EquityDividend,
    EquityVolatility,

    // FX
    FXForward,
    FXVolatility,

    // Generic
    Volatility,

    // Rates
    Discount,
    Forward,
    Forward3M,
    Forward6M,
    Inflation,
    OISDiscount,

    UNKNOWN
};

/**
 * @brief Quote instrument taxonomy for raw market inputs.
 */
enum class QuoteInstrumentType {
    // Commodity
    CommodityForward,
    CommodityFuture,
    CommoditySpot,
    CommodityVol,
    ConvenienceYield,

    // Credit
    Bond,
    BondPrice,
    BondSpread,
    CDS,
    CreditIndex,
    CreditSpread,
    HazardRate,
    RecoveryRate,

    // Equity
    BorrowRate,
    DividendYield,
    EquitySpot,
    EquityVol,

    // FX
    FXForward,
    FXForwardPoint,
    FXSpot,
    FXVol,

    // Generic
    Future,

    // Rates
    CapFloorVol,
    Deposit,
    FRA,
    InterestRateFuture,
    IRS,
    OIS,
    SwaptionVol,

    UNKNOWN
};

/**
 * @brief Interpolation policies for curves and surfaces.
 */
enum class InterpolationType {
    CubicSpline,
    Linear,
    LogLinear,
    UNKNOWN
};

/**
 * @brief Business-day adjustment rules used by schedules and curve helpers.
 */
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

/**
 * @brief Schedule and coupon frequencies used across products.
 */
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

/**
 * @brief Date-generation rules used when constructing schedules.
 */
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

/**
 * @brief Economic quote taxonomy independent of source instrument format.
 */
enum class QuoteType {
    // Commodity
    CommodityForward,

    // Credit
    BondYield,
    CreditSpread,
    HazardRate,
    RecoveryRate,

    // Equity
    BorrowRate,
    DividendYield,
    EquitySpot,

    // FX
    FXForward,
    FXForwardPoint,
    FXSpot,

    // Generic
    Future,
    Price,
    Volatility,

    // Rates
    BasisSwap,
    Deposit,
    FRA,
    InterestRateFuture,
    IRS,
    OIS,
    Swap,

    UNKNOWN
};

/**
 * @brief Raw market quote with normalized taxonomy, provenance, and convention metadata.
 */
struct MarketQuote {
    std::string id;                                                     // Stable quote identifier.
    QuoteInstrumentType instrument_type = QuoteInstrumentType::UNKNOWN; // Instrument family.
    Currency currency = Currency::UNKNOWN;                              // Quote currency.
    std::string tenor;                                                  // Quote tenor when applicable.
    double value = 0.0;                                                 // Numeric market quote value.

    // Enriching schema
    std::string risk_factor_id;                            // Linked risk factor id.
    QuoteType quote_type = QuoteType::UNKNOWN;             // Economic quote type.
    std::string underlier;                                 // e.g., FX pair, equity ticker, issuer/index, commodity hub
    std::string expiry;                                    // option/future/surface expiry bucket where applicable
    std::string strike;                                    // strike, delta bucket, or moneyness label where applicable
    std::string instrument_family;                         // ois, ibor_swap, etc.
    std::string index_family;                              // IBOR_3M, etc.
    DayCount day_count = DayCount::UNKNOWN;                // Quote accrual day-count convention.
    BusinessCalendar calendar = BusinessCalendar::UNKNOWN; // Quote business calendar.
    BusinessDayConvention bdc = BusinessDayConvention::UNKNOWN; // Quote date-adjustment convention.
    int settlement_days = -1;                                   // Settlement lag in business days.
    std::string market_ts;                                      // Timestamp when the quote was valid in market time.
    std::string recorded_ts;                                    // Timestamp when the platform recorded the quote.
    std::string source_name;                                    // Source system or vendor name.
    std::string source_ts;                                      // Source-provided timestamp.
    int stale_after_days = -1;                                  // Quote-specific staleness threshold.
};

/**
 * @brief Stable identifier for a market curve family in a currency.
 */
struct CurveId {
    Currency currency;  // Curve currency.
    std::string family; // e.g., "OIS", "IBOR_3M"

    /**
     * @brief Orders curve ids for use as keys in standard ordered containers.
     */
    bool operator<(const CurveId& other) const {
        if (currency != other.currency)
            return currency < other.currency;
        return family < other.family;
    }
};

/**
 * @brief Construction specification for a curve or future curve-like market object.
 */
struct CurveSpec {
    CurveId id;                                                   // Curve identifier.
    CurvePurpose purpose = CurvePurpose::UNKNOWN;                 // Intended curve usage.
    std::vector<std::string> quote_ids;                           // Ordered input quote ids.
    DayCount day_count = DayCount::UNKNOWN;                       // Curve day-count convention.
    BusinessCalendar calendar = BusinessCalendar::UNKNOWN;        // Curve business calendar.
    InterpolationType interpolation = InterpolationType::UNKNOWN; // Node interpolation method.
    std::string construction_family;                              // Curve construction family or template.
    std::string collateral_curve_id;                              // Collateral curve id for discounting dependencies.
    std::string discount_curve_id;                                // Discount curve id for forwarding dependencies.
    std::string metadata_json;                                    // Structured construction metadata.
};

/**
 * @brief Converts a curve purpose enum to its canonical external string.
 */
inline std::string to_string(CurvePurpose purpose) {
    switch (purpose) {
        // Commodity
        case CurvePurpose::CommodityForward:
            return "CommodityForward";
        case CurvePurpose::CommodityVolatility:
            return "CommodityVolatility";

        // Credit
        case CurvePurpose::Credit:
            return "Credit";
        case CurvePurpose::CreditSpread:
            return "CreditSpread";
        case CurvePurpose::Hazard:
            return "Hazard";
        case CurvePurpose::Recovery:
            return "Recovery";

        // Equity
        case CurvePurpose::EquityBorrow:
            return "EquityBorrow";
        case CurvePurpose::EquityDividend:
            return "EquityDividend";
        case CurvePurpose::EquityVolatility:
            return "EquityVolatility";

        // FX
        case CurvePurpose::FXForward:
            return "FXForward";
        case CurvePurpose::FXVolatility:
            return "FXVolatility";

        // Generic
        case CurvePurpose::Volatility:
            return "Volatility";

        // Rates
        case CurvePurpose::Discount:
            return "Discount";
        case CurvePurpose::Forward:
            return "Forward";
        case CurvePurpose::Forward3M:
            return "Forward3M";
        case CurvePurpose::Forward6M:
            return "Forward6M";
        case CurvePurpose::Inflation:
            return "Inflation";
        case CurvePurpose::OISDiscount:
            return "OISDiscount";

        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Converts a quote instrument type enum to its canonical external string.
 */
inline std::string to_string(QuoteInstrumentType type) {
    switch (type) {
        // Commodity
        case QuoteInstrumentType::CommodityForward:
            return "CommodityForward";
        case QuoteInstrumentType::CommodityFuture:
            return "CommodityFuture";
        case QuoteInstrumentType::CommoditySpot:
            return "CommoditySpot";
        case QuoteInstrumentType::CommodityVol:
            return "CommodityVol";
        case QuoteInstrumentType::ConvenienceYield:
            return "ConvenienceYield";

        // Credit
        case QuoteInstrumentType::Bond:
            return "Bond";
        case QuoteInstrumentType::BondPrice:
            return "BondPrice";
        case QuoteInstrumentType::BondSpread:
            return "BondSpread";
        case QuoteInstrumentType::CDS:
            return "CDS";
        case QuoteInstrumentType::CreditIndex:
            return "CreditIndex";
        case QuoteInstrumentType::CreditSpread:
            return "CreditSpread";
        case QuoteInstrumentType::HazardRate:
            return "HazardRate";
        case QuoteInstrumentType::RecoveryRate:
            return "RecoveryRate";

        // Equity
        case QuoteInstrumentType::BorrowRate:
            return "BorrowRate";
        case QuoteInstrumentType::DividendYield:
            return "DividendYield";
        case QuoteInstrumentType::EquitySpot:
            return "EquitySpot";
        case QuoteInstrumentType::EquityVol:
            return "EquityVol";

        // FX
        case QuoteInstrumentType::FXForward:
            return "FXForward";
        case QuoteInstrumentType::FXForwardPoint:
            return "FXForwardPoint";
        case QuoteInstrumentType::FXSpot:
            return "FXSpot";
        case QuoteInstrumentType::FXVol:
            return "FXVol";

        // Generic
        case QuoteInstrumentType::Future:
            return "Future";

        // Rates
        case QuoteInstrumentType::CapFloorVol:
            return "CapFloorVol";
        case QuoteInstrumentType::Deposit:
            return "Deposit";
        case QuoteInstrumentType::FRA:
            return "FRA";
        case QuoteInstrumentType::InterestRateFuture:
            return "InterestRateFuture";
        case QuoteInstrumentType::IRS:
            return "IRS";
        case QuoteInstrumentType::OIS:
            return "OIS";
        case QuoteInstrumentType::SwaptionVol:
            return "SwaptionVol";

        default:
            return "UNKNOWN";
    }
}

/**
 * @brief Converts a quote type enum to its canonical external string.
 */
inline std::string to_string(QuoteType type) {
    switch (type) {
        // Commodity
        case QuoteType::CommodityForward:
            return "CommodityForward";

        // Credit
        case QuoteType::BondYield:
            return "BondYield";
        case QuoteType::CreditSpread:
            return "CreditSpread";
        case QuoteType::HazardRate:
            return "HazardRate";
        case QuoteType::RecoveryRate:
            return "RecoveryRate";

        // Equity
        case QuoteType::BorrowRate:
            return "BorrowRate";
        case QuoteType::DividendYield:
            return "DividendYield";
        case QuoteType::EquitySpot:
            return "EquitySpot";

        // FX
        case QuoteType::FXForward:
            return "FXForward";
        case QuoteType::FXForwardPoint:
            return "FXForwardPoint";
        case QuoteType::FXSpot:
            return "FXSpot";

        // Generic
        case QuoteType::Future:
            return "Future";
        case QuoteType::Price:
            return "Price";
        case QuoteType::Volatility:
            return "Volatility";

        // Rates
        case QuoteType::BasisSwap:
            return "BasisSwap";
        case QuoteType::Deposit:
            return "Deposit";
        case QuoteType::FRA:
            return "FRA";
        case QuoteType::InterestRateFuture:
            return "InterestRateFuture";
        case QuoteType::IRS:
            return "IRS";
        case QuoteType::OIS:
            return "OIS";
        case QuoteType::Swap:
            return "Swap";

        default:
            return "UNKNOWN";
    }
}

} // namespace qrp::domain
