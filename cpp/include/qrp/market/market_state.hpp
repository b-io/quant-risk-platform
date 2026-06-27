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
#include <utility>

namespace qrp::market {

/**
 * @brief Owns mutable quote handles, built curves, and fixings for one valuation date.
 *
 * MarketState owns QuantLib::SimpleQuote handles so shocks can update market values in place.
 * Curves and instruments observe these handles, enabling lazy revaluation for risk and Monte Carlo.
 */
class MarketState {
public:
    /**
     * @brief Creates an empty market state for a valuation date.
     */
    MarketState(const QuantLib::Date& valuation_date) : valuation_date_(valuation_date) {}

    /**
     * @brief Returns the valuation date for this state.
     */
    const QuantLib::Date& valuation_date() const { return valuation_date_; }

    /**
     * @brief Adds or replaces a yield curve by curve id.
     */
    void add_curve(const domain::CurveId& id, QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> curve) {
        curves_[id] = std::move(curve);
    }

    /**
     * @brief Returns a yield curve by id, or nullptr when absent.
     */
    QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> get_curve(const domain::CurveId& id) const {
        auto it = curves_.find(id);
        if (it != curves_.end()) return it->second;
        return nullptr;
    }

    /**
     * @brief Adds or replaces a raw QuantLib quote handle.
     */
    void add_quote_handle(const std::string& id, QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> quote) {
        quote_handles_[id] = std::move(quote);
        auto& metadata = quote_metadata_[id];
        if (metadata.id.empty()) {
            metadata.id = id;
        }
        metadata.value = quote_handles_[id]->value();
    }

    /**
     * @brief Returns a mutable quote handle by id, or nullptr when absent.
     */
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> get_quote_handle(const std::string& id) const {
        auto it = quote_handles_.find(id);
        if (it != quote_handles_.end()) return it->second;
        return nullptr;
    }

    /**
     * @brief Adds a quote value or updates the existing handle in place.
     */
    void add_quote(const std::string& id, double value) {
        if (quote_handles_.contains(id)) {
            quote_handles_[id]->setValue(value);
        } else {
            add_quote_handle(id, QuantLib::ext::make_shared<QuantLib::SimpleQuote>(value));
        }

        auto& metadata = quote_metadata_[id];
        if (metadata.id.empty()) {
            metadata.id = id;
        }
        metadata.value = value;
    }

    /**
     * @brief Adds a normalized market quote with metadata.
     */
    void add_quote(const domain::MarketQuote& quote) {
        quote_metadata_[quote.id] = quote;
        add_quote(quote.id, quote.value);
    }

    /**
     * @brief Returns a quote value by id, or zero when absent.
     */
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
            add_quote(q);
        }
    }

    /**
     * @brief Captures a lightweight snapshot of the current state.
     * Rationale: Useful for saving a baseline (like frozen-aged) to reset to
     * during Monte Carlo paths.
     */
    domain::MarketSnapshot capture_snapshot() const;

    /**
     * @brief Adds or replaces a historical fixing.
     */
    void add_fixing(const std::string& index_name, const QuantLib::Date& date, double value) {
        fixings_[index_name][date] = value;
    }

    /**
     * @brief Returns a fixing value, or zero when absent.
     */
    double get_fixing(const std::string& index_name, const QuantLib::Date& date) const {
        if (fixings_.contains(index_name) && fixings_.at(index_name).contains(date)) {
            return fixings_.at(index_name).at(date);
        }
        return 0.0;
    }

    /**
     * @brief Returns all fixings keyed by index and fixing date.
     */
    const std::map<std::string, std::map<QuantLib::Date, double>>& fixings() const { return fixings_; }

private:
    QuantLib::Date valuation_date_;
    std::map<domain::CurveId, QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure>> curves_;
    std::map<std::string, QuantLib::ext::shared_ptr<QuantLib::SimpleQuote>> quote_handles_;
    std::map<std::string, domain::MarketQuote> quote_metadata_;
    std::map<std::string, std::map<QuantLib::Date, double>> fixings_;
};

} // namespace qrp::market
