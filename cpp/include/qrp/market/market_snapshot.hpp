#pragma once
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/termstructures/yield/zerocurve.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <ql/time/daycounters/thirty360.hpp>
#include <ql/time/period.hpp>
#include <ql/settings.hpp>
#include <qrp/domain/market_data.hpp>
#include <map>
#include <memory>
#include <string>

namespace qrp::market {

class CurveBuilder {
public:
    static QuantLib::Date parse_date(const std::string& date_str) {
        // Simple YYYY-MM-DD parser
        int y = std::stoi(date_str.substr(0, 4));
        int m = std::stoi(date_str.substr(5, 2));
        int d = std::stoi(date_str.substr(8, 2));
        return QuantLib::Date(d, static_cast<QuantLib::Month>(m), y);
    }

    static QuantLib::Period parse_tenor(const std::string& tenor) {
        if (tenor == "1M") return QuantLib::Period(1, QuantLib::Months);
        if (tenor == "3M") return QuantLib::Period(3, QuantLib::Months);
        if (tenor == "6M") return QuantLib::Period(6, QuantLib::Months);
        if (tenor == "1Y") return QuantLib::Period(1, QuantLib::Years);
        if (tenor == "2Y") return QuantLib::Period(2, QuantLib::Years);
        if (tenor == "5Y") return QuantLib::Period(5, QuantLib::Years);
        if (tenor == "10Y") return QuantLib::Period(10, QuantLib::Years);
        if (tenor == "30Y") return QuantLib::Period(30, QuantLib::Years);
        return QuantLib::Period(0, QuantLib::Days);
    }

    static std::shared_ptr<QuantLib::YieldTermStructure> build_curve(
        const domain::CurveDefinition& def,
        const QuantLib::Date& valuation_date) {

        std::vector<QuantLib::Date> dates;
        std::vector<QuantLib::Rate> yields;
        QuantLib::DayCounter dc = QuantLib::Actual365Fixed();
        QuantLib::Calendar cal = QuantLib::TARGET();

        dates.push_back(valuation_date);
        yields.push_back(def.nodes[0].value); // Assume first node or flat start

        for (const auto& node : def.nodes) {
            dates.push_back(cal.advance(valuation_date, parse_tenor(node.tenor)));
            yields.push_back(node.value);
        }

        auto curve = std::make_shared<QuantLib::ZeroCurve>(dates, yields, dc, cal);
        return curve;
    }
};

class MarketSnapshot {
public:
    explicit MarketSnapshot(const domain::MarketSnapshot& dto) {
        valuation_date_ = CurveBuilder::parse_date(dto.valuation_date);
        QuantLib::Settings::instance().evaluationDate() = valuation_date_;

        if (dto.markets.contains("rates")) {
            for (const auto& [name, def] : dto.markets.at("rates")) {
                curves_[name] = CurveBuilder::build_curve(def, valuation_date_);
            }
        }
    }

    const QuantLib::Date& valuation_date() const { return valuation_date_; }

    std::shared_ptr<QuantLib::YieldTermStructure> get_curve(const std::string& name) const {
        auto it = curves_.find(name);
        if (it != curves_.end()) return it->second;
        return nullptr;
    }

private:
    QuantLib::Date valuation_date_;
    std::map<std::string, std::shared_ptr<QuantLib::YieldTermStructure>> curves_;
};

} // namespace qrp::market
