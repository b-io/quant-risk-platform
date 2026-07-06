#pragma once

// Defines per-product pricing adjustments and support metadata used by valuation services.

#include <qrp/domain/product.hpp>

#include <map>
#include <string>

namespace qrp::analytics {

/**
 * @brief Realized cashflow extraction status and diagnostics for one trade.
 */
struct CashflowExtractionResult {
    double realized_cash_pnl = 0.0;    // Cash PnL realized between two valuation dates.
    bool extraction_supported = false; // Whether this product has cashflow extraction logic.
    std::string model_name;            // Cashflow extraction model or approximation name.
    domain::SupportStatus support_status = domain::SupportStatus::Unsupported; // Extraction status.
    std::string status_message;                                                // Diagnostic message.
    std::map<std::string, std::string> tags;                                   // Product-specific metadata.
};

/**
 * @brief Product-specific adjustment applied around an instrument NPV.
 */
struct InstrumentPricingProfile {
    double additive_npv = 0.0; // Additive adjustment applied after instrument pricing.
    double multiplier = 1.0;   // Multiplicative adjustment applied to instrument NPV.
};

/**
 * @brief Declares whether a product can be priced and why.
 */
struct ProductSupportProfile {
    domain::AssetClass asset_class = domain::AssetClass::Unknown;      // Product asset class.
    std::string model_name;                                            // Pricing model or approximation name.
    domain::ProductType product_type = domain::ProductType::Unknown;   // Product taxonomy value.
    std::string reason;                                                // Human-readable support reason.
    domain::SupportStatus status = domain::SupportStatus::Unsupported; // Support status.
};

/**
 * @brief Normalized valuation output for one trade.
 */
struct ValuationResult {
    domain::AssetClass asset_class = domain::AssetClass::Unknown;         // Trade asset class.
    std::string currency;                                                 // Valuation currency.
    std::string model_name;                                               // Pricing model or approximation name.
    double npv = 0.0;                                                     // Net present value.
    domain::ProductType product_type = domain::ProductType::Unknown;      // Product taxonomy value.
    domain::SupportStatus support_status = domain::SupportStatus::Failed; // Pricing support status.
    std::string status_message;                                           // Diagnostic message.
    std::map<std::string, std::string> tags;                              // Product-specific metadata.
    std::string trade_id;                                                 // Valued trade id.
};

} // namespace qrp::analytics
