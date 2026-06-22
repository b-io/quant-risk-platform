#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <nlohmann/json.hpp>

namespace qrp::domain {

enum class TradeType {
    Unknown,
    VanillaSwap,
    FixedRateBond,
    EquitySpot,
    FxForward
};

inline TradeType parse_trade_type(const std::string& value) {
    if (value == "vanilla_swap") return TradeType::VanillaSwap;
    if (value == "fixed_rate_bond") return TradeType::FixedRateBond;
    if (value == "equity_spot") return TradeType::EquitySpot;
    if (value == "fx_forward") return TradeType::FxForward;
    throw std::runtime_error("Unknown trade type: " + value);
}

inline std::string to_string(TradeType type) {
    switch (type) {
        case TradeType::VanillaSwap: return "vanilla_swap";
        case TradeType::FixedRateBond: return "fixed_rate_bond";
        case TradeType::EquitySpot: return "equity_spot";
        case TradeType::FxForward: return "fx_forward";
        case TradeType::Unknown: return "unknown";
    }
    return "unknown";
}

struct Trade {
    std::string id;
    std::string asset_class;
    std::string type;
    TradeType trade_type = TradeType::Unknown;
    std::string currency;
    std::string direction;
    std::string book;
    std::string strategy;

    virtual ~Trade() = default;

    virtual void from_json(const nlohmann::json& j) {
        j.at("id").get_to(id);
        j.at("asset_class").get_to(asset_class);
        trade_type = parse_trade_type(j.at("type").get<std::string>());
        type = to_string(trade_type);
        j.at("currency").get_to(currency);
        j.at("direction").get_to(direction);
        j.at("book").get_to(book);
        j.at("strategy").get_to(strategy);
    }
};

struct VanillaSwapTrade : public Trade {
    VanillaSwapTrade() {
        trade_type = TradeType::VanillaSwap;
        type = to_string(trade_type);
    }

    double notional = 0.0;
    std::string start_date;
    std::string maturity_date;
    double fixed_rate = 0.0;
    std::string floating_index;

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

struct FixedRateBondTrade : public Trade {
    FixedRateBondTrade() {
        trade_type = TradeType::FixedRateBond;
        type = to_string(trade_type);
    }

    double notional = 0.0;
    std::string start_date;
    std::string maturity_date;
    double coupon_rate = 0.0;
    std::string frequency;

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

struct EquitySpotTrade : public Trade {
    EquitySpotTrade() {
        trade_type = TradeType::EquitySpot;
        type = to_string(trade_type);
    }

    double quantity = 0.0;
    double reference_price = 0.0;
    std::string underlier;

    void from_json(const nlohmann::json& j) override {
        Trade::from_json(j);
        j.at("quantity").get_to(quantity);

        const auto& details = j.at("details");
        details.at("reference_price").get_to(reference_price);
        details.at("underlier").get_to(underlier);
    }
};

struct FxForwardTrade : public Trade {
    FxForwardTrade() {
        trade_type = TradeType::FxForward;
        type = to_string(trade_type);
    }

    double notional = 0.0;
    std::string start_date;
    std::string maturity_date;
    std::string base_currency;
    std::string quote_currency;
    double forward_rate = 0.0;

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

struct Portfolio {
    std::string portfolio_id;
    std::vector<std::shared_ptr<Trade>> trades;
};

inline std::shared_ptr<Trade> make_trade(TradeType type) {
    switch (type) {
        case TradeType::VanillaSwap: return std::make_shared<VanillaSwapTrade>();
        case TradeType::FixedRateBond: return std::make_shared<FixedRateBondTrade>();
        case TradeType::EquitySpot: return std::make_shared<EquitySpotTrade>();
        case TradeType::FxForward: return std::make_shared<FxForwardTrade>();
        case TradeType::Unknown: break;
    }
    throw std::runtime_error("Cannot construct unknown trade type");
}

inline std::shared_ptr<Trade> make_trade(const std::string& type) {
    return make_trade(parse_trade_type(type));
}

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
