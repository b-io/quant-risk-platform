#pragma once

#include <qrp/domain/product.hpp>
#include <map>
#include <string>

namespace qrp::analytics {

struct CashflowExtractionResult {
    double realized_cash_pnl = 0.0;
    domain::SupportStatus support_status = domain::SupportStatus::Supported;
    std::string model_name;
    std::string status_message;
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
