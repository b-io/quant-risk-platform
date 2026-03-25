#pragma once
#include <qrp/domain/types.hpp>
#include <ql/time/date.hpp>
#include <ql/termstructures/yieldtermstructure.hpp>
#include <ql/handle.hpp>
#include <ql/quotes/simplequote.hpp>
#include <map>
#include <memory>
#include <string>

namespace qrp::market {

/*
Design note (see docs/design/ANALYTICS_SERVICES.md):
- MarketState owns SimpleQuote handles so we can apply shocks by setting values on quotes.
- QuantLib curves and instruments observe these handles (Observer pattern); updating a quote triggers
  a lazy revaluation without rebuilding curves or instruments, which is key for PV01/key-rate and Monte Carlo.
*/
class MarketState {
public:
    MarketState(const QuantLib::Date& valuation_date) : valuation_date_(valuation_date) {}

    const QuantLib::Date& valuation_date() const { return valuation_date_; }

    void add_curve(const domain::CurveId& id, std::shared_ptr<QuantLib::YieldTermStructure> curve) {
        curves_[id] = std::move(curve);
    }

    std::shared_ptr<QuantLib::YieldTermStructure> get_curve(const domain::CurveId& id) const {
        auto it = curves_.find(id);
        if (it != curves_.end()) return it->second;
        return nullptr;
    }

    void add_quote_handle(const std::string& id, QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> quote) {
        quote_handles_[id] = std::move(quote);
    }

    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> get_quote_handle(const std::string& id) const {
        auto it = quote_handles_.find(id);
        if (it != quote_handles_.end()) return it->second;
        return nullptr;
    }

    void add_quote(const std::string& id, double value) {
        if (quote_handles_.contains(id)) {
            quote_handles_[id]->setValue(value);
        } else {
            add_quote_handle(id, QuantLib::ext::make_shared<QuantLib::SimpleQuote>(value));
        }
    }

    double get_quote(const std::string& id) const {
        if (quote_handles_.contains(id)) return quote_handles_.at(id)->value();
        return 0.0;
    }

private:
    QuantLib::Date valuation_date_;
    std::map<domain::CurveId, std::shared_ptr<QuantLib::YieldTermStructure>> curves_;
    std::map<std::string, QuantLib::ext::shared_ptr<QuantLib::SimpleQuote>> quote_handles_;
};

} // namespace qrp::market
