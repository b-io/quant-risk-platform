#pragma once

// Defines market snapshot DTOs, JSON conversion, validation diagnostics, and readiness gates.

#include <qrp/domain/types.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <iterator>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace qrp::domain {

/**
 * @brief Parses a curve purpose string from market-data JSON.
 */
inline CurvePurpose parse_curve_purpose(const std::string& s) {
    // Commodity
    if (s == "CommodityForward")
        return CurvePurpose::CommodityForward;
    if (s == "CommodityVolatility")
        return CurvePurpose::CommodityVolatility;

    // Credit
    if (s == "Credit")
        return CurvePurpose::Credit;
    if (s == "CreditSpread")
        return CurvePurpose::CreditSpread;
    if (s == "Hazard")
        return CurvePurpose::Hazard;
    if (s == "Recovery")
        return CurvePurpose::Recovery;

    // Equity
    if (s == "EquityBorrow")
        return CurvePurpose::EquityBorrow;
    if (s == "EquityDividend")
        return CurvePurpose::EquityDividend;
    if (s == "EquityVolatility")
        return CurvePurpose::EquityVolatility;

    // FX
    if (s == "FXForward")
        return CurvePurpose::FXForward;
    if (s == "FXVolatility")
        return CurvePurpose::FXVolatility;

    // Generic
    if (s == "Volatility")
        return CurvePurpose::Volatility;

    // Rates
    if (s == "Discount")
        return CurvePurpose::Discount;
    if (s == "Forward")
        return CurvePurpose::Forward;
    if (s == "Forward3M")
        return CurvePurpose::Forward3M;
    if (s == "Forward6M")
        return CurvePurpose::Forward6M;
    if (s == "Inflation")
        return CurvePurpose::Inflation;
    if (s == "OISDiscount")
        return CurvePurpose::OISDiscount;

    return CurvePurpose::UNKNOWN;
}

/**
 * @brief Parses a quote instrument type string from market-data JSON.
 */
inline QuoteInstrumentType parse_quote_instrument_type(const std::string& s) {
    // Commodity
    if (s == "CommodityForward")
        return QuoteInstrumentType::CommodityForward;
    if (s == "CommodityFuture")
        return QuoteInstrumentType::CommodityFuture;
    if (s == "CommoditySpot")
        return QuoteInstrumentType::CommoditySpot;
    if (s == "CommodityVol")
        return QuoteInstrumentType::CommodityVol;
    if (s == "ConvenienceYield")
        return QuoteInstrumentType::ConvenienceYield;

    // Credit
    if (s == "Bond")
        return QuoteInstrumentType::Bond;
    if (s == "BondPrice")
        return QuoteInstrumentType::BondPrice;
    if (s == "BondSpread")
        return QuoteInstrumentType::BondSpread;
    if (s == "CDS")
        return QuoteInstrumentType::CDS;
    if (s == "CreditIndex")
        return QuoteInstrumentType::CreditIndex;
    if (s == "CreditSpread")
        return QuoteInstrumentType::CreditSpread;
    if (s == "HazardRate")
        return QuoteInstrumentType::HazardRate;
    if (s == "RecoveryRate")
        return QuoteInstrumentType::RecoveryRate;

    // Equity
    if (s == "BorrowRate")
        return QuoteInstrumentType::BorrowRate;
    if (s == "DividendYield")
        return QuoteInstrumentType::DividendYield;
    if (s == "EquitySpot")
        return QuoteInstrumentType::EquitySpot;
    if (s == "EquityVol")
        return QuoteInstrumentType::EquityVol;

    // FX
    if (s == "FXForward")
        return QuoteInstrumentType::FXForward;
    if (s == "FXForwardPoint")
        return QuoteInstrumentType::FXForwardPoint;
    if (s == "FXSpot")
        return QuoteInstrumentType::FXSpot;
    if (s == "FXVol")
        return QuoteInstrumentType::FXVol;

    // Generic
    if (s == "Future")
        return QuoteInstrumentType::Future;

    // Rates
    if (s == "CapFloorVol")
        return QuoteInstrumentType::CapFloorVol;
    if (s == "Deposit")
        return QuoteInstrumentType::Deposit;
    if (s == "FRA")
        return QuoteInstrumentType::FRA;
    if (s == "InterestRateFuture")
        return QuoteInstrumentType::InterestRateFuture;
    if (s == "IRS")
        return QuoteInstrumentType::IRS;
    if (s == "OIS")
        return QuoteInstrumentType::OIS;
    if (s == "SwaptionVol")
        return QuoteInstrumentType::SwaptionVol;

    return QuoteInstrumentType::UNKNOWN;
}

/**
 * @brief Parses a quote type string from market-data JSON.
 */
inline QuoteType parse_quote_type(const std::string& s) {
    // Commodity
    if (s == "CommodityForward")
        return QuoteType::CommodityForward;

    // Credit
    if (s == "BondYield")
        return QuoteType::BondYield;
    if (s == "CreditSpread")
        return QuoteType::CreditSpread;
    if (s == "HazardRate")
        return QuoteType::HazardRate;
    if (s == "RecoveryRate")
        return QuoteType::RecoveryRate;

    // Equity
    if (s == "BorrowRate")
        return QuoteType::BorrowRate;
    if (s == "DividendYield")
        return QuoteType::DividendYield;
    if (s == "EquitySpot")
        return QuoteType::EquitySpot;

    // FX
    if (s == "FXForward")
        return QuoteType::FXForward;
    if (s == "FXForwardPoint")
        return QuoteType::FXForwardPoint;
    if (s == "FXSpot")
        return QuoteType::FXSpot;

    // Generic
    if (s == "Future")
        return QuoteType::Future;
    if (s == "Price")
        return QuoteType::Price;
    if (s == "Volatility")
        return QuoteType::Volatility;

    // Rates
    if (s == "BasisSwap")
        return QuoteType::BasisSwap;
    if (s == "Deposit")
        return QuoteType::Deposit;
    if (s == "FRA")
        return QuoteType::FRA;
    if (s == "InterestRateFuture")
        return QuoteType::InterestRateFuture;
    if (s == "IRS")
        return QuoteType::IRS;
    if (s == "OIS")
        return QuoteType::OIS;
    if (s == "Swap")
        return QuoteType::Swap;

    return QuoteType::UNKNOWN;
}

/**
 * @brief Deserializes a currency from its canonical string.
 */
inline void from_json(const nlohmann::json& j, Currency& c) {
    c = from_string(j.get<std::string>());
}

/**
 * @brief Deserializes a curve purpose from its canonical string.
 */
inline void from_json(const nlohmann::json& j, CurvePurpose& p) {
    p = parse_curve_purpose(j.get<std::string>());
}

/**
 * @brief Deserializes a quote instrument type from its canonical string.
 */
inline void from_json(const nlohmann::json& j, QuoteInstrumentType& q) {
    q = parse_quote_instrument_type(j.get<std::string>());
}

/**
 * @brief Deserializes a quote type from its canonical string.
 */
inline void from_json(const nlohmann::json& j, QuoteType& q) {
    q = parse_quote_type(j.get<std::string>());
}

/**
 * @brief Deserializes a day-count convention from its canonical string.
 */
inline void from_json(const nlohmann::json& j, DayCount& d) {
    std::string s = j.get<std::string>();
    if (s == "ACT360")
        d = DayCount::ACT360;
    else if (s == "ACT365")
        d = DayCount::ACT365;
    else if (s == "ACT365F")
        d = DayCount::ACT365F;
    else if (s == "ACTACT")
        d = DayCount::ACTACT;
    else if (s == "ACTACT_AFB")
        d = DayCount::ACTACT_AFB;
    else if (s == "ACTACT_EURO")
        d = DayCount::ACTACT_EURO;
    else if (s == "ACTACT_ISDA")
        d = DayCount::ACTACT_ISDA;
    else if (s == "ACTACT_ISMA")
        d = DayCount::ACTACT_ISMA;
    else if (s == "Thirty360")
        d = DayCount::Thirty360;
    else
        d = DayCount::UNKNOWN;
}

/**
 * @brief Deserializes a business calendar from its canonical string.
 */
inline void from_json(const nlohmann::json& j, BusinessCalendar& b) {
    std::string s = j.get<std::string>();
    if (s == "CHF" || s == "CHE")
        b = BusinessCalendar::CHF;
    else if (s == "JP" || s == "JPN")
        b = BusinessCalendar::JP;
    else if (s == "Target")
        b = BusinessCalendar::Target;
    else if (s == "UK" || s == "GBR")
        b = BusinessCalendar::UK;
    else if (s == "US" || s == "USA")
        b = BusinessCalendar::US;
    else if (s == "WeekendsOnly")
        b = BusinessCalendar::WeekendsOnly;
    else
        b = BusinessCalendar::UNKNOWN;
}

/**
 * @brief Deserializes a business-day convention from its canonical string.
 */
inline void from_json(const nlohmann::json& j, BusinessDayConvention& b) {
    std::string s = j.get<std::string>();
    if (s == "Following")
        b = BusinessDayConvention::Following;
    else if (s == "HalfMonthModifiedFollowing")
        b = BusinessDayConvention::HalfMonthModifiedFollowing;
    else if (s == "ModifiedFollowing")
        b = BusinessDayConvention::ModifiedFollowing;
    else if (s == "ModifiedPreceding")
        b = BusinessDayConvention::ModifiedPreceding;
    else if (s == "Nearest")
        b = BusinessDayConvention::Nearest;
    else if (s == "Preceding")
        b = BusinessDayConvention::Preceding;
    else if (s == "Unadjusted")
        b = BusinessDayConvention::Unadjusted;
    else
        b = BusinessDayConvention::UNKNOWN;
}

/**
 * @brief Deserializes a schedule frequency from its canonical string.
 */
inline void from_json(const nlohmann::json& j, Frequency& f) {
    std::string s = j.get<std::string>();
    if (s == "Annual")
        f = Frequency::Annual;
    else if (s == "Bimonthly")
        f = Frequency::Bimonthly;
    else if (s == "Biweekly")
        f = Frequency::Biweekly;
    else if (s == "Daily")
        f = Frequency::Daily;
    else if (s == "EveryFourthMonth")
        f = Frequency::EveryFourthMonth;
    else if (s == "EveryFourthWeek")
        f = Frequency::EveryFourthWeek;
    else if (s == "Monthly")
        f = Frequency::Monthly;
    else if (s == "Once")
        f = Frequency::Once;
    else if (s == "OtherFrequency")
        f = Frequency::OtherFrequency;
    else if (s == "Quarterly")
        f = Frequency::Quarterly;
    else if (s == "Semiannual")
        f = Frequency::Semiannual;
    else if (s == "Weekly")
        f = Frequency::Weekly;
    else
        f = Frequency::UNKNOWN;
}

/**
 * @brief Deserializes a date-generation rule from its canonical string.
 */
inline void from_json(const nlohmann::json& j, DateGeneration& d) {
    std::string s = j.get<std::string>();
    if (s == "Backward")
        d = DateGeneration::Backward;
    else if (s == "CDS")
        d = DateGeneration::CDS;
    else if (s == "CDS2015")
        d = DateGeneration::CDS2015;
    else if (s == "Forward")
        d = DateGeneration::Forward;
    else if (s == "OldCDS")
        d = DateGeneration::OldCDS;
    else if (s == "ThirdWednesday")
        d = DateGeneration::ThirdWednesday;
    else if (s == "Twentieth")
        d = DateGeneration::Twentieth;
    else if (s == "TwentiethIMM")
        d = DateGeneration::TwentiethIMM;
    else if (s == "Zero")
        d = DateGeneration::Zero;
    else
        d = DateGeneration::UNKNOWN;
}

/**
 * @brief Deserializes an interpolation policy from its canonical string.
 */
inline void from_json(const nlohmann::json& j, InterpolationType& i) {
    std::string s = j.get<std::string>();
    if (s == "CubicSpline")
        i = InterpolationType::CubicSpline;
    else if (s == "Linear")
        i = InterpolationType::Linear;
    else if (s == "LogLinear")
        i = InterpolationType::LogLinear;
    else
        i = InterpolationType::UNKNOWN;
}

/**
 * @brief Deserializes a raw market quote and its optional provenance fields.
 */
inline void from_json(const nlohmann::json& j, MarketQuote& q) {
    j.at("id").get_to(q.id);
    j.at("instrument_type").get_to(q.instrument_type);
    j.at("currency").get_to(q.currency);
    j.at("tenor").get_to(q.tenor);
    j.at("value").get_to(q.value);

    if (j.contains("risk_factor_id"))
        j.at("risk_factor_id").get_to(q.risk_factor_id);
    if (j.contains("quote_type"))
        j.at("quote_type").get_to(q.quote_type);
    if (j.contains("underlier"))
        j.at("underlier").get_to(q.underlier);
    if (j.contains("expiry"))
        j.at("expiry").get_to(q.expiry);
    if (j.contains("strike"))
        j.at("strike").get_to(q.strike);
    if (j.contains("instrument_family"))
        j.at("instrument_family").get_to(q.instrument_family);
    if (j.contains("index_family"))
        j.at("index_family").get_to(q.index_family);
    if (j.contains("day_count"))
        j.at("day_count").get_to(q.day_count);
    if (j.contains("calendar"))
        j.at("calendar").get_to(q.calendar);
    if (j.contains("bdc"))
        j.at("bdc").get_to(q.bdc);
    if (j.contains("settlement_days"))
        j.at("settlement_days").get_to(q.settlement_days);
    if (j.contains("market_ts"))
        j.at("market_ts").get_to(q.market_ts);
    if (j.contains("recorded_ts"))
        j.at("recorded_ts").get_to(q.recorded_ts);
    if (j.contains("source_name"))
        j.at("source_name").get_to(q.source_name);
    if (j.contains("source_ts"))
        j.at("source_ts").get_to(q.source_ts);
    if (j.contains("stale_after_days"))
        j.at("stale_after_days").get_to(q.stale_after_days);
}

/**
 * @brief Deserializes a curve identifier from JSON.
 */
inline void from_json(const nlohmann::json& j, CurveId& id) {
    j.at("currency").get_to(id.currency);
    j.at("family").get_to(id.family);
}

/**
 * @brief Deserializes a curve construction specification from JSON.
 */
inline void from_json(const nlohmann::json& j, CurveSpec& s) {
    j.at("id").get_to(s.id);
    if (j.contains("purpose"))
        j.at("purpose").get_to(s.purpose);
    j.at("quote_ids").get_to(s.quote_ids);
    if (j.contains("day_count"))
        j.at("day_count").get_to(s.day_count);
    if (j.contains("calendar"))
        j.at("calendar").get_to(s.calendar);
    if (j.contains("interpolation"))
        j.at("interpolation").get_to(s.interpolation);
    if (j.contains("construction_family"))
        j.at("construction_family").get_to(s.construction_family);
    if (j.contains("collateral_curve_id"))
        j.at("collateral_curve_id").get_to(s.collateral_curve_id);
    if (j.contains("discount_curve_id"))
        j.at("discount_curve_id").get_to(s.discount_curve_id);
    if (j.contains("metadata_json"))
        j.at("metadata_json").get_to(s.metadata_json);
}

/**
 * @brief Diagnostic emitted while validating market snapshots.
 */
struct MarketDataDiagnostic {
    std::string severity; // Diagnostic severity.
    std::string code;     // Stable diagnostic code.
    std::string message;  // Human-readable diagnostic message.
    std::string quote_id; // Related quote id, when applicable.
    std::string curve_id; // Related curve id, when applicable.
};

/**
 * @brief Deserializes a market-data diagnostic from JSON.
 */
inline void from_json(const nlohmann::json& j, MarketDataDiagnostic& d) {
    if (j.contains("severity"))
        j.at("severity").get_to(d.severity);
    if (j.contains("code"))
        j.at("code").get_to(d.code);
    if (j.contains("message"))
        j.at("message").get_to(d.message);
    if (j.contains("quote_id"))
        j.at("quote_id").get_to(d.quote_id);
    if (j.contains("curve_id"))
        j.at("curve_id").get_to(d.curve_id);
}

/**
 * @brief Versioned market snapshot DTO used as raw input to market-state construction.
 */
struct MarketSnapshot {
    std::string valuation_date;                                   // Market state valuation date.
    std::string snapshot_id;                                      // Stable market snapshot id.
    int schema_version = 2;                                       // Market snapshot schema version.
    Currency base_currency = Currency::UNKNOWN;                   // Reporting/base currency.
    std::string source_name;                                      // Source system or fixture name.
    std::string recorded_ts;                                      // Timestamp when the snapshot was recorded.
    int default_stale_after_days = -1;                            // Default quote staleness threshold.
    std::vector<MarketQuote> quotes;                              // Raw market quotes included in the snapshot.
    std::vector<CurveSpec> curves;                                // Curve construction specifications.
    std::map<std::string, std::map<std::string, double>> fixings; // index_name -> { date -> value }
    std::vector<MarketDataDiagnostic> diagnostics;                // Validation diagnostics captured with the snapshot.
};

/**
 * @brief Deserializes a market snapshot and optional validation diagnostics from JSON.
 */
inline void from_json(const nlohmann::json& j, MarketSnapshot& m) {
    j.at("valuation_date").get_to(m.valuation_date);
    if (j.contains("snapshot_id"))
        j.at("snapshot_id").get_to(m.snapshot_id);
    if (j.contains("schema_version"))
        j.at("schema_version").get_to(m.schema_version);
    if (j.contains("base_currency"))
        j.at("base_currency").get_to(m.base_currency);
    if (j.contains("source_name"))
        j.at("source_name").get_to(m.source_name);
    if (j.contains("recorded_ts"))
        j.at("recorded_ts").get_to(m.recorded_ts);
    if (j.contains("default_stale_after_days"))
        j.at("default_stale_after_days").get_to(m.default_stale_after_days);
    j.at("quotes").get_to(m.quotes);
    j.at("curves").get_to(m.curves);
    if (j.contains("fixings")) {
        j.at("fixings").get_to(m.fixings);
    }
    if (j.contains("diagnostics")) {
        j.at("diagnostics").get_to(m.diagnostics);
    }
}

namespace market_data_detail {

/**
 * @brief Checks whether a character position contains an ASCII digit.
 */
inline bool is_digit_at(const std::string& value, std::size_t index) {
    return index < value.size() && std::isdigit(static_cast<unsigned char>(value[index])) != 0;
}

/**
 * @brief Parses the date component of an ISO timestamp into a calendar day.
 */
inline std::optional<std::chrono::sys_days> parse_iso_day(const std::string& value) {
    if (value.size() < 10 || value[4] != '-' || value[7] != '-' || !is_digit_at(value, 0) || !is_digit_at(value, 1) ||
        !is_digit_at(value, 2) || !is_digit_at(value, 3) || !is_digit_at(value, 5) || !is_digit_at(value, 6) ||
        !is_digit_at(value, 8) || !is_digit_at(value, 9)) {
        return std::nullopt;
    }

    const int year = std::stoi(value.substr(0, 4));
    const unsigned month = static_cast<unsigned>(std::stoi(value.substr(5, 2)));
    const unsigned day = static_cast<unsigned>(std::stoi(value.substr(8, 2)));
    const std::chrono::year_month_day ymd{std::chrono::year{year}, std::chrono::month{month}, std::chrono::day{day}};
    if (!ymd.ok()) {
        return std::nullopt;
    }
    return std::chrono::sys_days{ymd};
}

/**
 * @brief Orders diagnostics by severity for stable reporting.
 */
inline int diagnostic_severity_rank(const std::string& severity) {
    if (severity == "ERROR")
        return 0;
    if (severity == "WARNING")
        return 1;
    return 2;
}

/**
 * @brief Formats a curve id for diagnostics.
 */
inline std::string curve_id_to_string(const CurveId& id) {
    return to_string(id.currency) + ":" + id.family;
}

} // namespace market_data_detail

/**
 * @brief Validates quote identity, quote freshness, curve references, and diagnostic order.
 */
inline std::vector<MarketDataDiagnostic> validate_market_snapshot(const MarketSnapshot& snapshot) {
    std::vector<MarketDataDiagnostic> diagnostics;

    auto add = [&](std::string severity,
                   std::string code,
                   std::string message,
                   std::string quote_id = {},
                   std::string curve_id = {}) {
        diagnostics.push_back(MarketDataDiagnostic{std::move(severity),
                                                   std::move(code),
                                                   std::move(message),
                                                   std::move(quote_id),
                                                   std::move(curve_id)});
    };

    const auto valuation_day = market_data_detail::parse_iso_day(snapshot.valuation_date);
    if (!valuation_day) {
        add("ERROR", "INVALID_VALUATION_DATE", "Market snapshot valuation_date must be a valid YYYY-MM-DD date");
    }

    std::set<std::string> quote_ids;
    for (const auto& quote : snapshot.quotes) {
        if (quote.id.empty()) {
            add("ERROR", "MISSING_QUOTE_ID", "Market quote id must not be empty");
            continue;
        }

        if (!quote_ids.insert(quote.id).second) {
            add("ERROR", "DUPLICATE_QUOTE_ID", "Market snapshot contains duplicate quote id", quote.id);
        }

        if (quote.currency == Currency::UNKNOWN) {
            add("WARNING", "UNKNOWN_QUOTE_CURRENCY", "Market quote currency is UNKNOWN", quote.id);
        }
        if (quote.instrument_type == QuoteInstrumentType::UNKNOWN) {
            add("WARNING", "UNKNOWN_QUOTE_INSTRUMENT_TYPE", "Market quote instrument_type is UNKNOWN", quote.id);
        }
        if (quote.source_name.empty()) {
            add("WARNING", "MISSING_QUOTE_SOURCE", "Market quote source_name is missing", quote.id);
        }

        const std::string staleness_ts = !quote.source_ts.empty() ? quote.source_ts : quote.market_ts;
        if (staleness_ts.empty()) {
            add("WARNING", "MISSING_QUOTE_TIMESTAMP", "Market quote source_ts or market_ts is missing", quote.id);
        } else if (valuation_day) {
            const auto quote_day = market_data_detail::parse_iso_day(staleness_ts);
            if (!quote_day) {
                add("WARNING",
                    "INVALID_QUOTE_TIMESTAMP",
                    "Market quote source_ts/market_ts must begin with YYYY-MM-DD",
                    quote.id);
            } else {
                const int stale_after_days =
                    quote.stale_after_days >= 0 ? quote.stale_after_days : snapshot.default_stale_after_days;
                if (stale_after_days >= 0) {
                    const auto age = std::chrono::duration_cast<std::chrono::days>(*valuation_day - *quote_day).count();
                    if (age > stale_after_days) {
                        add("WARNING",
                            "STALE_QUOTE",
                            "Market quote is older than its stale_after_days threshold",
                            quote.id);
                    }
                }
            }
        }
    }

    for (const auto& curve : snapshot.curves) {
        const auto curve_id = market_data_detail::curve_id_to_string(curve.id);
        if (curve.id.currency == Currency::UNKNOWN || curve.id.family.empty()) {
            add("ERROR", "INVALID_CURVE_ID", "Curve id must include a currency and family", {}, curve_id);
        }
        if (curve.purpose == CurvePurpose::UNKNOWN) {
            add("WARNING", "UNKNOWN_CURVE_PURPOSE", "Curve purpose is UNKNOWN", {}, curve_id);
        }
        if (curve.quote_ids.empty()) {
            add("WARNING", "EMPTY_CURVE_QUOTE_SET", "Curve has no quote_ids", {}, curve_id);
        }

        std::set<std::string> curve_quote_ids;
        for (const auto& quote_id : curve.quote_ids) {
            if (!curve_quote_ids.insert(quote_id).second) {
                add("WARNING",
                    "DUPLICATE_CURVE_QUOTE_ID",
                    "Curve quote_ids contains a duplicate quote id",
                    quote_id,
                    curve_id);
            }
            if (!quote_ids.contains(quote_id)) {
                add("ERROR",
                    "MISSING_CURVE_QUOTE",
                    "Curve references a quote_id that is absent from the snapshot",
                    quote_id,
                    curve_id);
            }
        }
    }

    std::sort(diagnostics.begin(), diagnostics.end(), [](const auto& lhs, const auto& rhs) {
        const auto lhs_rank = market_data_detail::diagnostic_severity_rank(lhs.severity);
        const auto rhs_rank = market_data_detail::diagnostic_severity_rank(rhs.severity);
        if (lhs_rank != rhs_rank)
            return lhs_rank < rhs_rank;
        if (lhs.code != rhs.code)
            return lhs.code < rhs.code;
        if (lhs.curve_id != rhs.curve_id)
            return lhs.curve_id < rhs.curve_id;
        if (lhs.quote_id != rhs.quote_id)
            return lhs.quote_id < rhs.quote_id;
        return lhs.message < rhs.message;
    });

    return diagnostics;
}

/**
 * @brief Formats one market-data diagnostic for error messages and logs.
 */
inline std::string format_market_data_diagnostic(const MarketDataDiagnostic& diagnostic) {
    std::string formatted = diagnostic.severity + ":" + diagnostic.code;
    if (!diagnostic.quote_id.empty()) {
        formatted += " quote=" + diagnostic.quote_id;
    }
    if (!diagnostic.curve_id.empty()) {
        formatted += " curve=" + diagnostic.curve_id;
    }
    if (!diagnostic.message.empty()) {
        formatted += " - " + diagnostic.message;
    }
    return formatted;
}

/**
 * @brief Indicates whether a diagnostic must block valuation, risk, and reporting workflows.
 */
inline bool is_blocking_market_data_diagnostic(const MarketDataDiagnostic& diagnostic) {
    return diagnostic.severity == "ERROR" || diagnostic.code == "STALE_QUOTE";
}

/**
 * @brief Combines source-provided and platform-computed diagnostics without duplicate entries.
 */
inline std::vector<MarketDataDiagnostic> collect_market_snapshot_diagnostics(const MarketSnapshot& snapshot) {
    auto diagnostics = validate_market_snapshot(snapshot);
    auto exists = [&](const MarketDataDiagnostic& candidate) {
        return std::any_of(diagnostics.begin(), diagnostics.end(), [&](const auto& diagnostic) {
            return diagnostic.severity == candidate.severity && diagnostic.code == candidate.code &&
                   diagnostic.message == candidate.message && diagnostic.quote_id == candidate.quote_id &&
                   diagnostic.curve_id == candidate.curve_id;
        });
    };

    for (const auto& diagnostic : snapshot.diagnostics) {
        if (!exists(diagnostic)) {
            diagnostics.push_back(diagnostic);
        }
    }

    std::sort(diagnostics.begin(), diagnostics.end(), [](const auto& lhs, const auto& rhs) {
        const auto lhs_rank = market_data_detail::diagnostic_severity_rank(lhs.severity);
        const auto rhs_rank = market_data_detail::diagnostic_severity_rank(rhs.severity);
        if (lhs_rank != rhs_rank)
            return lhs_rank < rhs_rank;
        if (lhs.code != rhs.code)
            return lhs.code < rhs.code;
        if (lhs.curve_id != rhs.curve_id)
            return lhs.curve_id < rhs.curve_id;
        if (lhs.quote_id != rhs.quote_id)
            return lhs.quote_id < rhs.quote_id;
        return lhs.message < rhs.message;
    });

    return diagnostics;
}

/**
 * @brief Returns the diagnostics that block downstream workflows for a market snapshot.
 */
inline std::vector<MarketDataDiagnostic> blocking_market_snapshot_diagnostics(const MarketSnapshot& snapshot) {
    const auto diagnostics = collect_market_snapshot_diagnostics(snapshot);
    std::vector<MarketDataDiagnostic> blocking;
    std::copy_if(diagnostics.begin(),
                 diagnostics.end(),
                 std::back_inserter(blocking),
                 is_blocking_market_data_diagnostic);
    return blocking;
}

/**
 * @brief Throws when a market snapshot is not ready for valuation, risk, or reporting workflows.
 */
inline void throw_if_market_snapshot_not_ready(const MarketSnapshot& snapshot) {
    const auto blocking = blocking_market_snapshot_diagnostics(snapshot);
    if (blocking.empty()) {
        return;
    }

    std::string message = "Market snapshot is not ready for workflow execution";
    if (!snapshot.snapshot_id.empty()) {
        message += " (snapshot_id=" + snapshot.snapshot_id + ")";
    }
    message += ": ";
    for (std::size_t i = 0; i < blocking.size(); ++i) {
        if (i > 0) {
            message += "; ";
        }
        message += format_market_data_diagnostic(blocking[i]);
    }
    throw std::runtime_error(message);
}

} // namespace qrp::domain
