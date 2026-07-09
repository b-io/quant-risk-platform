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
inline double
optional_double(const nlohmann::json& j, std::initializer_list<std::string> fields, double fallback = 0.0) {
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
inline std::string
optional_string(const nlohmann::json& j, std::initializer_list<std::string> fields, std::string fallback = {}) {
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
inline std::vector<std::string> optional_string_vector(const nlohmann::json& j,
                                                       std::initializer_list<std::string> fields) {
    for (const auto& field : fields) {
        if (j.contains(field)) {
            return j.at(field).get<std::vector<std::string>>();
        }
    }
    return {};
}

/**
 * @brief Reads an optional numeric-vector detail, returning an empty vector when absent.
 */
inline std::vector<double> optional_double_vector(const nlohmann::json& j, std::initializer_list<std::string> fields) {
    for (const auto& field : fields) {
        if (j.contains(field)) {
            return j.at(field).get<std::vector<double>>();
        }
    }
    return {};
}

} // namespace portfolio_detail

/**
 * @brief Trade implementation types currently supported by the canonical portfolio DTO.
 */
enum class TradeType {
    // Commodity products use the commodity business order.
    CommoditySpot,
    CommodityForward,
    CommodityFuture,
    CommodityFutureStrip,
    CommodityFutureOption,
    CommodityCalendarSpreadOption,
    CommoditySwing,

    // Credit products use the credit business order.
    CreditBond,
    Cds,
    CdsIndex,
    CdsOption,
    CreditIndexOption,

    // Equity products use the equity business order.
    EquitySpot,
    EquityForward,
    EquityFuture,
    EquityOption,

    // FX products use the FX business order.
    FxSpot,
    FxForward,
    FxSwap,
    Ndf,
    FxOption,

    // Rates products use the rates business order.
    Deposit,
    Fra,
    InterestRateFuture,
    VanillaSwap,
    OisSwap,
    FixedRateBond,
    CallableBond,
    FloatingRateNote,
    CapFloor,
    EuropeanSwaption,
    BermudanSwaption,

    Unknown
};

/**
 * @brief Parses a canonical trade type string from JSON or storage.
 */
inline TradeType parse_trade_type(const std::string& value) {
    if (value == "commodity_spot")
        return TradeType::CommoditySpot;
    if (value == "commodity_forward")
        return TradeType::CommodityForward;
    if (value == "commodity_future")
        return TradeType::CommodityFuture;
    if (value == "commodity_future_strip")
        return TradeType::CommodityFutureStrip;
    if (value == "commodity_future_option")
        return TradeType::CommodityFutureOption;
    if (value == "commodity_calendar_spread_option")
        return TradeType::CommodityCalendarSpreadOption;
    if (value == "commodity_swing")
        return TradeType::CommoditySwing;
    if (value == "credit_bond")
        return TradeType::CreditBond;
    if (value == "cds")
        return TradeType::Cds;
    if (value == "cds_index")
        return TradeType::CdsIndex;
    if (value == "cds_option")
        return TradeType::CdsOption;
    if (value == "credit_index_option")
        return TradeType::CreditIndexOption;
    if (value == "equity_spot")
        return TradeType::EquitySpot;
    if (value == "equity_forward")
        return TradeType::EquityForward;
    if (value == "equity_future")
        return TradeType::EquityFuture;
    if (value == "equity_option")
        return TradeType::EquityOption;
    if (value == "fx_spot")
        return TradeType::FxSpot;
    if (value == "fx_forward")
        return TradeType::FxForward;
    if (value == "fx_swap")
        return TradeType::FxSwap;
    if (value == "ndf")
        return TradeType::Ndf;
    if (value == "fx_option")
        return TradeType::FxOption;
    if (value == "deposit")
        return TradeType::Deposit;
    if (value == "fra")
        return TradeType::Fra;
    if (value == "interest_rate_future")
        return TradeType::InterestRateFuture;
    if (value == "vanilla_swap")
        return TradeType::VanillaSwap;
    if (value == "ois_swap")
        return TradeType::OisSwap;
    if (value == "fixed_rate_bond")
        return TradeType::FixedRateBond;
    if (value == "callable_bond")
        return TradeType::CallableBond;
    if (value == "floating_rate_note")
        return TradeType::FloatingRateNote;
    if (value == "cap_floor")
        return TradeType::CapFloor;
    if (value == "european_swaption")
        return TradeType::EuropeanSwaption;
    if (value == "bermudan_swaption")
        return TradeType::BermudanSwaption;
    throw std::runtime_error("Unknown trade type: " + value);
}

/**
 * @brief Converts a trade type to its canonical snake_case string.
 */
inline std::string to_string(TradeType type) {
    switch (type) {
        case TradeType::CommoditySpot:
            return "commodity_spot";
        case TradeType::CommodityForward:
            return "commodity_forward";
        case TradeType::CommodityFuture:
            return "commodity_future";
        case TradeType::CommodityFutureStrip:
            return "commodity_future_strip";
        case TradeType::CommodityFutureOption:
            return "commodity_future_option";
        case TradeType::CommodityCalendarSpreadOption:
            return "commodity_calendar_spread_option";
        case TradeType::CommoditySwing:
            return "commodity_swing";
        case TradeType::CreditBond:
            return "credit_bond";
        case TradeType::Cds:
            return "cds";
        case TradeType::CdsIndex:
            return "cds_index";
        case TradeType::CdsOption:
            return "cds_option";
        case TradeType::CreditIndexOption:
            return "credit_index_option";
        case TradeType::EquitySpot:
            return "equity_spot";
        case TradeType::EquityForward:
            return "equity_forward";
        case TradeType::EquityFuture:
            return "equity_future";
        case TradeType::EquityOption:
            return "equity_option";
        case TradeType::FxSpot:
            return "fx_spot";
        case TradeType::FxForward:
            return "fx_forward";
        case TradeType::FxSwap:
            return "fx_swap";
        case TradeType::Ndf:
            return "ndf";
        case TradeType::FxOption:
            return "fx_option";
        case TradeType::Deposit:
            return "deposit";
        case TradeType::Fra:
            return "fra";
        case TradeType::InterestRateFuture:
            return "interest_rate_future";
        case TradeType::VanillaSwap:
            return "vanilla_swap";
        case TradeType::OisSwap:
            return "ois_swap";
        case TradeType::FixedRateBond:
            return "fixed_rate_bond";
        case TradeType::CallableBond:
            return "callable_bond";
        case TradeType::FloatingRateNote:
            return "floating_rate_note";
        case TradeType::CapFloor:
            return "cap_floor";
        case TradeType::EuropeanSwaption:
            return "european_swaption";
        case TradeType::BermudanSwaption:
            return "bermudan_swaption";
        case TradeType::Unknown:
            return "unknown";
    }
    return "unknown";
}

/**
 * @brief Maps a concrete trade type to the broader product taxonomy.
 */
inline ProductType product_type_from_trade_type(TradeType type) {
    switch (type) {
        case TradeType::CommoditySpot:
            return ProductType::CommoditySpot;
        case TradeType::CommodityForward:
            return ProductType::CommodityForward;
        case TradeType::CommodityFuture:
            return ProductType::CommodityFuture;
        case TradeType::CommodityFutureStrip:
            return ProductType::CommodityFutureStrip;
        case TradeType::CommodityFutureOption:
            return ProductType::CommodityFutureOption;
        case TradeType::CommodityCalendarSpreadOption:
            return ProductType::CommodityCalendarSpreadOption;
        case TradeType::CommoditySwing:
            return ProductType::CommoditySwing;
        case TradeType::CreditBond:
            return ProductType::CreditBond;
        case TradeType::Cds:
            return ProductType::Cds;
        case TradeType::CdsIndex:
            return ProductType::CdsIndex;
        case TradeType::CdsOption:
            return ProductType::CdsOption;
        case TradeType::CreditIndexOption:
            return ProductType::CreditIndexOption;
        case TradeType::EquitySpot:
            return ProductType::EquitySpot;
        case TradeType::EquityForward:
            return ProductType::EquityForward;
        case TradeType::EquityFuture:
            return ProductType::EquityFuture;
        case TradeType::EquityOption:
            return ProductType::EquityOption;
        case TradeType::FxSpot:
            return ProductType::FxSpot;
        case TradeType::FxForward:
            return ProductType::FxForward;
        case TradeType::FxSwap:
            return ProductType::FxSwap;
        case TradeType::Ndf:
            return ProductType::Ndf;
        case TradeType::FxOption:
            return ProductType::FxOption;
        case TradeType::Deposit:
            return ProductType::Deposit;
        case TradeType::Fra:
            return ProductType::Fra;
        case TradeType::InterestRateFuture:
            return ProductType::InterestRateFuture;
        case TradeType::VanillaSwap:
            return ProductType::VanillaSwap;
        case TradeType::OisSwap:
            return ProductType::OisSwap;
        case TradeType::FixedRateBond:
            return ProductType::FixedRateBond;
        case TradeType::CallableBond:
            return ProductType::CallableBond;
        case TradeType::FloatingRateNote:
            return ProductType::FloatingRateNote;
        case TradeType::CapFloor:
            return ProductType::CapFloor;
        case TradeType::EuropeanSwaption:
            return ProductType::EuropeanSwaption;
        case TradeType::BermudanSwaption:
            return ProductType::BermudanSwaption;
        case TradeType::Unknown:
            return ProductType::Unknown;
    }
    return ProductType::Unknown;
}

/**
 * @brief Base trade DTO shared by all concrete trade economics.
 */
struct Trade {
    std::string id;                                    // Stable trade identifier.
    std::string asset_class;                           // Canonical asset-class label from the payload.
    std::string type;                                  // Canonical trade type label from the payload.
    AssetClass asset_class_type = AssetClass::Unknown; // Parsed asset-class enum.
    ProductType product_type = ProductType::Unknown;   // Parsed product taxonomy enum.
    TradeType trade_type = TradeType::Unknown;         // Parsed concrete trade DTO enum.
    std::string currency;                              // Reporting or settlement currency.
    std::string direction;                             // Long/short, buy/sell, payer/receiver, or pay/receive side.
    std::string book;                                  // Owning book inside the portfolio.
    std::string strategy;                              // Strategy, mandate, or sleeve label.

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

    double notional = 0.0;     // Principal exchanged at start and repaid at maturity.
    std::string start_date;    // Deposit start date.
    std::string maturity_date; // Deposit maturity date.
    double deposit_rate = 0.0; // Simple annualized deposit rate.

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

    double notional = 0.0;      // FRA notional used for settlement payoff.
    std::string start_date;     // Forward accrual start date.
    std::string maturity_date;  // Forward accrual end date.
    double strike_rate = 0.0;   // Contract fixed rate.
    std::string floating_index; // Floating index family, such as IBOR_3M.

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

    double quantity = 0.0;         // Number of listed futures contracts.
    double contract_size = 2500.0; // Monetary value of one price point per contract.
    double reference_price = 0.0;  // Trade entry futures price.
    std::string start_date;        // Optional accrual start date for the implied deposit.
    std::string maturity_date;     // Futures expiry or delivery date.
    std::string floating_index;    // Reference short-rate index family.
    std::string future_quote_id;   // Optional explicit futures quote id.

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

    double notional = 0.0;      // Swap notional.
    std::string start_date;     // Swap effective date.
    std::string maturity_date;  // Swap maturity date.
    double fixed_rate = 0.0;    // Fixed leg coupon rate.
    std::string floating_index; // Floating leg index family.

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

    double notional = 0.0;       // OIS notional.
    std::string start_date;      // Swap effective date.
    std::string maturity_date;   // Swap maturity date.
    double fixed_rate = 0.0;     // Fixed leg coupon rate.
    double spread = 0.0;         // Optional spread on the overnight leg.
    std::string overnight_index; // Overnight index family.

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

    double notional = 0.0;     // Bond face amount.
    std::string start_date;    // Bond issue or accrual start date.
    std::string maturity_date; // Bond maturity date.
    double coupon_rate = 0.0;  // Fixed coupon rate.
    std::string frequency;     // Coupon payment frequency.

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
 * @brief Fixed-rate bond with issuer call rights.
 */
struct CallableBondTrade : public FixedRateBondTrade {
    /**
     * @brief Initializes taxonomy fields for callable bonds.
     */
    CallableBondTrade() {
        trade_type = TradeType::CallableBond;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    std::vector<std::string> call_dates; // Dates on which the issuer may call the bond.
    std::vector<double> call_prices;     // Call prices as percentages of par, aligned with call_dates.
    double mean_reversion = 0.03;        // One-factor rates-driver mean-reversion parameter.
    double volatility = 0.01;            // Fallback short-rate volatility.
    std::string volatility_quote_id;     // Optional rates-volatility quote id.

    /**
     * @brief Reads callable-bond economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        FixedRateBondTrade::from_json(j);
        trade_type = TradeType::CallableBond;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);

        const auto& details = j.at("details");
        call_dates = portfolio_detail::optional_string_vector(details, {"call_dates", "exercise_dates"});
        call_prices = portfolio_detail::optional_double_vector(details, {"call_prices", "exercise_prices"});
        if (call_prices.empty() && details.contains("call_price")) {
            call_prices.assign(call_dates.size(), details.at("call_price").get<double>());
        }
        if (call_prices.empty() && !call_dates.empty()) {
            call_prices.assign(call_dates.size(), 100.0);
        }
        mean_reversion = portfolio_detail::optional_double(details, {"mean_reversion"}, mean_reversion);
        volatility = portfolio_detail::optional_double(details, {"volatility"}, volatility);
        volatility_quote_id = portfolio_detail::optional_string(details, {"volatility_quote_id", "vol_quote_id"});
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

    double notional = 0.0;      // Note face amount.
    std::string start_date;     // Issue or accrual start date.
    std::string maturity_date;  // Note maturity date.
    double spread = 0.0;        // Quoted spread over the floating index.
    std::string floating_index; // Floating coupon index family.
    std::string frequency;      // Coupon reset and payment frequency.

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

    double notional = 0.0;              // Option notional.
    std::string start_date;             // Floating leg start date.
    std::string maturity_date;          // Cap/floor maturity date.
    double strike_rate = 0.0;           // Cap or floor strike rate.
    double volatility = 0.0;            // Fallback normal/Black volatility when no quote id is supplied.
    std::string cap_floor_type = "cap"; // Cap or floor payoff direction.
    std::string floating_index;         // Floating index family.
    std::string volatility_quote_id;    // Optional cap/floor volatility quote id.

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

    double notional = 0.0;           // Underlying swap notional.
    std::string option_expiry_date;  // Swaption exercise date.
    std::string start_date;          // Underlying swap effective date.
    std::string maturity_date;       // Underlying swap maturity date.
    double fixed_rate = 0.0;         // Underlying swap fixed rate or option strike.
    double volatility = 0.0;         // Fallback swaption volatility when no quote id is supplied.
    std::string floating_index;      // Underlying swap floating index family.
    std::string volatility_quote_id; // Optional swaption volatility quote id.

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

    double notional = 0.0;                   // Underlying swap notional.
    std::string start_date;                  // Underlying swap effective date.
    std::string maturity_date;               // Underlying swap maturity date.
    double fixed_rate = 0.0;                 // Underlying swap fixed rate or option strike.
    double mean_reversion = 0.03;            // Short-rate mean-reversion parameter.
    double volatility = 0.01;                // Fallback short-rate volatility.
    std::vector<std::string> exercise_dates; // Bermudan exercise schedule dates.
    std::string floating_index;              // Underlying swap floating index family.
    std::string volatility_quote_id;         // Optional swaption volatility quote id.

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

    double notional = 0.0;       // Base-currency notional.
    double reference_rate = 0.0; // Trade entry spot rate.
    std::string base_currency;   // Currency bought or sold as the base leg.
    std::string quote_currency;  // Pricing and settlement quote currency.
    std::string spot_quote_id;   // Optional explicit FX spot quote id.

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

    double notional = 0.0;               // Base-currency notional.
    std::string start_date;              // Forward trade start date.
    std::string maturity_date;           // Forward settlement date.
    std::string base_currency;           // Currency bought or sold as the base leg.
    std::string quote_currency;          // Pricing and settlement quote currency.
    double forward_rate = 0.0;           // Contract forward rate.
    std::string forward_points_quote_id; // Optional forward-points quote id.
    std::string forward_quote_id;        // Optional outright forward quote id.
    std::string spot_quote_id;           // Optional fallback FX spot quote id.

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

    double notional = 0.0;                    // Base-currency notional exchanged on both legs.
    std::string start_date;                   // Near-leg settlement date.
    std::string maturity_date;                // Far-leg settlement date.
    std::string base_currency;                // Currency bought or sold as the base leg.
    std::string quote_currency;               // Pricing and settlement quote currency.
    double near_rate = 0.0;                   // Near-leg agreed FX rate.
    double far_rate = 0.0;                    // Far-leg agreed FX rate.
    std::string far_forward_points_quote_id;  // Optional far-leg forward-points quote id.
    std::string far_forward_quote_id;         // Optional far-leg outright forward quote id.
    std::string near_forward_points_quote_id; // Optional near-leg forward-points quote id.
    std::string near_forward_quote_id;        // Optional near-leg outright forward quote id.
    std::string spot_quote_id;                // Optional fallback FX spot quote id.

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

    double notional = 0.0;               // Base-currency notional.
    std::string fixing_date;             // NDF fixing date.
    std::string maturity_date;           // Cash settlement date.
    std::string base_currency;           // Non-deliverable base currency.
    std::string quote_currency;          // Deliverable settlement currency.
    double forward_rate = 0.0;           // Contract NDF rate.
    std::string fixing_quote_id;         // Optional fixing-rate quote id.
    std::string forward_points_quote_id; // Optional forward-points quote id.
    std::string forward_quote_id;        // Optional outright forward quote id.
    std::string spot_quote_id;           // Optional fallback FX spot quote id.

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

    double notional = 0.0;            // Base-currency option notional.
    std::string expiry_date;          // Option expiry date.
    std::string settlement_date;      // Premium/payoff settlement date.
    std::string base_currency;        // Option base currency.
    std::string quote_currency;       // Option quote and settlement currency.
    double strike_rate = 0.0;         // Option strike FX rate.
    double volatility = 0.0;          // Fallback volatility when no quote id is supplied.
    std::string option_type = "call"; // Call or put option type.
    std::string spot_quote_id;        // Optional explicit FX spot quote id.
    std::string volatility_quote_id;  // Optional FX volatility quote id.

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
 * @brief Credit bond valued from risk-free discounting plus issuer spread discounting.
 */
struct CreditBondTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for credit bonds.
     */
    CreditBondTrade() {
        trade_type = TradeType::CreditBond;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double notional = 0.0;            // Bond face amount.
    std::string start_date;           // Bond issue or accrual start date.
    std::string maturity_date;        // Bond maturity date.
    double coupon_rate = 0.0;         // Fixed coupon rate.
    double credit_spread = 0.0;       // Fallback issuer spread when no quote id is supplied.
    std::string frequency = "Annual"; // Coupon payment frequency.
    std::string issuer;               // Issuer or reference entity.
    std::string spread_quote_id;      // Optional issuer spread quote id.

    /**
     * @brief Reads credit-bond economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        j.at("notional").get_to(notional);
        j.at("start_date").get_to(start_date);
        j.at("maturity_date").get_to(maturity_date);

        const auto& details = j.at("details");
        coupon_rate = portfolio_detail::optional_double(details, {"coupon_rate", "fixed_rate", "rate"});
        credit_spread = portfolio_detail::optional_double(details, {"credit_spread", "spread", "z_spread"});
        frequency = portfolio_detail::optional_string(details, {"frequency"}, frequency);
        issuer = portfolio_detail::optional_string(details, {"issuer", "reference_entity", "underlier"});
        spread_quote_id =
            portfolio_detail::optional_string(details, {"spread_quote_id", "credit_spread_quote_id", "quote_id"});
    }
};

/**
 * @brief Single-name credit default swap trade DTO.
 */
struct CdsTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for single-name CDS trades.
     */
    CdsTrade() {
        trade_type = TradeType::Cds;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double notional = 0.0;               // CDS protection notional.
    std::string start_date;              // Premium leg start date.
    std::string maturity_date;           // Protection maturity date.
    double coupon_rate = 0.0;            // Running premium coupon.
    double recovery_rate = 0.40;         // Fallback recovery assumption.
    std::string frequency = "Quarterly"; // Premium payment frequency.
    std::string issuer;                  // Reference entity.
    std::string recovery_quote_id;       // Optional recovery-rate quote id.
    std::string spread_quote_id;         // Optional CDS spread quote id.

    /**
     * @brief Reads single-name CDS economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        j.at("notional").get_to(notional);
        j.at("start_date").get_to(start_date);
        j.at("maturity_date").get_to(maturity_date);

        const auto& details = j.at("details");
        coupon_rate = portfolio_detail::optional_double(details, {"coupon_rate", "running_spread", "spread"});
        frequency = portfolio_detail::optional_string(details, {"frequency"}, frequency);
        issuer = portfolio_detail::optional_string(details, {"issuer", "reference_entity", "underlier"});
        recovery_quote_id = portfolio_detail::optional_string(details, {"recovery_quote_id"});
        recovery_rate = portfolio_detail::optional_double(details, {"recovery_rate"}, recovery_rate);
        spread_quote_id = portfolio_detail::optional_string(details, {"spread_quote_id", "cds_quote_id", "quote_id"});
    }
};

/**
 * @brief CDS index trade DTO using index spread and recovery inputs.
 */
struct CdsIndexTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for CDS index trades.
     */
    CdsIndexTrade() {
        trade_type = TradeType::CdsIndex;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double notional = 0.0;               // CDS index protection notional.
    std::string start_date;              // Premium leg start date.
    std::string maturity_date;           // Protection maturity date.
    double coupon_rate = 0.0;            // Running premium coupon.
    double index_factor = 1.0;           // Current index factor applied to notional.
    double recovery_rate = 0.40;         // Fallback recovery assumption.
    std::string frequency = "Quarterly"; // Premium payment frequency.
    std::string index_name;              // CDS index family or series name.
    std::string recovery_quote_id;       // Optional recovery-rate quote id.
    std::string spread_quote_id;         // Optional CDS index spread quote id.

    /**
     * @brief Reads CDS index economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        j.at("notional").get_to(notional);
        j.at("start_date").get_to(start_date);
        j.at("maturity_date").get_to(maturity_date);

        const auto& details = j.at("details");
        coupon_rate = portfolio_detail::optional_double(details, {"coupon_rate", "running_spread", "spread"});
        frequency = portfolio_detail::optional_string(details, {"frequency"}, frequency);
        index_factor = portfolio_detail::optional_double(details, {"index_factor"}, index_factor);
        index_name =
            portfolio_detail::optional_string(details, {"index_name", "issuer", "reference_entity", "underlier"});
        recovery_quote_id = portfolio_detail::optional_string(details, {"recovery_quote_id"});
        recovery_rate = portfolio_detail::optional_double(details, {"recovery_rate"}, recovery_rate);
        spread_quote_id = portfolio_detail::optional_string(details, {"spread_quote_id", "cds_quote_id", "quote_id"});
    }
};

/**
 * @brief European option on a single-name CDS spread.
 */
struct CdsOptionTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for CDS options.
     */
    CdsOptionTrade() {
        trade_type = TradeType::CdsOption;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double notional = 0.0;               // Underlying CDS protection notional.
    std::string expiry_date;             // Option expiry date.
    std::string maturity_date;           // Underlying CDS maturity date.
    double strike_spread = 0.0;          // Option strike spread.
    double volatility = 0.0;             // Fallback spread volatility.
    double recovery_rate = 0.40;         // Fallback recovery assumption.
    std::string frequency = "Quarterly"; // Underlying premium payment frequency.
    std::string issuer;                  // Reference entity.
    std::string option_type = "call";    // Call or put on spread/protection value.
    std::string recovery_quote_id;       // Optional recovery-rate quote id.
    std::string spread_quote_id;         // Optional underlying CDS spread quote id.
    std::string volatility_quote_id;     // Optional credit volatility quote id.

    /**
     * @brief Reads CDS option economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        j.at("notional").get_to(notional);
        j.at("maturity_date").get_to(maturity_date);

        const auto& details = j.at("details");
        expiry_date = portfolio_detail::optional_string(details, {"expiry_date", "option_expiry_date"});
        frequency = portfolio_detail::optional_string(details, {"frequency"}, frequency);
        issuer = portfolio_detail::optional_string(details, {"issuer", "reference_entity", "underlier"});
        option_type = portfolio_detail::optional_string(details, {"option_type", "payer_receiver"}, option_type);
        recovery_quote_id = portfolio_detail::optional_string(details, {"recovery_quote_id"});
        recovery_rate = portfolio_detail::optional_double(details, {"recovery_rate"}, recovery_rate);
        spread_quote_id = portfolio_detail::optional_string(details, {"spread_quote_id", "cds_quote_id", "quote_id"});
        strike_spread = portfolio_detail::optional_double(details, {"strike_spread", "strike", "strike_rate"});
        volatility = portfolio_detail::optional_double(details, {"volatility"});
        volatility_quote_id = portfolio_detail::optional_string(details, {"volatility_quote_id", "vol_quote_id"});
    }
};

/**
 * @brief European option on a CDS index spread.
 */
struct CreditIndexOptionTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for credit index options.
     */
    CreditIndexOptionTrade() {
        trade_type = TradeType::CreditIndexOption;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double notional = 0.0;               // Underlying CDS index protection notional.
    std::string expiry_date;             // Option expiry date.
    std::string maturity_date;           // Underlying CDS index maturity date.
    double index_factor = 1.0;           // Current index factor applied to notional.
    double strike_spread = 0.0;          // Option strike spread.
    double volatility = 0.0;             // Fallback spread volatility.
    double recovery_rate = 0.40;         // Fallback recovery assumption.
    std::string frequency = "Quarterly"; // Underlying premium payment frequency.
    std::string index_name;              // CDS index family or series name.
    std::string option_type = "call";    // Call or put on spread/protection value.
    std::string recovery_quote_id;       // Optional recovery-rate quote id.
    std::string spread_quote_id;         // Optional underlying index spread quote id.
    std::string volatility_quote_id;     // Optional credit-index volatility quote id.

    /**
     * @brief Reads credit index option economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        j.at("notional").get_to(notional);
        j.at("maturity_date").get_to(maturity_date);

        const auto& details = j.at("details");
        expiry_date = portfolio_detail::optional_string(details, {"expiry_date", "option_expiry_date"});
        frequency = portfolio_detail::optional_string(details, {"frequency"}, frequency);
        index_factor = portfolio_detail::optional_double(details, {"index_factor"}, index_factor);
        index_name =
            portfolio_detail::optional_string(details, {"index_name", "issuer", "reference_entity", "underlier"});
        option_type = portfolio_detail::optional_string(details, {"option_type", "payer_receiver"}, option_type);
        recovery_quote_id = portfolio_detail::optional_string(details, {"recovery_quote_id"});
        recovery_rate = portfolio_detail::optional_double(details, {"recovery_rate"}, recovery_rate);
        spread_quote_id = portfolio_detail::optional_string(details, {"spread_quote_id", "cds_quote_id", "quote_id"});
        strike_spread = portfolio_detail::optional_double(details, {"strike_spread", "strike", "strike_rate"});
        volatility = portfolio_detail::optional_double(details, {"volatility"});
        volatility_quote_id = portfolio_detail::optional_string(details, {"volatility_quote_id", "vol_quote_id"});
    }
};

/**
 * @brief Spot commodity inventory or index exposure marked to a spot quote.
 */
struct CommoditySpotTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for commodity spot trades.
     */
    CommoditySpotTrade() {
        trade_type = TradeType::CommoditySpot;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double quantity = 0.0;        // Physical or financial units exposed to the spot quote.
    double reference_price = 0.0; // Trade entry price used for mark-to-market PnL.
    std::string spot_quote_id;    // Optional explicit spot quote id.
    std::string underlier;        // Commodity name, hub, grade, or benchmark.
    std::string unit;             // Unit label such as bbl, MWh, or MMBtu.

    /**
     * @brief Reads commodity spot economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        const auto& details = j.at("details");
        quantity = j.contains("quantity") ? j.at("quantity").get<double>()
                                          : portfolio_detail::optional_double(details, {"quantity", "notional"});
        reference_price = portfolio_detail::optional_double(details, {"reference_price", "trade_price", "price"});
        spot_quote_id = portfolio_detail::optional_string(details, {"spot_quote_id", "quote_id"});
        underlier = portfolio_detail::optional_string(details, {"underlier", "commodity", "benchmark"});
        unit = portfolio_detail::optional_string(details, {"unit"});
    }
};

/**
 * @brief OTC commodity forward settled against a forward or spot quote.
 */
struct CommodityForwardTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for commodity forwards.
     */
    CommodityForwardTrade() {
        trade_type = TradeType::CommodityForward;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double quantity = 0.0;        // Contract quantity in the configured unit.
    double contract_price = 0.0;  // Fixed delivery price agreed in the forward.
    std::string forward_quote_id; // Optional explicit forward quote id.
    std::string maturity_date;    // Settlement or delivery end date.
    std::string spot_quote_id;    // Optional fallback spot quote id.
    std::string tenor;            // Forward tenor used when selecting market quotes.
    std::string underlier;        // Commodity name, hub, grade, or benchmark.
    std::string unit;             // Unit label such as bbl, MWh, or MMBtu.

    /**
     * @brief Reads commodity forward economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        if (j.contains("maturity_date")) {
            j.at("maturity_date").get_to(maturity_date);
        }
        const auto& details = j.at("details");
        quantity = j.contains("quantity") ? j.at("quantity").get<double>()
                                          : portfolio_detail::optional_double(details, {"quantity", "notional"});
        contract_price =
            portfolio_detail::optional_double(details, {"contract_price", "forward_price", "trade_price", "price"});
        forward_quote_id =
            portfolio_detail::optional_string(details, {"forward_quote_id", "future_quote_id", "quote_id"});
        maturity_date = portfolio_detail::optional_string(details, {"maturity_date", "delivery_date"}, maturity_date);
        spot_quote_id = portfolio_detail::optional_string(details, {"spot_quote_id"});
        tenor = portfolio_detail::optional_string(details, {"tenor"});
        underlier = portfolio_detail::optional_string(details, {"underlier", "commodity", "benchmark"});
        unit = portfolio_detail::optional_string(details, {"unit"});
    }
};

/**
 * @brief Exchange-traded commodity future exposure marked to a futures quote.
 */
struct CommodityFutureTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for commodity futures.
     */
    CommodityFutureTrade() {
        trade_type = TradeType::CommodityFuture;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double contract_size = 1.0;   // Units represented by one listed future.
    double quantity = 0.0;        // Number of futures contracts.
    double reference_price = 0.0; // Trade entry futures price.
    std::string future_quote_id;  // Optional explicit futures quote id.
    std::string maturity_date;    // Futures expiry or delivery date.
    std::string tenor;            // Futures tenor used when selecting market quotes.
    std::string underlier;        // Commodity name, hub, grade, or benchmark.
    std::string unit;             // Unit label such as bbl, MWh, or MMBtu.

    /**
     * @brief Reads commodity future economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        if (j.contains("maturity_date")) {
            j.at("maturity_date").get_to(maturity_date);
        }
        const auto& details = j.at("details");
        contract_size = portfolio_detail::optional_double(details, {"contract_size"}, contract_size);
        quantity = j.contains("quantity")
                       ? j.at("quantity").get<double>()
                       : portfolio_detail::optional_double(details, {"quantity", "contracts", "notional"});
        reference_price = portfolio_detail::optional_double(details, {"reference_price", "trade_price", "price"});
        future_quote_id = portfolio_detail::optional_string(details, {"future_quote_id", "quote_id"});
        maturity_date = portfolio_detail::optional_string(details,
                                                          {"maturity_date", "expiry_date", "delivery_date"},
                                                          maturity_date);
        tenor = portfolio_detail::optional_string(details, {"tenor"});
        underlier = portfolio_detail::optional_string(details, {"underlier", "commodity", "benchmark"});
        unit = portfolio_detail::optional_string(details, {"unit"});
    }
};

/**
 * @brief Basket of listed commodity futures represented by a weighted strip price.
 */
struct CommodityFutureStripTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for commodity futures strips.
     */
    CommodityFutureStripTrade() {
        trade_type = TradeType::CommodityFutureStrip;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double contract_size = 1.0;                // Units represented by one listed future.
    double quantity = 0.0;                     // Number of strip-equivalent contracts.
    double reference_price = 0.0;              // Weighted entry price for the strip.
    std::string maturity_date;                 // Last delivery date in the strip.
    std::string start_date;                    // First delivery date in the strip.
    std::string underlier;                     // Commodity name, hub, grade, or benchmark.
    std::string unit;                          // Unit label such as bbl, MWh, or MMBtu.
    std::vector<std::string> future_quote_ids; // Futures quotes included in the strip.
    std::vector<double> weights;               // Optional strip weights; equal weights are used when empty.

    /**
     * @brief Reads commodity futures strip economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        if (j.contains("start_date")) {
            j.at("start_date").get_to(start_date);
        }
        if (j.contains("maturity_date")) {
            j.at("maturity_date").get_to(maturity_date);
        }
        const auto& details = j.at("details");
        contract_size = portfolio_detail::optional_double(details, {"contract_size"}, contract_size);
        quantity = j.contains("quantity")
                       ? j.at("quantity").get<double>()
                       : portfolio_detail::optional_double(details, {"quantity", "contracts", "notional"});
        reference_price = portfolio_detail::optional_double(details, {"reference_price", "trade_price", "price"});
        future_quote_ids = portfolio_detail::optional_string_vector(details, {"future_quote_ids", "quote_ids"});
        if (details.contains("weights")) {
            weights = details.at("weights").get<std::vector<double>>();
        }
        maturity_date = portfolio_detail::optional_string(details, {"maturity_date", "delivery_end"}, maturity_date);
        start_date = portfolio_detail::optional_string(details, {"start_date", "delivery_start"}, start_date);
        underlier = portfolio_detail::optional_string(details, {"underlier", "commodity", "benchmark"});
        unit = portfolio_detail::optional_string(details, {"unit"});
    }
};

/**
 * @brief European option on a listed commodity future.
 */
struct CommodityFutureOptionTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for commodity options on futures.
     */
    CommodityFutureOptionTrade() {
        trade_type = TradeType::CommodityFutureOption;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double contract_size = 1.0;       // Units represented by one listed option contract.
    double quantity = 0.0;            // Number of option contracts.
    double strike_price = 0.0;        // Option strike in price units.
    double volatility = 0.0;          // Fallback Black volatility when no quote id is supplied.
    std::string expiry_date;          // Option expiry date.
    std::string future_quote_id;      // Explicit underlying futures quote id.
    std::string maturity_date;        // Underlying future maturity or delivery date.
    std::string option_type = "call"; // Call or put option style.
    std::string tenor;                // Futures tenor used when selecting market quotes.
    std::string underlier;            // Commodity name, hub, grade, or benchmark.
    std::string unit;                 // Unit label such as bbl, MWh, or MMBtu.
    std::string volatility_quote_id;  // Optional volatility quote id.

    /**
     * @brief Reads commodity future option economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        if (j.contains("maturity_date")) {
            j.at("maturity_date").get_to(maturity_date);
        }
        const auto& details = j.at("details");
        contract_size = portfolio_detail::optional_double(details, {"contract_size"}, contract_size);
        quantity = j.contains("quantity")
                       ? j.at("quantity").get<double>()
                       : portfolio_detail::optional_double(details, {"quantity", "contracts", "notional"});
        expiry_date = portfolio_detail::optional_string(details, {"expiry_date", "option_expiry_date"});
        future_quote_id =
            portfolio_detail::optional_string(details, {"future_quote_id", "forward_quote_id", "quote_id"});
        maturity_date = portfolio_detail::optional_string(details,
                                                          {"maturity_date", "future_maturity_date", "delivery_date"},
                                                          maturity_date);
        option_type = portfolio_detail::optional_string(details, {"option_type", "call_put"}, option_type);
        strike_price = portfolio_detail::optional_double(details, {"strike_price", "strike"});
        tenor = portfolio_detail::optional_string(details, {"tenor"});
        underlier = portfolio_detail::optional_string(details, {"underlier", "commodity", "benchmark"});
        unit = portfolio_detail::optional_string(details, {"unit"});
        volatility = portfolio_detail::optional_double(details, {"volatility"});
        volatility_quote_id = portfolio_detail::optional_string(details, {"volatility_quote_id", "vol_quote_id"});
    }
};

/**
 * @brief European option on the spread between two commodity futures.
 */
struct CommodityCalendarSpreadOptionTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for commodity calendar spread options.
     */
    CommodityCalendarSpreadOptionTrade() {
        trade_type = TradeType::CommodityCalendarSpreadOption;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double contract_size = 1.0;       // Units represented by one spread option contract.
    double quantity = 0.0;            // Number of spread option contracts.
    double strike_spread = 0.0;       // Strike on far-minus-near spread.
    double volatility = 0.0;          // Fallback spread volatility when no quote id is supplied.
    std::string expiry_date;          // Option expiry date.
    std::string far_future_quote_id;  // Far-leg futures quote id.
    std::string near_future_quote_id; // Near-leg futures quote id.
    std::string option_type = "call"; // Call or put on the spread.
    std::string underlier;            // Commodity name, hub, grade, or benchmark.
    std::string unit;                 // Unit label such as bbl, MWh, or MMBtu.
    std::string volatility_quote_id;  // Optional spread volatility quote id.

    /**
     * @brief Reads commodity calendar spread option economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        const auto& details = j.at("details");
        contract_size = portfolio_detail::optional_double(details, {"contract_size"}, contract_size);
        quantity = j.contains("quantity")
                       ? j.at("quantity").get<double>()
                       : portfolio_detail::optional_double(details, {"quantity", "contracts", "notional"});
        expiry_date = portfolio_detail::optional_string(details, {"expiry_date", "option_expiry_date"});
        far_future_quote_id = portfolio_detail::optional_string(details, {"far_future_quote_id", "far_quote_id"});
        near_future_quote_id = portfolio_detail::optional_string(details, {"near_future_quote_id", "near_quote_id"});
        option_type = portfolio_detail::optional_string(details, {"option_type", "call_put"}, option_type);
        strike_spread = portfolio_detail::optional_double(details, {"strike_spread", "strike"});
        underlier = portfolio_detail::optional_string(details, {"underlier", "commodity", "benchmark"});
        unit = portfolio_detail::optional_string(details, {"unit"});
        volatility = portfolio_detail::optional_double(details, {"volatility"});
        volatility_quote_id = portfolio_detail::optional_string(details, {"volatility_quote_id", "vol_quote_id"});
    }
};

/**
 * @brief Simplified commodity swing contract with an exercise volume envelope.
 */
struct CommoditySwingTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for commodity swing contracts.
     */
    CommoditySwingTrade() {
        trade_type = TradeType::CommoditySwing;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double max_quantity = 0.0;                  // Maximum exercisable quantity over the delivery window.
    double min_quantity = 0.0;                  // Minimum committed quantity over the delivery window.
    double strike_price = 0.0;                  // Fixed exercise price.
    double volatility = 0.0;                    // Fallback volatility used for optionality uplift.
    std::string maturity_date;                  // Delivery window end date.
    std::string start_date;                     // Delivery window start date.
    std::string underlier;                      // Commodity name, hub, grade, or benchmark.
    std::string unit;                           // Unit label such as bbl, MWh, or MMBtu.
    std::vector<std::string> exercise_dates;    // Optional exercisable dates inside the window.
    std::vector<std::string> forward_quote_ids; // Forward quotes used to value exercise dates.

    /**
     * @brief Reads commodity swing economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        if (j.contains("start_date")) {
            j.at("start_date").get_to(start_date);
        }
        if (j.contains("maturity_date")) {
            j.at("maturity_date").get_to(maturity_date);
        }
        const auto& details = j.at("details");
        exercise_dates = portfolio_detail::optional_string_vector(details, {"exercise_dates"});
        forward_quote_ids = portfolio_detail::optional_string_vector(details, {"forward_quote_ids", "quote_ids"});
        max_quantity = portfolio_detail::optional_double(details, {"max_quantity", "quantity", "notional"});
        min_quantity = portfolio_detail::optional_double(details, {"min_quantity"});
        maturity_date = portfolio_detail::optional_string(details, {"maturity_date", "delivery_end"}, maturity_date);
        start_date = portfolio_detail::optional_string(details, {"start_date", "delivery_start"}, start_date);
        strike_price = portfolio_detail::optional_double(details, {"strike_price", "strike"});
        underlier = portfolio_detail::optional_string(details, {"underlier", "commodity", "benchmark"});
        unit = portfolio_detail::optional_string(details, {"unit"});
        volatility = portfolio_detail::optional_double(details, {"volatility"});
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

    double quantity = 0.0;        // Number of shares or index units.
    double reference_price = 0.0; // Trade entry spot price.
    std::string underlier;        // Equity ticker or index identifier.

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
 * @brief Equity forward exposure using spot, funding, dividend, and borrow inputs.
 */
struct EquityForwardTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for equity forwards.
     */
    EquityForwardTrade() {
        trade_type = TradeType::EquityForward;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double borrow_rate = 0.0;            // Fallback borrow rate when no quote id is supplied.
    double dividend_yield = 0.0;         // Fallback continuous dividend yield.
    double forward_price = 0.0;          // Contract forward price.
    double quantity = 0.0;               // Number of shares or index units.
    std::string borrow_rate_quote_id;    // Optional borrow-rate quote id.
    std::string dividend_yield_quote_id; // Optional dividend-yield quote id.
    std::string maturity_date;           // Forward settlement date.
    std::string spot_quote_id;           // Optional explicit equity spot quote id.
    std::string underlier;               // Equity ticker or index identifier.

    /**
     * @brief Reads equity forward economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        if (j.contains("quantity")) {
            j.at("quantity").get_to(quantity);
        }
        if (j.contains("maturity_date")) {
            j.at("maturity_date").get_to(maturity_date);
        }

        const auto& details = j.at("details");
        borrow_rate = portfolio_detail::optional_double(details, {"borrow_rate"});
        borrow_rate_quote_id = portfolio_detail::optional_string(details, {"borrow_rate_quote_id", "borrow_quote_id"});
        dividend_yield = portfolio_detail::optional_double(details, {"dividend_yield", "dividend_rate"});
        dividend_yield_quote_id =
            portfolio_detail::optional_string(details, {"dividend_yield_quote_id", "dividend_quote_id"});
        forward_price = portfolio_detail::optional_double(details, {"forward_price", "contract_price", "price"});
        maturity_date = portfolio_detail::optional_string(details, {"maturity_date", "settlement_date"}, maturity_date);
        quantity = quantity != 0.0 ? quantity : portfolio_detail::optional_double(details, {"quantity", "notional"});
        spot_quote_id = portfolio_detail::optional_string(details, {"spot_quote_id", "quote_id"});
        underlier = portfolio_detail::optional_string(details, {"underlier", "ticker", "index"});
    }
};

/**
 * @brief Listed equity or index future exposure.
 */
struct EquityFutureTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for equity futures.
     */
    EquityFutureTrade() {
        trade_type = TradeType::EquityFuture;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double borrow_rate = 0.0;            // Fallback borrow rate when no quote id is supplied.
    double contract_size = 1.0;          // Index multiplier or shares represented by one contract.
    double dividend_yield = 0.0;         // Fallback continuous dividend yield.
    double quantity = 0.0;               // Number of futures contracts.
    double reference_price = 0.0;        // Trade entry futures price.
    std::string borrow_rate_quote_id;    // Optional borrow-rate quote id.
    std::string dividend_yield_quote_id; // Optional dividend-yield quote id.
    std::string future_quote_id;         // Optional listed futures quote id.
    std::string maturity_date;           // Future expiry or settlement date.
    std::string spot_quote_id;           // Optional spot quote fallback id.
    std::string underlier;               // Equity ticker or index identifier.

    /**
     * @brief Reads equity future economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        if (j.contains("quantity")) {
            j.at("quantity").get_to(quantity);
        }
        if (j.contains("maturity_date")) {
            j.at("maturity_date").get_to(maturity_date);
        }

        const auto& details = j.at("details");
        borrow_rate = portfolio_detail::optional_double(details, {"borrow_rate"});
        borrow_rate_quote_id = portfolio_detail::optional_string(details, {"borrow_rate_quote_id", "borrow_quote_id"});
        contract_size = portfolio_detail::optional_double(details, {"contract_size"}, contract_size);
        dividend_yield = portfolio_detail::optional_double(details, {"dividend_yield", "dividend_rate"});
        dividend_yield_quote_id =
            portfolio_detail::optional_string(details, {"dividend_yield_quote_id", "dividend_quote_id"});
        future_quote_id = portfolio_detail::optional_string(details, {"future_quote_id", "quote_id"});
        maturity_date = portfolio_detail::optional_string(details,
                                                          {"maturity_date", "expiry_date", "settlement_date"},
                                                          maturity_date);
        quantity = quantity != 0.0 ? quantity
                                   : portfolio_detail::optional_double(details, {"quantity", "contracts", "notional"});
        reference_price = portfolio_detail::optional_double(details, {"reference_price", "trade_price", "price"});
        spot_quote_id = portfolio_detail::optional_string(details, {"spot_quote_id"});
        underlier = portfolio_detail::optional_string(details, {"underlier", "ticker", "index"});
    }
};

/**
 * @brief European or American option on an equity or equity index.
 */
struct EquityOptionTrade : public Trade {
    /**
     * @brief Initializes taxonomy fields for equity options.
     */
    EquityOptionTrade() {
        trade_type = TradeType::EquityOption;
        product_type = product_type_from_trade_type(trade_type);
        type = to_string(trade_type);
    }

    double borrow_rate = 0.0;                // Fallback borrow rate when no quote id is supplied.
    double dividend_yield = 0.0;             // Fallback continuous dividend yield.
    double quantity = 0.0;                   // Number of option units.
    double strike_price = 0.0;               // Option strike.
    double volatility = 0.0;                 // Fallback volatility when no quote id is supplied.
    std::string borrow_rate_quote_id;        // Optional borrow-rate quote id.
    std::string dividend_yield_quote_id;     // Optional dividend-yield quote id.
    std::string exercise_style = "european"; // European or American exercise.
    std::string expiry_date;                 // Option expiry date.
    std::string option_type = "call";        // Call or put option type.
    std::string settlement_date;             // Settlement date used for discounting.
    std::string spot_quote_id;               // Optional explicit equity spot quote id.
    std::string underlier;                   // Equity ticker or index identifier.
    std::string volatility_quote_id;         // Optional equity volatility quote id.

    /**
     * @brief Reads equity option economics and shared trade fields from JSON.
     */
    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        if (j.contains("quantity")) {
            j.at("quantity").get_to(quantity);
        }

        const auto& details = j.at("details");
        borrow_rate = portfolio_detail::optional_double(details, {"borrow_rate"});
        borrow_rate_quote_id = portfolio_detail::optional_string(details, {"borrow_rate_quote_id", "borrow_quote_id"});
        dividend_yield = portfolio_detail::optional_double(details, {"dividend_yield", "dividend_rate"});
        dividend_yield_quote_id =
            portfolio_detail::optional_string(details, {"dividend_yield_quote_id", "dividend_quote_id"});
        exercise_style = portfolio_detail::optional_string(details, {"exercise_style"}, exercise_style);
        expiry_date = portfolio_detail::optional_string(details, {"expiry_date", "option_expiry_date"});
        option_type = portfolio_detail::optional_string(details, {"option_type", "call_put"}, option_type);
        quantity = quantity != 0.0 ? quantity : portfolio_detail::optional_double(details, {"quantity", "notional"});
        settlement_date = portfolio_detail::optional_string(details, {"settlement_date", "maturity_date"}, expiry_date);
        spot_quote_id = portfolio_detail::optional_string(details, {"spot_quote_id", "quote_id"});
        strike_price = portfolio_detail::optional_double(details, {"strike_price", "strike"});
        underlier = portfolio_detail::optional_string(details, {"underlier", "ticker", "index"});
        volatility = portfolio_detail::optional_double(details, {"volatility"});
        volatility_quote_id = portfolio_detail::optional_string(details, {"volatility_quote_id", "vol_quote_id"});
    }
};

/**
 * @brief Portfolio DTO containing a stable id and owned trade collection.
 */
struct Portfolio {
    std::string portfolio_id;                   // Stable portfolio identifier.
    std::vector<std::shared_ptr<Trade>> trades; // Owned trade DTOs in portfolio order.
};

/**
 * @brief Constructs the concrete trade DTO for a parsed trade type.
 */
inline std::shared_ptr<Trade> make_trade(TradeType type) {
    switch (type) {
        case TradeType::CommoditySpot:
            return std::make_shared<CommoditySpotTrade>();
        case TradeType::CommodityForward:
            return std::make_shared<CommodityForwardTrade>();
        case TradeType::CommodityFuture:
            return std::make_shared<CommodityFutureTrade>();
        case TradeType::CommodityFutureStrip:
            return std::make_shared<CommodityFutureStripTrade>();
        case TradeType::CommodityFutureOption:
            return std::make_shared<CommodityFutureOptionTrade>();
        case TradeType::CommodityCalendarSpreadOption:
            return std::make_shared<CommodityCalendarSpreadOptionTrade>();
        case TradeType::CommoditySwing:
            return std::make_shared<CommoditySwingTrade>();
        case TradeType::CreditBond:
            return std::make_shared<CreditBondTrade>();
        case TradeType::Cds:
            return std::make_shared<CdsTrade>();
        case TradeType::CdsIndex:
            return std::make_shared<CdsIndexTrade>();
        case TradeType::CdsOption:
            return std::make_shared<CdsOptionTrade>();
        case TradeType::CreditIndexOption:
            return std::make_shared<CreditIndexOptionTrade>();
        case TradeType::EquitySpot:
            return std::make_shared<EquitySpotTrade>();
        case TradeType::EquityForward:
            return std::make_shared<EquityForwardTrade>();
        case TradeType::EquityFuture:
            return std::make_shared<EquityFutureTrade>();
        case TradeType::EquityOption:
            return std::make_shared<EquityOptionTrade>();
        case TradeType::FxSpot:
            return std::make_shared<FxSpotTrade>();
        case TradeType::FxForward:
            return std::make_shared<FxForwardTrade>();
        case TradeType::FxSwap:
            return std::make_shared<FxSwapTrade>();
        case TradeType::Ndf:
            return std::make_shared<NdfTrade>();
        case TradeType::FxOption:
            return std::make_shared<FxOptionTrade>();
        case TradeType::Deposit:
            return std::make_shared<DepositTrade>();
        case TradeType::Fra:
            return std::make_shared<FraTrade>();
        case TradeType::InterestRateFuture:
            return std::make_shared<InterestRateFutureTrade>();
        case TradeType::VanillaSwap:
            return std::make_shared<VanillaSwapTrade>();
        case TradeType::OisSwap:
            return std::make_shared<OisSwapTrade>();
        case TradeType::FixedRateBond:
            return std::make_shared<FixedRateBondTrade>();
        case TradeType::CallableBond:
            return std::make_shared<CallableBondTrade>();
        case TradeType::FloatingRateNote:
            return std::make_shared<FloatingRateNoteTrade>();
        case TradeType::CapFloor:
            return std::make_shared<CapFloorTrade>();
        case TradeType::EuropeanSwaption:
            return std::make_shared<EuropeanSwaptionTrade>();
        case TradeType::BermudanSwaption:
            return std::make_shared<BermudanSwaptionTrade>();
        case TradeType::Unknown:
            break;
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
