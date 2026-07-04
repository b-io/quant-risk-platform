#pragma once

// Defines bitemporal market quote events used for as-of market reconstruction.

#include <qrp/domain/types.hpp>

#include <string>

namespace qrp::domain {

/**
 * @brief Represents a bitemporal market quote event.
 *
 * Supports bitemporal (dual-timestamp) storage to enable exact market state replay
 * and handling of historical corrections.
 */
struct MarketQuoteEvent {
    std::string event_id;
    std::string quote_id;
    std::string factor_id;      // Optional: Links the event to a risk factor

    // Bitemporal timestamps
    std::string market_ts;      // When the price occurred in the market (t_v)
    std::string recorded_ts;    // When the platform learned about it (t_k)

    // Provenance
    std::string source_name;
    std::string source_ts;

    // Value and Metadata
    double value = 0.0;
    Currency currency = Currency::UNKNOWN;
    std::string tenor;
    std::string instrument_type; // Deposit, OIS, IRS, etc.

    std::string overlay_set_id;  // Links the event to an override/correction set
    std::string metadata_json;   // Structured metadata for debugging/audit
};

} // namespace qrp::domain
