#pragma once

// Defines canonical trade and portfolio DTOs used by persistence, JSON, analytics, and bindings.

#include <qrp/domain/product.hpp>

#include <nlohmann/json.hpp>

#include <initializer_list>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace qrp::domain {

namespace portfolio_detail {

/**
 * @brief Reads an optional numeric detail, accepting the first matching field name.
 */
inline double optional_double(
    const nlohmann::json& j,
    std::initializer_list<std::string> fields,
    double fallback = 0.0) {
    for (const auto& field : fields) {
        if (j.contains(field)) {
            return j.at(field).get<double>();
        }
    }
    return fallback;
}

/**
 * @brief Reads an optional string detail, accepting the first matching field name.
 */
inline std::string optional_string(
    const nlohmann::json& j,
    std::initializer_list<std::string> fields,
    std::string fallback = {}) {
    for (const auto& field : fields) {
        if (j.contains(field)) {
            return j.at(field).get<std::string>();
        }
    }
    return fallback;
}

/**
 * @brief Reads an optional string-vector detail, returning an empty vector when absent.
 */
inline std::vector<std::string> optional_string_vector(
    const nlohmann::json& j,
    std::initializer_list<std::string> fields) {
    for (const auto& field : fields) {
        if (j.contains(field)) {
            return j.at(field).get<std::vector<std::string>>();
        }
    }
    return {};
}

} // namespace portfolio_detail

/**
 * @brief Trade implementation types currently supported by the canonical portfolio DTO.
 */
enum class TradeType {
    // Rates products use the Phase 3 business order.
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

    // FX products use the Phase 4 business order.
    FxSpot,
    FxForward,
    FxSwap,
    Ndf,
    FxOption,

    // Other asset-class products remain grouped after their phases expand.
    EquitySpot,
    Unknown
};

/**
 * @brief Parses a canonical trade type string from JSON or storage.
 */
inline TradeType parse_trade_type(const std::string& value) {
    if (value == "deposit") return TradeType::Deposit;
    if (value == "fra") return TradeType::Fra;
    if (value == "interest_rate_future") return TradeType::InterestRateFuture;
    if (value == "vanilla_swap") return TradeType::VanillaSwap;
    if (value == "ois_swap") return TradeType::OisSwap;
    if (value == "fixed_rate_bond") return TradeType::FixedRateBond;
    if (value == "floating_rate_note") return TradeType::FloatingRateNote;
    if (value == "cap_floor") return TradeType::CapFloor;
    if (value == "european_swaption") return TradeType::EuropeanSwaption;
    if (value == "bermudan_swaption") return TradeType::BermudanSwaption;
    if (value == "fx_spot") return TradeType::FxSpot;
    if (value == "fx_forward") return TradeType::FxForward;
    if (value == "fx_swap") return TradeType::FxSwap;
    if (value == "ndf") return TradeType::Ndf;
    if (value == "fx_option") return TradeType::FxOption;
    if (value == "equity_spot") return TradeType::EquitySpot;
    throw std::runtime_error("Unknown trade type: " + value);
}

/**
 * @brief Converts a trade type to its canonical snake_case string.
 */
inline std::string to_string(TradeType type) {
    switch (type) {
        case TradeType::Deposit: return "deposit";
        case TradeType::Fra: return "fra";
        case TradeType::InterestRateFuture: return "interest_rate_future";
        case TradeType::VanillaSwap: return "vanilla_swap";
        case TradeType::OisSwap: return "ois_swap";
        case TradeType::FixedRateBond: return "fixed_rate_bond";
        case TradeType::FloatingRateNote: return "floating_rate_note";
        case TradeType::CapFloor: return "cap_floor";
        case TradeType::EuropeanSwaption: return "european_swaption";
        case TradeType::BermudanSwaption: return "bermudan_swaption";
        case TradeType::FxSpot: return "fx_spot";
        case TradeType::FxForward: return "fx_forward";
        case TradeType::FxSwap: return "fx_swap";
        case TradeType::Ndf: return "ndf";
        case TradeType::FxOption: return "fx_option";
        case TradeType::EquitySpot: return "equity_spot";
        case TradeType::Unknown: return "unknown";
    }
    return "unknown";
}

/**
 * @brief Maps a concrete trade type to the broader product taxonomy.
 */
inline ProductType product_type_from_trade_type(TradeType type) {
    switch (type) {
        case TradeType::Deposit: return ProductType::Deposit;
        case TradeType::Fra: return ProductType::Fra;
        case TradeType::InterestRateFuture: return ProductType::InterestRateFuture;
        case TradeType::VanillaSwap: return ProductType::VanillaSwap;
        case TradeType::OisSwap: return ProductType::OisSwap;
        case TradeType::FixedRateBond: return ProductType::FixedRateBond;
        case TradeType::FloatingRateNote: return ProductType::FloatingRateNote;
        case TradeType::CapFloor: return ProductType::CapFloor;
        case TradeType::EuropeanSwaption: return ProductType::EuropeanSwaption;
        case TradeType::BermudanSwaption: return ProductType::BermudanSwaption;
        case TradeType::FxSpot: return ProductType::FxSpot;
        case TradeType::FxForward: return ProductType::FxForward;
        case TradeType::FxSwap: return ProductType::FxSwap;
        case TradeType::Ndf: return ProductType::Ndf;
        case TradeType::FxOption: return ProductType::FxOption;
        case TradeType::EquitySpot: return ProductType::EquitySpot;
        case TradeType::Unknown: return ProductType::Unknown;
    }
    return ProductType::Unknown;
}

/**
 * @brief Base trade DTO shared by all concrete trade economics.
 */
struct Trade {
    std::string id;
    std::string asset_class;
    std::string type;
    AssetClass asset_class_type = AssetClass::Unknown;
    ProductType product_type = ProductType::Unknown;
    TradeType trade_type = TradeType::Unknown;
    std::string currency;
    std::string direction;
    std::string book;
    std::string strategy;

    /**
     * @brief Allows derived trade DTOs to be deleted through base pointers.
     */
    virtual ~Trade() = default;

    /**
     * @brief Reads fields common to every trade from JSON.
     */
    virtual void from_json(const nlohmann::json& j) {
        j.at("id").get_to(id);
        j.at("asset_class").get_to(asset_class);
        asset_class_type = parse_asset_class(asset_class);
        trade_type = parse_trade_type(j.at("type").get<std::string>());
        type = to_string(trade_type);
        product_type = product_type_from_trade_type(trade_type);
        j.at("currency").get_to(currency);
        j.at("direction").get_to(direction);
        j.at("book").get_to(book);
        j.at("strategy").get_to(strategy);
    }
};

/**
 * @brief Cash deposit or money-market lending trade DTO.
 */
struct DepositTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for deposits.
     */
    DepositTrade() {
        trade_type = TradeType::Deposit;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double notional = 0.0;
    std::string start_date;
    std::string maturity_date;
    double deposit_rate = 0.0;

    /**
     * @brief Reads deposit economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        j.at("notional").get_to(notional);
        j.at("start_date").get_to(start_date);
        j.at("maturity_date").get_to(maturity_date);

        const auto& details = j.at("details");
        deposit_rate = portfolio_detail::optional_double(details, {"deposit_rate", "rate"});
    }
};

/**
 * @brief Forward-rate agreement trade DTO.
 */
struct FraTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for FRAs.
     */
    FraTrade() {
        trade_type = TradeType::Fra;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double notional = 0.0;
    std::string start_date;
    std::string maturity_date;
    double strike_rate = 0.0;
    std::string floating_index;

    /**
     * @brief Reads FRA economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        j.at("notional").get_to(notional);
        j.at("start_date").get_to(start_date);
        j.at("maturity_date").get_to(maturity_date);

        const auto& details = j.at("details");
        strike_rate = portfolio_detail::optional_double(details, {"strike_rate", "fixed_rate", "rate"});
        floating_index = portfolio_detail::optional_string(details, {"floating_index", "index_family"}, "IBOR_3M");
    }
};

/**
 * @brief Exchange-traded short-rate futures trade DTO.
 */
struct InterestRateFutureTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for interest-rate futures.
     */
    InterestRateFutureTrade() {
        trade_type = TradeType::InterestRateFuture;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double quantity = 0.0;
    double contract_size = 2500.0;
    double reference_price = 0.0;
    std::string start_date;
    std::string maturity_date;
    std::string floating_index;
    std::string future_quote_id;

    /**
     * @brief Reads short-rate future economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        if (j.contains("quantity")) {
            j.at("quantity").get_to(quantity);
        }
        if (j.contains("start_date")) {
            j.at("start_date").get_to(start_date);
        }
        if (j.contains("maturity_date")) {
            j.at("maturity_date").get_to(maturity_date);
        }

        const auto& details = j.at("details");
        contract_size = portfolio_detail::optional_double(details, {"contract_size"}, contract_size);
        floating_index = portfolio_detail::optional_string(details, {"floating_index", "index_family"}, "IBOR_3M");
        future_quote_id = portfolio_detail::optional_string(details, {"future_quote_id", "quote_id"});
        reference_price = portfolio_detail::optional_double(details, {"reference_price", "trade_price", "price"});
    }
};

/**
 * @brief Fixed-float vanilla interest-rate swap trade DTO.
 */
struct VanillaSwapTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for vanilla swaps.
     */
    VanillaSwapTrade() {
        trade_type = TradeType::VanillaSwap;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double notional = 0.0;
    std::string start_date;
    std::string maturity_date;
    double fixed_rate = 0.0;
    std::string floating_index;

    /**
     * @brief Reads swap economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        j.at("notional").get_to(notional);
        j.at("start_date").get_to(start_date);
        j.at("maturity_date").get_to(maturity_date);

        const auto& details = j.at("details");
        details.at("fixed_rate").get_to(fixed_rate);
        details.at("floating_index").get_to(floating_index);
    }
};

/**
 * @brief Overnight-indexed fixed-float swap trade DTO.
 */
struct OisSwapTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for OIS swaps.
     */
    OisSwapTrade() {
        trade_type = TradeType::OisSwap;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double notional = 0.0;
    std::string start_date;
    std::string maturity_date;
    double fixed_rate = 0.0;
    double spread = 0.0;
    std::string overnight_index;

    /**
     * @brief Reads OIS swap economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        j.at("notional").get_to(notional);
        j.at("start_date").get_to(start_date);
        j.at("maturity_date").get_to(maturity_date);

        const auto& details = j.at("details");
        fixed_rate = portfolio_detail::optional_double(details, {"fixed_rate", "rate"});
        overnight_index = portfolio_detail::optional_string(details, {"overnight_index", "floating_index"}, "OIS");
        spread = portfolio_detail::optional_double(details, {"spread"});
    }
};

/**
 * @brief Fixed-rate bullet bond trade DTO.
 */
struct FixedRateBondTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for fixed-rate bonds.
     */
    FixedRateBondTrade() {
        trade_type = TradeType::FixedRateBond;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double notional = 0.0;
    std::string start_date;
    std::string maturity_date;
    double coupon_rate = 0.0;
    std::string frequency;

    /**
     * @brief Reads fixed-rate bond economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        j.at("notional").get_to(notional);
        j.at("start_date").get_to(start_date);
        j.at("maturity_date").get_to(maturity_date);

        const auto& details = j.at("details");
        details.at("coupon_rate").get_to(coupon_rate);
        details.at("frequency").get_to(frequency);
    }
};

/**
 * @brief Floating-rate note trade DTO.
 */
struct FloatingRateNoteTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for floating-rate notes.
     */
    FloatingRateNoteTrade() {
        trade_type = TradeType::FloatingRateNote;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double notional = 0.0;
    std::string start_date;
    std::string maturity_date;
    double spread = 0.0;
    std::string floating_index;
    std::string frequency;

    /**
     * @brief Reads floating-rate note economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        j.at("notional").get_to(notional);
        j.at("start_date").get_to(start_date);
        j.at("maturity_date").get_to(maturity_date);

        const auto& details = j.at("details");
        floating_index = portfolio_detail::optional_string(details, {"floating_index", "index_family"}, "IBOR_3M");
        frequency = portfolio_detail::optional_string(details, {"frequency"}, "Quarterly");
        spread = portfolio_detail::optional_double(details, {"spread"});
    }
};

/**
 * @brief Cap, floor, or collar on an IBOR-style floating leg.
 */
struct CapFloorTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for caps and floors.
     */
    CapFloorTrade() {
        trade_type = TradeType::CapFloor;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double notional = 0.0;
    std::string start_date;
    std::string maturity_date;
    double strike_rate = 0.0;
    double volatility = 0.0;
    std::string cap_floor_type = "cap";
    std::string floating_index;
    std::string volatility_quote_id;

    /**
     * @brief Reads cap/floor economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        j.at("notional").get_to(notional);
        j.at("start_date").get_to(start_date);
        j.at("maturity_date").get_to(maturity_date);

        const auto& details = j.at("details");
        cap_floor_type = portfolio_detail::optional_string(details, {"cap_floor_type", "option_type"}, cap_floor_type);
        floating_index = portfolio_detail::optional_string(details, {"floating_index", "index_family"}, "IBOR_3M");
        strike_rate = portfolio_detail::optional_double(details, {"strike_rate", "strike"});
        volatility = portfolio_detail::optional_double(details, {"volatility"});
        volatility_quote_id = portfolio_detail::optional_string(details, {"volatility_quote_id", "vol_quote_id"});
    }
};

/**
 * @brief European option to enter a vanilla fixed-float swap.
 */
struct EuropeanSwaptionTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for European swaptions.
     */
    EuropeanSwaptionTrade() {
        trade_type = TradeType::EuropeanSwaption;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double notional = 0.0;
    std::string option_expiry_date;
    std::string start_date;
    std::string maturity_date;
    double fixed_rate = 0.0;
    double volatility = 0.0;
    std::string floating_index;
    std::string volatility_quote_id;

    /**
     * @brief Reads European swaption economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        j.at("notional").get_to(notional);
        j.at("start_date").get_to(start_date);
        j.at("maturity_date").get_to(maturity_date);

        const auto& details = j.at("details");
        fixed_rate = portfolio_detail::optional_double(details, {"fixed_rate", "strike_rate", "strike"});
        floating_index = portfolio_detail::optional_string(details, {"floating_index", "index_family"}, "IBOR_3M");
        option_expiry_date = portfolio_detail::optional_string(details, {"option_expiry_date", "expiry_date"});
        volatility = portfolio_detail::optional_double(details, {"volatility"});
        volatility_quote_id = portfolio_detail::optional_string(details, {"volatility_quote_id", "vol_quote_id"});
    }
};

/**
 * @brief Bermudan option to enter a vanilla fixed-float swap.
 */
struct BermudanSwaptionTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for Bermudan swaptions.
     */
    BermudanSwaptionTrade() {
        trade_type = TradeType::BermudanSwaption;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double notional = 0.0;
    std::string start_date;
    std::string maturity_date;
    double fixed_rate = 0.0;
    double mean_reversion = 0.03;
    double volatility = 0.01;
    std::vector<std::string> exercise_dates;
    std::string floating_index;
    std::string volatility_quote_id;

    /**
     * @brief Reads Bermudan swaption economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        j.at("notional").get_to(notional);
        j.at("start_date").get_to(start_date);
        j.at("maturity_date").get_to(maturity_date);

        const auto& details = j.at("details");
        exercise_dates = portfolio_detail::optional_string_vector(details, {"exercise_dates"});
        fixed_rate = portfolio_detail::optional_double(details, {"fixed_rate", "strike_rate", "strike"});
        floating_index = portfolio_detail::optional_string(details, {"floating_index", "index_family"}, "IBOR_3M");
        mean_reversion = portfolio_detail::optional_double(details, {"mean_reversion"}, mean_reversion);
        volatility = portfolio_detail::optional_double(details, {"volatility"}, volatility);
        volatility_quote_id = portfolio_detail::optional_string(details, {"volatility_quote_id", "vol_quote_id"});
    }
};

/**
 * @brief FX spot exposure in base-currency notional against a quote-currency rate.
 */
struct FxSpotTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for FX spot exposure.
     */
    FxSpotTrade() {
        trade_type = TradeType::FxSpot;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double notional = 0.0;
    double reference_rate = 0.0;
    std::string base_currency;
    std::string quote_currency;
    std::string spot_quote_id;

    /**
     * @brief Reads FX spot economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        j.at("notional").get_to(notional);

        const auto& details = j.at("details");
        base_currency = portfolio_detail::optional_string(details, {"base_currency"});
        quote_currency = portfolio_detail::optional_string(details, {"quote_currency"}, currency);
        reference_rate = portfolio_detail::optional_double(details, {"reference_rate", "reference_price", "spot_rate"});
        spot_quote_id = portfolio_detail::optional_string(details, {"spot_quote_id", "quote_id"});
    }
};

/**
 * @brief Deliverable FX forward in base-currency notional.
 */
struct FxForwardTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for FX forwards.
     */
    FxForwardTrade() {
        trade_type = TradeType::FxForward;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double notional = 0.0;
    std::string start_date;
    std::string maturity_date;
    std::string base_currency;
    std::string quote_currency;
    double forward_rate = 0.0;
    std::string forward_points_quote_id;
    std::string forward_quote_id;
    std::string spot_quote_id;

    /**
     * @brief Reads FX forward economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        j.at("notional").get_to(notional);
        j.at("start_date").get_to(start_date);
        j.at("maturity_date").get_to(maturity_date);

        const auto& details = j.at("details");
        details.at("base_currency").get_to(base_currency);
        details.at("quote_currency").get_to(quote_currency);
        details.at("forward_rate").get_to(forward_rate);
        forward_points_quote_id = portfolio_detail::optional_string(details, {"forward_points_quote_id"});
        forward_quote_id = portfolio_detail::optional_string(details, {"forward_quote_id"});
        spot_quote_id = portfolio_detail::optional_string(details, {"spot_quote_id", "quote_id"});
    }
};

/**
 * @brief FX swap represented as near and far deliverable forward legs.
 */
struct FxSwapTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for FX swaps.
     */
    FxSwapTrade() {
        trade_type = TradeType::FxSwap;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double notional = 0.0;
    std::string start_date;
    std::string maturity_date;
    std::string base_currency;
    std::string quote_currency;
    double near_rate = 0.0;
    double far_rate = 0.0;
    std::string far_forward_points_quote_id;
    std::string far_forward_quote_id;
    std::string near_forward_points_quote_id;
    std::string near_forward_quote_id;
    std::string spot_quote_id;

    /**
     * @brief Reads FX swap economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        j.at("notional").get_to(notional);
        j.at("start_date").get_to(start_date);
        j.at("maturity_date").get_to(maturity_date);

        const auto& details = j.at("details");
        details.at("base_currency").get_to(base_currency);
        details.at("quote_currency").get_to(quote_currency);
        near_rate = portfolio_detail::optional_double(details, {"near_rate", "near_forward_rate", "spot_rate"});
        far_rate = portfolio_detail::optional_double(details, {"far_rate", "far_forward_rate", "forward_rate"});
        far_forward_points_quote_id = portfolio_detail::optional_string(details, {"far_forward_points_quote_id"});
        far_forward_quote_id = portfolio_detail::optional_string(details, {"far_forward_quote_id"});
        near_forward_points_quote_id = portfolio_detail::optional_string(details, {"near_forward_points_quote_id"});
        near_forward_quote_id = portfolio_detail::optional_string(details, {"near_forward_quote_id"});
        spot_quote_id = portfolio_detail::optional_string(details, {"spot_quote_id", "quote_id"});
    }
};

/**
 * @brief Non-deliverable FX forward settled in the quote currency.
 */
struct NdfTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for NDFs.
     */
    NdfTrade() {
        trade_type = TradeType::Ndf;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double notional = 0.0;
    std::string fixing_date;
    std::string maturity_date;
    std::string base_currency;
    std::string quote_currency;
    double forward_rate = 0.0;
    std::string fixing_quote_id;
    std::string forward_points_quote_id;
    std::string forward_quote_id;
    std::string spot_quote_id;

    /**
     * @brief Reads NDF economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        j.at("notional").get_to(notional);
        j.at("maturity_date").get_to(maturity_date);

        const auto& details = j.at("details");
        details.at("base_currency").get_to(base_currency);
        details.at("quote_currency").get_to(quote_currency);
        details.at("forward_rate").get_to(forward_rate);
        fixing_date = portfolio_detail::optional_string(details, {"fixing_date"});
        fixing_quote_id = portfolio_detail::optional_string(details, {"fixing_quote_id"});
        forward_points_quote_id = portfolio_detail::optional_string(details, {"forward_points_quote_id"});
        forward_quote_id = portfolio_detail::optional_string(details, {"forward_quote_id"});
        spot_quote_id = portfolio_detail::optional_string(details, {"spot_quote_id", "quote_id"});
    }
};

/**
 * @brief Vanilla European FX option in base-currency notional.
 */
struct FxOptionTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for vanilla European FX options.
     */
    FxOptionTrade() {
        trade_type = TradeType::FxOption;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double notional = 0.0;
    std::string expiry_date;
    std::string settlement_date;
    std::string base_currency;
    std::string quote_currency;
    double strike_rate = 0.0;
    double volatility = 0.0;
    std::string option_type = "call";
    std::string spot_quote_id;
    std::string volatility_quote_id;

    /**
     * @brief Reads FX option economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        j.at("notional").get_to(notional);

        const auto& details = j.at("details");
        details.at("base_currency").get_to(base_currency);
        details.at("quote_currency").get_to(quote_currency);
        strike_rate = portfolio_detail::optional_double(details, {"strike_rate", "strike"});
        volatility = portfolio_detail::optional_double(details, {"volatility"});
        expiry_date = portfolio_detail::optional_string(details, {"expiry_date", "option_expiry_date"});
        option_type = portfolio_detail::optional_string(details, {"option_type", "call_put"}, option_type);
        settlement_date = portfolio_detail::optional_string(details, {"settlement_date", "maturity_date"}, expiry_date);
        spot_quote_id = portfolio_detail::optional_string(details, {"spot_quote_id", "quote_id"});
        volatility_quote_id = portfolio_detail::optional_string(details, {"volatility_quote_id", "vol_quote_id"});
    }
};

/**
 * @brief Equity spot exposure trade DTO.
 */
struct EquitySpotTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for equity spot trades.
     */
    EquitySpotTrade() {
        trade_type = TradeType::EquitySpot;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double quantity = 0.0;
    double reference_price = 0.0;
    std::string underlier;

    /**
     * @brief Reads equity spot economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        j.at("quantity").get_to(quantity);

        const auto& details = j.at("details");
        details.at("reference_price").get_to(reference_price);
        details.at("underlier").get_to(underlier);
    }
};

/**
 * @brief Portfolio DTO containing a stable id and owned trade collection.
 */
struct Portfolio {
    std::string portfolio_id;
    std::vector<std::shared_ptr<Trade>> trades;
};

/**
 * @brief Constructs the concrete trade DTO for a parsed trade type.
 */
inline std::shared_ptr<Trade> make_trade(TradeType type) {
    switch (type) {
        case TradeType::Deposit: return std::make_shared<DepositTrade>();
        case TradeType::Fra: return std::make_shared<FraTrade>();
        case TradeType::InterestRateFuture: return std::make_shared<InterestRateFutureTrade>();
        case TradeType::VanillaSwap: return std::make_shared<VanillaSwapTrade>();
        case TradeType::OisSwap: return std::make_shared<OisSwapTrade>();
        case TradeType::FixedRateBond: return std::make_shared<FixedRateBondTrade>();
        case TradeType::FloatingRateNote: return std::make_shared<FloatingRateNoteTrade>();
        case TradeType::CapFloor: return std::make_shared<CapFloorTrade>();
        case TradeType::EuropeanSwaption: return std::make_shared<EuropeanSwaptionTrade>();
        case TradeType::BermudanSwaption: return std::make_shared<BermudanSwaptionTrade>();
        case TradeType::FxSpot: return std::make_shared<FxSpotTrade>();
        case TradeType::FxForward: return std::make_shared<FxForwardTrade>();
        case TradeType::FxSwap: return std::make_shared<FxSwapTrade>();
        case TradeType::Ndf: return std::make_shared<NdfTrade>();
        case TradeType::FxOption: return std::make_shared<FxOptionTrade>();
        case TradeType::EquitySpot: return std::make_shared<EquitySpotTrade>();
        case TradeType::Unknown: break;
    }
    throw std::runtime_error("Cannot construct unknown trade type");
}

/**
 * @brief Constructs the concrete trade DTO for a canonical trade type string.
 */
inline std::shared_ptr<Trade> make_trade(const std::string& type) {
    return make_trade(parse_trade_type(type));
}

/**
 * @brief Deserializes a portfolio and its concrete trade DTOs from JSON.
 */
inline void from_json(const nlohmann::json& j, Portfolio& p) {
    j.at("portfolio_id").get_to(p.portfolio_id);
    p.trades.clear();
    for (const auto& tj : j.at("trades")) {
        auto trade = make_trade(tj.at("type").get<std::string>());
        trade->from_json(tj);
        p.trades.push_back(trade);
    }
}

} // namespace qrp::domain
