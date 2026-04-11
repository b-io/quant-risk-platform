#pragma once
#include <qrp/domain/types.hpp>
#include <string>
#include <vector>

namespace qrp::domain {

/**
 * @brief Categorizes risk factors for modeling and simulation.
 * 
 * Each type implies a specific mapping to market quotes and curve nodes.
 */
enum class FactorType {
    FXSpot,           // Relative moves on spot rates
    RateZero,         // Absolute shifts on zero rates
    RateForward,      // Absolute shifts on forward rates
    CreditSpread,     // Absolute shifts on credit spreads
    HazardRate,       // Absolute shifts on default probabilities
    Volatility,       // Absolute or relative moves on vol surfaces
    EquitySpot,       // Relative/Log-returns on equity prices
    CommodityForward, // Relative moves on forward curves
    BasisSpread,      // Absolute shifts on basis curves
    Custom            // User-defined factor
};

/**
 * @brief Specifies how a shock is applied to the underlying market factor.
 */
enum class ShockMeasure {
    Absolute,         // f_new = f_old + shock
    Relative,         // f_new = f_old * (1 + shock)
    LogReturn,        // f_new = f_old * exp(shock)
    BasisPoints,      // f_new = f_old + shock / 10000
    VolPoints         // f_new = f_old + shock / 100 (if old is in percent)
};

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
