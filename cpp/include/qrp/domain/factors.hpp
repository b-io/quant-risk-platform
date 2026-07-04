#pragma once

// Defines canonical risk-factor taxonomy, shock measures, histories, and bindings.

#include <qrp/domain/types.hpp>

#include <initializer_list>
#include <stdexcept>
#include <string>
#include <vector>

namespace qrp::domain {

/**
 * @brief Categorizes risk factors for modeling and simulation.
 *
 * Each type implies a specific mapping to market quotes and curve nodes.
 */
enum class FactorType {
    // Commodity
    CommodityForward, // Relative moves on forward curves

    // Credit
    CreditSpread,     // Absolute shifts on credit spreads
    HazardRate,       // Absolute shifts on default probabilities

    // Equity
    EquitySpot,       // Relative/Log-returns on equity prices

    // FX
    FXSpot,           // Relative moves on spot rates

    // Generic
    Custom,           // User-defined factor
    Volatility,       // Absolute or relative moves on vol surfaces

    // Rates
    BasisSpread,      // Absolute shifts on basis curves
    RateForward,      // Absolute shifts on forward rates
    RateZero          // Absolute shifts on zero rates
};

/**
 * @brief Specifies how a shock is applied to the underlying market factor.
 */
enum class ShockMeasure {
    Absolute,         // f_new = f_old + shock
    BasisPoints,      // f_new = f_old + shock / 10000
    LogReturn,        // f_new = f_old * exp(shock)
    Relative,         // f_new = f_old * (1 + shock)
    VolPoints         // f_new = f_old + shock / 100 (if old is in percent)
};

/**
 * @brief Parses a risk factor type from its canonical string.
 */
inline FactorType parse_factor_type(const std::string& value) {
    // Commodity
    if (value == "CommodityForward") return FactorType::CommodityForward;

    // Credit
    if (value == "CreditSpread") return FactorType::CreditSpread;
    if (value == "HazardRate") return FactorType::HazardRate;

    // Equity
    if (value == "EquitySpot") return FactorType::EquitySpot;

    // FX
    if (value == "FXSpot") return FactorType::FXSpot;

    // Generic
    if (value == "Custom") return FactorType::Custom;
    if (value == "Volatility") return FactorType::Volatility;

    // Rates
    if (value == "BasisSpread") return FactorType::BasisSpread;
    if (value == "RateForward") return FactorType::RateForward;
    if (value == "RateZero") return FactorType::RateZero;

    throw std::invalid_argument("Unknown factor_type: " + value);
}

/**
 * @brief Parses a shock measure from its canonical string.
 */
inline ShockMeasure parse_shock_measure(const std::string& value) {
    if (value == "Absolute") return ShockMeasure::Absolute;
    if (value == "BasisPoints") return ShockMeasure::BasisPoints;
    if (value == "LogReturn") return ShockMeasure::LogReturn;
    if (value == "Relative") return ShockMeasure::Relative;
    if (value == "VolPoints") return ShockMeasure::VolPoints;
    throw std::invalid_argument("Unknown shock_measure: " + value);
}

/**
 * @brief Converts a factor type to its canonical string.
 */
inline std::string to_string(FactorType factor_type) {
    switch (factor_type) {
        // Commodity
        case FactorType::CommodityForward: return "CommodityForward";

        // Credit
        case FactorType::CreditSpread: return "CreditSpread";
        case FactorType::HazardRate: return "HazardRate";

        // Equity
        case FactorType::EquitySpot: return "EquitySpot";

        // FX
        case FactorType::FXSpot: return "FXSpot";

        // Generic
        case FactorType::Custom: return "Custom";
        case FactorType::Volatility: return "Volatility";

        // Rates
        case FactorType::BasisSpread: return "BasisSpread";
        case FactorType::RateForward: return "RateForward";
        case FactorType::RateZero: return "RateZero";
    }
    return "Custom";
}

/**
 * @brief Converts a shock measure to its canonical string.
 */
inline std::string to_string(ShockMeasure shock_measure) {
    switch (shock_measure) {
        case ShockMeasure::Absolute: return "Absolute";
        case ShockMeasure::BasisPoints: return "BasisPoints";
        case ShockMeasure::LogReturn: return "LogReturn";
        case ShockMeasure::Relative: return "Relative";
        case ShockMeasure::VolPoints: return "VolPoints";
    }
    return "Absolute";
}

namespace factor_id_detail {

/**
 * @brief Splits a canonical factor id into colon-delimited fields.
 */
inline std::vector<std::string> split(const std::string& value) {
    std::vector<std::string> parts;
    std::size_t begin = 0;
    while (begin <= value.size()) {
        const auto end = value.find(':', begin);
        parts.push_back(value.substr(begin, end == std::string::npos ? std::string::npos : end - begin));
        if (end == std::string::npos) break;
        begin = end + 1;
    }
    return parts;
}

/**
 * @brief Validates and returns one token in a canonical factor id.
 */
inline std::string token(const std::string& value, const std::string& field_name) {
    if (value.empty()) {
        throw std::invalid_argument("Risk factor " + field_name + " must not be empty");
    }
    if (value.find(':') != std::string::npos) {
        throw std::invalid_argument("Risk factor " + field_name + " must not contain ':'");
    }
    return value;
}

/**
 * @brief Converts a currency to a factor-id token and rejects UNKNOWN.
 */
inline std::string currency_token(Currency currency) {
    if (currency == Currency::UNKNOWN) {
        throw std::invalid_argument("Risk factor currency must not be UNKNOWN");
    }
    return to_string(currency);
}

/**
 * @brief Joins factor-id fields with colon separators.
 */
inline std::string join(std::initializer_list<std::string> parts) {
    std::string result;
    for (const auto& part : parts) {
        if (!result.empty()) result += ":";
        result += part;
    }
    return result;
}

} // namespace factor_id_detail

/**
 * @brief Checks whether a factor id follows the platform canonical taxonomy.
 */
inline bool is_canonical_factor_id(const std::string& factor_id) {
    const auto parts = factor_id_detail::split(factor_id);
    if (parts.size() < 2 || parts[0] != "RF") return false;
    for (const auto& part : parts) {
        if (part.empty()) return false;
    }

    const auto& family = parts[1];
    if (family == "COM") return parts.size() == 5 && parts[3] == "FWD";
    if (family == "COMVOL") return parts.size() == 5;
    if (family == "CREDIT") return parts.size() == 5 && parts[3] == "SPREAD";
    if (family == "CREDITVOL") return parts.size() == 5;
    if (family == "EQ") return parts.size() == 4 && parts[3] == "SPOT";
    if (family == "EQVOL") return parts.size() == 5;
    if (family == "FX") return parts.size() == 4 && parts[3] == "SPOT";
    if (family == "FXVOL") return parts.size() == 5;
    if (family == "RATES") return parts.size() == 5;
    if (family == "RATESVOL") return parts.size() == 6;
    return false;
}

/**
 * @brief Builds a canonical commodity forward factor id.
 */
inline std::string make_commodity_forward_factor_id(const std::string& underlier, const std::string& tenor) {
    return factor_id_detail::join({
        "RF", "COM",
        factor_id_detail::token(underlier, "commodity underlier"),
        "FWD",
        factor_id_detail::token(tenor, "tenor")
    });
}

/**
 * @brief Builds a canonical commodity volatility factor id.
 */
inline std::string make_commodity_vol_factor_id(
    const std::string& underlier,
    const std::string& expiry,
    const std::string& strike) {
    return factor_id_detail::join({
        "RF", "COMVOL",
        factor_id_detail::token(underlier, "commodity underlier"),
        factor_id_detail::token(expiry, "expiry"),
        factor_id_detail::token(strike, "strike")
    });
}

/**
 * @brief Builds a canonical credit spread factor id.
 */
inline std::string make_credit_spread_factor_id(const std::string& issuer_or_index, const std::string& tenor) {
    return factor_id_detail::join({
        "RF", "CREDIT",
        factor_id_detail::token(issuer_or_index, "credit issuer or index"),
        "SPREAD",
        factor_id_detail::token(tenor, "tenor")
    });
}

/**
 * @brief Builds a canonical credit volatility factor id.
 */
inline std::string make_credit_vol_factor_id(
    const std::string& issuer_or_index,
    const std::string& expiry,
    const std::string& strike) {
    return factor_id_detail::join({
        "RF", "CREDITVOL",
        factor_id_detail::token(issuer_or_index, "credit issuer or index"),
        factor_id_detail::token(expiry, "expiry"),
        factor_id_detail::token(strike, "strike")
    });
}

/**
 * @brief Builds a canonical equity spot factor id.
 */
inline std::string make_equity_spot_factor_id(const std::string& underlier) {
    return factor_id_detail::join({
        "RF", "EQ",
        factor_id_detail::token(underlier, "equity underlier"),
        "SPOT"
    });
}

/**
 * @brief Builds a canonical equity volatility factor id.
 */
inline std::string make_equity_vol_factor_id(
    const std::string& underlier,
    const std::string& expiry,
    const std::string& strike) {
    return factor_id_detail::join({
        "RF", "EQVOL",
        factor_id_detail::token(underlier, "equity underlier"),
        factor_id_detail::token(expiry, "expiry"),
        factor_id_detail::token(strike, "strike")
    });
}

/**
 * @brief Builds a canonical FX spot factor id.
 */
inline std::string make_fx_spot_factor_id(const std::string& pair) {
    return factor_id_detail::join({
        "RF", "FX",
        factor_id_detail::token(pair, "FX pair"),
        "SPOT"
    });
}

/**
 * @brief Builds a canonical FX volatility factor id.
 */
inline std::string make_fx_vol_factor_id(
    const std::string& pair,
    const std::string& expiry,
    const std::string& delta_or_strike) {
    return factor_id_detail::join({
        "RF", "FXVOL",
        factor_id_detail::token(pair, "FX pair"),
        factor_id_detail::token(expiry, "expiry"),
        factor_id_detail::token(delta_or_strike, "delta or strike")
    });
}

/**
 * @brief Builds a canonical rates curve factor id.
 */
inline std::string make_rates_factor_id(Currency currency, const std::string& curve, const std::string& tenor) {
    return factor_id_detail::join({
        "RF", "RATES",
        factor_id_detail::currency_token(currency),
        factor_id_detail::token(curve, "curve"),
        factor_id_detail::token(tenor, "tenor")
    });
}

/**
 * @brief Builds a canonical rates volatility factor id.
 */
inline std::string make_rates_vol_factor_id(
    Currency currency,
    const std::string& surface,
    const std::string& expiry,
    const std::string& tenor_or_strike) {
    return factor_id_detail::join({
        "RF", "RATESVOL",
        factor_id_detail::currency_token(currency),
        factor_id_detail::token(surface, "surface"),
        factor_id_detail::token(expiry, "expiry"),
        factor_id_detail::token(tenor_or_strike, "tenor or strike")
    });
}

/**
 * @brief Formal definition of a risk factor for Monte Carlo or Stress Testing.
 *
 * A factor is the atomic unit of risk. It may map to one or more market quotes.
 */
struct FactorDefinition {
    std::string factor_id;
    FactorType factor_type = FactorType::Custom;
    ShockMeasure shock_measure = ShockMeasure::Absolute;
    Currency currency = Currency::UNKNOWN;
    std::string curve_id;     // Optional: The curve this factor impacts
    std::string tenor;        // Optional: The tenor node on a curve/surface
    std::vector<std::string> quote_ids; // The specific raw quotes linked to this factor
    std::string description;
};

/**
 * @brief Represents a historical observation of a factor move.
 *
 * Used for covariance estimation and historical simulation.
 */
struct FactorObservation {
    std::string factor_id;
    std::string market_date;
    double level = 0.0;       // Raw value at that date
    double move = 0.0;        // The move (shock) according to ShockMeasure
    std::string move_unit;    // e.g., "absolute", "log_return", "bp"
};

/**
 * @brief Describes how a factor affects a specific market quote.
 */
struct FactorBinding {
    std::string factor_id;
    std::string quote_id;
    ShockMeasure shock_measure = ShockMeasure::Absolute;
    double weight = 1.0;
    std::string transform;      // optional: identity, log_return, bp, vol_points, custom
    std::string selector_json;  // optional metadata for curve/surface selection rules
};

} // namespace qrp::domain
