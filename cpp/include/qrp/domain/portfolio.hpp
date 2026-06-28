#pragma once

// Defines canonical trade and portfolio DTOs used by persistence, JSON, analytics, and bindings.

#include <qrp/domain/product.hpp>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace qrp::domain {

/**
 * @brief Trade implementation types currently supported by the canonical portfolio DTO.
 */
enum class TradeType {
    EquitySpot,
    FixedRateBond,
    FxForward,
    VanillaSwap,
    Unknown
};

/**
 * @brief Parses a canonical trade type string from JSON or storage.
 */
inline TradeType parse_trade_type(const std::string& value) {
    if (value == "equity_spot") return TradeType::EquitySpot;
    if (value == "fixed_rate_bond") return TradeType::FixedRateBond;
    if (value == "fx_forward") return TradeType::FxForward;
    if (value == "vanilla_swap") return TradeType::VanillaSwap;
    throw std::runtime_error("Unknown trade type: " + value);
}

/**
 * @brief Converts a trade type to its canonical snake_case string.
 */
inline std::string to_string(TradeType type) {
    switch (type) {
        case TradeType::EquitySpot: return "equity_spot";
        case TradeType::FixedRateBond: return "fixed_rate_bond";
        case TradeType::FxForward: return "fx_forward";
        case TradeType::VanillaSwap: return "vanilla_swap";
        case TradeType::Unknown: return "unknown";
    }
    return "unknown";
}

/**
 * @brief Maps a concrete trade type to the broader product taxonomy.
 */
inline ProductType product_type_from_trade_type(TradeType type) {
    switch (type) {
        case TradeType::EquitySpot: return ProductType::EquitySpot;
        case TradeType::FixedRateBond: return ProductType::FixedRateBond;
        case TradeType::FxForward: return ProductType::FxForward;
        case TradeType::VanillaSwap: return ProductType::VanillaSwap;
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
 * @brief FX forward exposure trade DTO.
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
        case TradeType::EquitySpot: return std::make_shared<EquitySpotTrade>();
        case TradeType::FixedRateBond: return std::make_shared<FixedRateBondTrade>();
        case TradeType::FxForward: return std::make_shared<FxForwardTrade>();
        case TradeType::VanillaSwap: return std::make_shared<VanillaSwapTrade>();
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
