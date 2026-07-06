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
    std::string event_id;  // Stable event identifier.
    std::string quote_id;  // Market quote identifier.
    std::string factor_id; // Optional risk factor linked to the event.

    // Bitemporal timestamps
    std::string market_ts;   // Timestamp when the price occurred in the market.
    std::string recorded_ts; // Timestamp when the platform learned about the price.

    // Provenance
    std::string source_name; // Source system or vendor name.
    std::string source_ts;   // Source-provided publication timestamp.

    // Value and Metadata
    double value = 0.0;                    // Quoted market value.
    Currency currency = Currency::UNKNOWN; // Quote currency.
    std::string tenor;                     // Quote tenor when applicable.
    std::string instrument_type;           // Instrument family such as Deposit, OIS, or IRS.

    std::string overlay_set_id; // Override or correction set id.
    std::string metadata_json;  // Structured metadata for debugging and audit.
};

} // namespace qrp::domain
