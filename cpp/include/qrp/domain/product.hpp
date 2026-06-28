#pragma once

// Defines product taxonomy, asset-class mapping, support statuses, and canonical string helpers.

#include <stdexcept>
#include <string>
#include <vector>

namespace qrp::domain {

/**
 * @brief Business asset-class taxonomy used for grouping trades and market data.
 */
enum class AssetClass {
    Commodity,
    Credit,
    Equity,
    FX,
    Inflation,
    Rates,
    Unknown
};

/**
 * @brief Product taxonomy used by support checks and pricing registry dispatch.
 */
enum class ProductType {
    CallableBond,
    CapFloor,
    Cds,
    CdsOption,
    CommodityFuture,
    CommodityFutureOption,
    CommoditySwing,
    CreditBond,
    CrossCurrencySwap,
    Deposit,
    EquityOption,
    EquitySpot,
    FixedRateBond,
    Fra,
    Future,
    FxForward,
    FxOption,
    OisSwap,
    Swaption,
    VanillaSwap,
    Unknown
};

/**
 * @brief Product support status returned by pricing and diagnostics workflows.
 */
enum class SupportStatus {
    Failed,
    PartiallySupported,
    Supported,
    Unsupported,
};

/**
 * @brief Parses an asset-class string from JSON or storage fields.
 */
inline AssetClass parse_asset_class(const std::string& value) {
    if (value == "commodity" || value == "commodities" || value == "Commodity") return AssetClass::Commodity;
    if (value == "credit" || value == "Credit") return AssetClass::Credit;
    if (value == "equity" || value == "Equity") return AssetClass::Equity;
    if (value == "fx" || value == "FX") return AssetClass::FX;
    if (value == "inflation" || value == "Inflation") return AssetClass::Inflation;
    if (value == "rates" || value == "Rates" || value == "IR") return AssetClass::Rates;
    return AssetClass::Unknown;
}

/**
 * @brief Converts an asset class to its canonical lowercase string.
 */
inline std::string to_string(AssetClass asset_class) {
    switch (asset_class) {
        case AssetClass::Commodity: return "commodity";
        case AssetClass::Credit: return "credit";
        case AssetClass::Equity: return "equity";
        case AssetClass::FX: return "fx";
        case AssetClass::Inflation: return "inflation";
        case AssetClass::Rates: return "rates";
        case AssetClass::Unknown: return "unknown";
    }
    return "unknown";
}

/**
 * @brief Parses a product type from its canonical snake_case string.
 */
inline ProductType parse_product_type(const std::string& value) {
    if (value == "callable_bond") return ProductType::CallableBond;
    if (value == "cap_floor") return ProductType::CapFloor;
    if (value == "cds") return ProductType::Cds;
    if (value == "cds_option") return ProductType::CdsOption;
    if (value == "commodity_future") return ProductType::CommodityFuture;
    if (value == "commodity_future_option") return ProductType::CommodityFutureOption;
    if (value == "commodity_swing") return ProductType::CommoditySwing;
    if (value == "credit_bond") return ProductType::CreditBond;
    if (value == "cross_currency_swap") return ProductType::CrossCurrencySwap;
    if (value == "deposit") return ProductType::Deposit;
    if (value == "equity_option") return ProductType::EquityOption;
    if (value == "equity_spot") return ProductType::EquitySpot;
    if (value == "fixed_rate_bond") return ProductType::FixedRateBond;
    if (value == "fra") return ProductType::Fra;
    if (value == "future") return ProductType::Future;
    if (value == "fx_forward") return ProductType::FxForward;
    if (value == "fx_option") return ProductType::FxOption;
    if (value == "ois_swap") return ProductType::OisSwap;
    if (value == "swaption") return ProductType::Swaption;
    if (value == "vanilla_swap") return ProductType::VanillaSwap;
    return ProductType::Unknown;
}

/**
 * @brief Returns every product type declared by the platform taxonomy.
 */
inline std::vector<ProductType> all_product_types() {
    return {
        ProductType::CallableBond,
        ProductType::CapFloor,
        ProductType::Cds,
        ProductType::CdsOption,
        ProductType::CommodityFuture,
        ProductType::CommodityFutureOption,
        ProductType::CommoditySwing,
        ProductType::CreditBond,
        ProductType::CrossCurrencySwap,
        ProductType::Deposit,
        ProductType::EquityOption,
        ProductType::EquitySpot,
        ProductType::FixedRateBond,
        ProductType::Fra,
        ProductType::Future,
        ProductType::FxForward,
        ProductType::FxOption,
        ProductType::OisSwap,
        ProductType::Swaption,
        ProductType::VanillaSwap,
        ProductType::Unknown
    };
}

/**
 * @brief Resolves the owning asset class for a product type.
 */
inline AssetClass asset_class_from_product_type(ProductType product_type) {
    switch (product_type) {
        case ProductType::CallableBond: return AssetClass::Rates;
        case ProductType::CapFloor: return AssetClass::Rates;
        case ProductType::Cds: return AssetClass::Credit;
        case ProductType::CdsOption: return AssetClass::Credit;
        case ProductType::CommodityFuture: return AssetClass::Commodity;
        case ProductType::CommodityFutureOption: return AssetClass::Commodity;
        case ProductType::CommoditySwing: return AssetClass::Commodity;
        case ProductType::CreditBond: return AssetClass::Credit;
        case ProductType::CrossCurrencySwap: return AssetClass::FX;
        case ProductType::Deposit: return AssetClass::Rates;
        case ProductType::EquityOption: return AssetClass::Equity;
        case ProductType::EquitySpot: return AssetClass::Equity;
        case ProductType::FixedRateBond: return AssetClass::Rates;
        case ProductType::Fra: return AssetClass::Rates;
        case ProductType::Future: return AssetClass::Rates;
        case ProductType::FxForward: return AssetClass::FX;
        case ProductType::FxOption: return AssetClass::FX;
        case ProductType::OisSwap: return AssetClass::Rates;
        case ProductType::Swaption: return AssetClass::Rates;
        case ProductType::VanillaSwap: return AssetClass::Rates;
        case ProductType::Unknown: return AssetClass::Unknown;
    }
    return AssetClass::Unknown;
}

/**
 * @brief Converts a product type to its canonical snake_case string.
 */
inline std::string to_string(ProductType product_type) {
    switch (product_type) {
        case ProductType::CallableBond: return "callable_bond";
        case ProductType::CapFloor: return "cap_floor";
        case ProductType::Cds: return "cds";
        case ProductType::CdsOption: return "cds_option";
        case ProductType::CommodityFuture: return "commodity_future";
        case ProductType::CommodityFutureOption: return "commodity_future_option";
        case ProductType::CommoditySwing: return "commodity_swing";
        case ProductType::CreditBond: return "credit_bond";
        case ProductType::CrossCurrencySwap: return "cross_currency_swap";
        case ProductType::Deposit: return "deposit";
        case ProductType::EquityOption: return "equity_option";
        case ProductType::EquitySpot: return "equity_spot";
        case ProductType::FixedRateBond: return "fixed_rate_bond";
        case ProductType::Fra: return "fra";
        case ProductType::Future: return "future";
        case ProductType::FxForward: return "fx_forward";
        case ProductType::FxOption: return "fx_option";
        case ProductType::OisSwap: return "ois_swap";
        case ProductType::Swaption: return "swaption";
        case ProductType::VanillaSwap: return "vanilla_swap";
        case ProductType::Unknown: return "unknown";
    }
    return "unknown";
}

/**
 * @brief Converts a support status to its canonical storage/reporting string.
 */
inline std::string to_string(SupportStatus status) {
    switch (status) {
        case SupportStatus::Failed: return "failed";
        case SupportStatus::PartiallySupported: return "partially_supported";
        case SupportStatus::Supported: return "supported";
        case SupportStatus::Unsupported: return "unsupported";
    }
    return "failed";
}

} // namespace qrp::domain
