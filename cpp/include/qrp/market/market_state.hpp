#pragma once
#include <qrp/domain/market_data.hpp>
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

    void add_curve(const domain::CurveId& id, QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> curve) {
        curves_[id] = std::move(curve);
    }

    QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> get_curve(const domain::CurveId& id) const {
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

    /**
     * @brief Resets all quote handles to values provided in the snapshot.
     * This ensures the state is consistent with a known reference point.
     * @param snapshot The market snapshot containing base quote values.
     */
    void reset_to_snapshot(const domain::MarketSnapshot& snapshot) {
        for (const auto& q : snapshot.quotes) {
            add_quote(q.id, q.value);
        }
    }

    /**
     * @brief Captures a lightweight snapshot of the current quote values.
     * Rationale: Useful for saving a baseline (like frozen-aged) to reset to
     * during Monte Carlo paths.
     */
    domain::MarketSnapshot capture_snapshot() const {
        domain::MarketSnapshot snapshot;
        // In a real system, we'd preserve valuation_date and other metadata.
        // For MC reset, we primarily need the quotes.
        for (const auto& [id, handle] : quote_handles_) {
            domain::MarketQuote q;
            q.id = id;
            q.value = handle->value();
            snapshot.quotes.push_back(q);
        }
        return snapshot;
    }

    void add_fixing(const std::string& index_name, const QuantLib::Date& date, double value) {
        fixings_[index_name][date] = value;
    }

    double get_fixing(const std::string& index_name, const QuantLib::Date& date) const {
        if (fixings_.contains(index_name) && fixings_.at(index_name).contains(date)) {
            return fixings_.at(index_name).at(date);
        }
        return 0.0;
    }

    const std::map<std::string, std::map<QuantLib::Date, double>>& fixings() const { return fixings_; }

private:
    QuantLib::Date valuation_date_;
    std::map<domain::CurveId, QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure>> curves_;
    std::map<std::string, QuantLib::ext::shared_ptr<QuantLib::SimpleQuote>> quote_handles_;
    std::map<std::string, std::map<QuantLib::Date, double>> fixings_;
};

} // namespace qrp::market
