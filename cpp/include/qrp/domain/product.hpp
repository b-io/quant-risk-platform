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
    // Commodity
    CommoditySpot,
    CommodityForward,
    CommodityFuture,
    CommodityFutureStrip,
    CommodityFutureOption,
    CommodityCalendarSpreadOption,
    CommoditySwing,

    // Credit
    CreditBond,
    Cds,
    CdsIndex,
    CdsOption,
    CreditIndexOption,

    // Equity
    EquitySpot,
    EquityForward,
    EquityFuture,
    EquityOption,

    // FX
    FxSpot,
    FxForward,
    FxSwap,
    Ndf,
    FxOption,
    CrossCurrencySwap,

    // Rates
    Deposit,
    Fra,
    InterestRateFuture,
    VanillaSwap,
    OisSwap,
    FixedRateBond,
    FloatingRateNote,
    CapFloor,
    EuropeanSwaption,
    BermudanSwaption,
    CallableBond,

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
    if (value == "commodity" || value == "commodities" || value == "Commodity")
        return AssetClass::Commodity;
    if (value == "credit" || value == "Credit")
        return AssetClass::Credit;
    if (value == "equity" || value == "Equity")
        return AssetClass::Equity;
    if (value == "fx" || value == "FX")
        return AssetClass::FX;
    if (value == "inflation" || value == "Inflation")
        return AssetClass::Inflation;
    if (value == "rates" || value == "Rates" || value == "IR")
        return AssetClass::Rates;
    return AssetClass::Unknown;
}

/**
 * @brief Converts an asset class to its canonical lowercase string.
 */
inline std::string to_string(AssetClass asset_class) {
    switch (asset_class) {
        case AssetClass::Commodity:
            return "commodity";
        case AssetClass::Credit:
            return "credit";
        case AssetClass::Equity:
            return "equity";
        case AssetClass::FX:
            return "fx";
        case AssetClass::Inflation:
            return "inflation";
        case AssetClass::Rates:
            return "rates";
        case AssetClass::Unknown:
            return "unknown";
    }
    return "unknown";
}

/**
 * @brief Parses a product type from its canonical snake_case string.
 */
inline ProductType parse_product_type(const std::string& value) {
    // Commodity
    if (value == "commodity_spot")
        return ProductType::CommoditySpot;
    if (value == "commodity_forward")
        return ProductType::CommodityForward;
    if (value == "commodity_future")
        return ProductType::CommodityFuture;
    if (value == "commodity_future_strip")
        return ProductType::CommodityFutureStrip;
    if (value == "commodity_future_option")
        return ProductType::CommodityFutureOption;
    if (value == "commodity_calendar_spread_option")
        return ProductType::CommodityCalendarSpreadOption;
    if (value == "commodity_swing")
        return ProductType::CommoditySwing;

    // Credit
    if (value == "credit_bond")
        return ProductType::CreditBond;
    if (value == "cds")
        return ProductType::Cds;
    if (value == "cds_index")
        return ProductType::CdsIndex;
    if (value == "cds_option")
        return ProductType::CdsOption;
    if (value == "credit_index_option")
        return ProductType::CreditIndexOption;

    // Equity
    if (value == "equity_spot")
        return ProductType::EquitySpot;
    if (value == "equity_forward")
        return ProductType::EquityForward;
    if (value == "equity_future")
        return ProductType::EquityFuture;
    if (value == "equity_option")
        return ProductType::EquityOption;

    // FX
    if (value == "fx_spot")
        return ProductType::FxSpot;
    if (value == "fx_forward")
        return ProductType::FxForward;
    if (value == "fx_swap")
        return ProductType::FxSwap;
    if (value == "ndf")
        return ProductType::Ndf;
    if (value == "fx_option")
        return ProductType::FxOption;
    if (value == "cross_currency_swap")
        return ProductType::CrossCurrencySwap;

    // Rates
    if (value == "deposit")
        return ProductType::Deposit;
    if (value == "fra")
        return ProductType::Fra;
    if (value == "interest_rate_future")
        return ProductType::InterestRateFuture;
    if (value == "vanilla_swap")
        return ProductType::VanillaSwap;
    if (value == "ois_swap")
        return ProductType::OisSwap;
    if (value == "fixed_rate_bond")
        return ProductType::FixedRateBond;
    if (value == "floating_rate_note")
        return ProductType::FloatingRateNote;
    if (value == "cap_floor")
        return ProductType::CapFloor;
    if (value == "european_swaption")
        return ProductType::EuropeanSwaption;
    if (value == "bermudan_swaption")
        return ProductType::BermudanSwaption;
    if (value == "callable_bond")
        return ProductType::CallableBond;
    return ProductType::Unknown;
}

/**
 * @brief Returns every product type declared by the platform taxonomy.
 */
inline std::vector<ProductType> all_product_types() {
    return {ProductType::CommoditySpot,
            ProductType::CommodityForward,
            ProductType::CommodityFuture,
            ProductType::CommodityFutureStrip,
            ProductType::CommodityFutureOption,
            ProductType::CommodityCalendarSpreadOption,
            ProductType::CommoditySwing,
            ProductType::CreditBond,
            ProductType::Cds,
            ProductType::CdsIndex,
            ProductType::CdsOption,
            ProductType::CreditIndexOption,
            ProductType::EquitySpot,
            ProductType::EquityForward,
            ProductType::EquityFuture,
            ProductType::EquityOption,
            ProductType::FxSpot,
            ProductType::FxForward,
            ProductType::FxSwap,
            ProductType::Ndf,
            ProductType::FxOption,
            ProductType::CrossCurrencySwap,
            ProductType::Deposit,
            ProductType::Fra,
            ProductType::InterestRateFuture,
            ProductType::VanillaSwap,
            ProductType::OisSwap,
            ProductType::FixedRateBond,
            ProductType::FloatingRateNote,
            ProductType::CapFloor,
            ProductType::EuropeanSwaption,
            ProductType::BermudanSwaption,
            ProductType::CallableBond,
            ProductType::Unknown};
}

/**
 * @brief Resolves the owning asset class for a product type.
 */
inline AssetClass asset_class_from_product_type(ProductType product_type) {
    switch (product_type) {
        // Commodity
        case ProductType::CommoditySpot:
            return AssetClass::Commodity;
        case ProductType::CommodityForward:
            return AssetClass::Commodity;
        case ProductType::CommodityFuture:
            return AssetClass::Commodity;
        case ProductType::CommodityFutureStrip:
            return AssetClass::Commodity;
        case ProductType::CommodityFutureOption:
            return AssetClass::Commodity;
        case ProductType::CommodityCalendarSpreadOption:
            return AssetClass::Commodity;
        case ProductType::CommoditySwing:
            return AssetClass::Commodity;

        // Credit
        case ProductType::CreditBond:
            return AssetClass::Credit;
        case ProductType::Cds:
            return AssetClass::Credit;
        case ProductType::CdsIndex:
            return AssetClass::Credit;
        case ProductType::CdsOption:
            return AssetClass::Credit;
        case ProductType::CreditIndexOption:
            return AssetClass::Credit;

        // Equity
        case ProductType::EquitySpot:
            return AssetClass::Equity;
        case ProductType::EquityForward:
            return AssetClass::Equity;
        case ProductType::EquityFuture:
            return AssetClass::Equity;
        case ProductType::EquityOption:
            return AssetClass::Equity;

        // FX
        case ProductType::FxSpot:
            return AssetClass::FX;
        case ProductType::FxForward:
            return AssetClass::FX;
        case ProductType::FxSwap:
            return AssetClass::FX;
        case ProductType::Ndf:
            return AssetClass::FX;
        case ProductType::FxOption:
            return AssetClass::FX;
        case ProductType::CrossCurrencySwap:
            return AssetClass::FX;

        // Rates
        case ProductType::Deposit:
            return AssetClass::Rates;
        case ProductType::Fra:
            return AssetClass::Rates;
        case ProductType::InterestRateFuture:
            return AssetClass::Rates;
        case ProductType::VanillaSwap:
            return AssetClass::Rates;
        case ProductType::OisSwap:
            return AssetClass::Rates;
        case ProductType::FixedRateBond:
            return AssetClass::Rates;
        case ProductType::FloatingRateNote:
            return AssetClass::Rates;
        case ProductType::CapFloor:
            return AssetClass::Rates;
        case ProductType::EuropeanSwaption:
            return AssetClass::Rates;
        case ProductType::BermudanSwaption:
            return AssetClass::Rates;
        case ProductType::CallableBond:
            return AssetClass::Rates;
        case ProductType::Unknown:
            return AssetClass::Unknown;
    }
    return AssetClass::Unknown;
}

/**
 * @brief Converts a product type to its canonical snake_case string.
 */
inline std::string to_string(ProductType product_type) {
    switch (product_type) {
        // Commodity
        case ProductType::CommoditySpot:
            return "commodity_spot";
        case ProductType::CommodityForward:
            return "commodity_forward";
        case ProductType::CommodityFuture:
            return "commodity_future";
        case ProductType::CommodityFutureStrip:
            return "commodity_future_strip";
        case ProductType::CommodityFutureOption:
            return "commodity_future_option";
        case ProductType::CommodityCalendarSpreadOption:
            return "commodity_calendar_spread_option";
        case ProductType::CommoditySwing:
            return "commodity_swing";

        // Credit
        case ProductType::CreditBond:
            return "credit_bond";
        case ProductType::Cds:
            return "cds";
        case ProductType::CdsIndex:
            return "cds_index";
        case ProductType::CdsOption:
            return "cds_option";
        case ProductType::CreditIndexOption:
            return "credit_index_option";

        // Equity
        case ProductType::EquitySpot:
            return "equity_spot";
        case ProductType::EquityForward:
            return "equity_forward";
        case ProductType::EquityFuture:
            return "equity_future";
        case ProductType::EquityOption:
            return "equity_option";

        // FX
        case ProductType::FxSpot:
            return "fx_spot";
        case ProductType::FxForward:
            return "fx_forward";
        case ProductType::FxSwap:
            return "fx_swap";
        case ProductType::Ndf:
            return "ndf";
        case ProductType::FxOption:
            return "fx_option";
        case ProductType::CrossCurrencySwap:
            return "cross_currency_swap";

        // Rates
        case ProductType::Deposit:
            return "deposit";
        case ProductType::Fra:
            return "fra";
        case ProductType::InterestRateFuture:
            return "interest_rate_future";
        case ProductType::VanillaSwap:
            return "vanilla_swap";
        case ProductType::OisSwap:
            return "ois_swap";
        case ProductType::FixedRateBond:
            return "fixed_rate_bond";
        case ProductType::FloatingRateNote:
            return "floating_rate_note";
        case ProductType::CapFloor:
            return "cap_floor";
        case ProductType::EuropeanSwaption:
            return "european_swaption";
        case ProductType::BermudanSwaption:
            return "bermudan_swaption";
        case ProductType::CallableBond:
            return "callable_bond";
        case ProductType::Unknown:
            return "unknown";
    }
    return "unknown";
}

/**
 * @brief Converts a support status to its canonical storage/reporting string.
 */
inline std::string to_string(SupportStatus status) {
    switch (status) {
        case SupportStatus::Failed:
            return "failed";
        case SupportStatus::PartiallySupported:
            return "partially_supported";
        case SupportStatus::Supported:
            return "supported";
        case SupportStatus::Unsupported:
            return "unsupported";
    }
    return "failed";
}

} // namespace qrp::domain
