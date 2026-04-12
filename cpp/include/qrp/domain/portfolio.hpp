#pragma once
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

namespace qrp::domain {

struct Trade {
    std::string id;
    std::string asset_class;
    std::string type;
    std::string currency;
    std::string direction;
    std::string book;
    std::string strategy;

    virtual ~Trade() = default;

    virtual void from_json(const nlohmann::json& j) {
        j.at("id").get_to(id);
        j.at("asset_class").get_to(asset_class);
        j.at("type").get_to(type);
        j.at("currency").get_to(currency);
        j.at("direction").get_to(direction);
        j.at("book").get_to(book);
        j.at("strategy").get_to(strategy);
    }
};

struct VanillaSwapTrade : public Trade {
    double notional;
    std::string start_date;
    std::string maturity_date;
    double fixed_rate;
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
    double notional;
    std::string start_date;
    std::string maturity_date;
    double coupon_rate;
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
    double quantity;
    double reference_price;
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
    double notional;
    std::string start_date;
    std::string maturity_date;
    std::string base_currency;
    std::string quote_currency;
    double forward_rate;

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

inline void from_json(const nlohmann::json& j, Portfolio& p) {
    j.at("portfolio_id").get_to(p.portfolio_id);
    p.trades.clear();
    for (const auto& tj : j.at("trades")) {
        std::string type = tj.at("type").get<std::string>();
        std::shared_ptr<Trade> trade;
        if (type == "vanilla_swap") {
            trade = std::make_shared<VanillaSwapTrade>();
        } else if (type == "fixed_rate_bond") {
            trade = std::make_shared<FixedRateBondTrade>();
        } else if (type == "equity_spot") {
            trade = std::make_shared<EquitySpotTrade>();
        } else if (type == "fx_forward") {
            trade = std::make_shared<FxForwardTrade>();
        } else {
            throw std::runtime_error("Unknown trade type: " + type);
        }
        trade->from_json(tj);
        p.trades.push_back(trade);
    }
}

} // namespace qrp::domain
