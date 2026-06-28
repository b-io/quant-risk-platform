#pragma once

// Defines per-product pricing adjustments and support metadata used by valuation services.

#include <qrp/domain/product.hpp>
#include <map>
#include <string>

namespace qrp::analytics {

struct CashflowExtractionResult {
    double realized_cash_pnl = 0.0;
    bool extraction_supported = false;
    std::string model_name;
    domain::SupportStatus support_status = domain::SupportStatus::Unsupported;
    std::string status_message;
    std::map<std::string, std::string> tags;
};

struct InstrumentPricingProfile {
    double additive_npv = 0.0;
    double multiplier = 1.0;
};

struct ProductSupportProfile {
    domain::AssetClass asset_class = domain::AssetClass::Unknown;
    std::string model_name;
    domain::ProductType product_type = domain::ProductType::Unknown;
    std::string reason;
    domain::SupportStatus status = domain::SupportStatus::Unsupported;
};

struct ValuationResult {
    domain::AssetClass asset_class = domain::AssetClass::Unknown;
    std::string currency;
    std::string model_name;
    double npv = 0.0;
    domain::ProductType product_type = domain::ProductType::Unknown;
    domain::SupportStatus support_status = domain::SupportStatus::Failed;
    std::string status_message;
    std::map<std::string, std::string> tags;
    std::string trade_id;
};

} // namespace qrp::analytics
