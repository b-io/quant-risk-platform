#pragma once

// Shared internal implementation helpers for asset-class instrument factories.
// Production code should include the public instrument_factory.hpp facade rather than these helpers directly.

#include <qrp/analytics/pricing_context.hpp>
#include <qrp/conventions/market_convention_registry.hpp>
#include <qrp/market/market_snapshot.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <ql/cashflows/couponpricer.hpp>
#include <ql/cashflows/iborcoupon.hpp>
#include <ql/exercise.hpp>
#include <ql/instruments/bonds/fixedratebond.hpp>
#include <ql/instruments/bonds/floatingratebond.hpp>
#include <ql/instruments/capfloor.hpp>
#include <ql/instruments/forwardrateagreement.hpp>
#include <ql/instruments/overnightindexedswap.hpp>
#include <ql/instruments/stock.hpp>
#include <ql/instruments/swaption.hpp>
#include <ql/instruments/vanillaswap.hpp>
#include <ql/pricingengines/bond/discountingbondengine.hpp>
#include <ql/pricingengines/capfloor/blackcapfloorengine.hpp>
#include <ql/pricingengines/swap/discountingswapengine.hpp>
#include <ql/pricingengines/swaption/blackswaptionengine.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/settings.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace qrp::instruments {

/**
 * @brief Returns a lower-case copy for direction and type normalization.
 */
inline std::string lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

/**
 * @brief Maps a trade-level floating index label to the platform curve family.
 */
inline std::string normalize_index_family(const std::string& index_name) {
    const auto lowered = lower_copy(index_name);
    if (lowered.empty()) {
        return "IBOR_3M";
    }
    if (lowered.find("ois") != std::string::npos || lowered.find("sofr") != std::string::npos ||
        lowered.find("estr") != std::string::npos || lowered.find("sonia") != std::string::npos ||
        lowered.find("saron") != std::string::npos) {
        return "OIS";
    }
    if (lowered.find("6m") != std::string::npos) {
        return "IBOR_6M";
    }
    if (lowered.find("1m") != std::string::npos) {
        return "IBOR_1M";
    }
    return "IBOR_3M";
}

/**
 * @brief Parses coupon or reset frequency text, returning the supplied default on unknown values.
 */
inline domain::Frequency parse_frequency_string(const std::string& value, domain::Frequency fallback) {
    const auto lowered = lower_copy(value);
    if (lowered == "annual" || lowered == "yearly")
        return domain::Frequency::Annual;
    if (lowered == "monthly")
        return domain::Frequency::Monthly;
    if (lowered == "once")
        return domain::Frequency::Once;
    if (lowered == "quarterly")
        return domain::Frequency::Quarterly;
    if (lowered == "semiannual" || lowered == "semi_annually" || lowered == "semi-annual") {
        return domain::Frequency::Semiannual;
    }
    return fallback;
}

/**
 * @brief Converts deposit direction text into a signed lend/borrow exposure.
 */
inline double deposit_direction_sign(const std::string& direction) {
    const auto lowered = lower_copy(direction);
    if (lowered == "borrow" || lowered == "borrower" || lowered == "short") {
        return -1.0;
    }
    return 1.0;
}

/**
 * @brief Converts FRA direction text into QuantLib long/short position semantics.
 */
inline QuantLib::Position::Type fra_position_type(const std::string& direction) {
    const auto lowered = lower_copy(direction);
    if (lowered == "short" || lowered == "sell" || lowered == "lend" || lowered == "lender") {
        return QuantLib::Position::Short;
    }
    return QuantLib::Position::Long;
}

/**
 * @brief Converts fixed-leg swap direction text into QuantLib payer/receiver semantics.
 */
inline QuantLib::VanillaSwap::Type swap_type_from_direction(const std::string& direction) {
    const auto lowered = lower_copy(direction);
    if (lowered == "pay_fixed" || lowered == "payer" || lowered == "pay") {
        return QuantLib::VanillaSwap::Payer;
    }
    return QuantLib::VanillaSwap::Receiver;
}

/**
 * @brief Returns the signed FX base-currency exposure implied by the trade direction.
 */
inline double fx_direction_sign(const std::string& direction) {
    const auto lowered = lower_copy(direction);
    if (lowered == "pay_base" || lowered == "sell" || lowered == "sell_base" || lowered == "short") {
        return -1.0;
    }
    return 1.0;
}

/**
 * @brief Checks whether an FX option type is a put on the base currency.
 */
inline bool is_put_option(const std::string& option_type) {
    const auto lowered = lower_copy(option_type);
    return lowered == "put" || lowered == "put_base";
}

/**
 * @brief Returns the signed credit bond quantity implied by buy/sell direction.
 */
inline double credit_bond_direction_sign(const std::string& direction) {
    const auto lowered = lower_copy(direction);
    if (lowered == "short" || lowered == "sell") {
        return -1.0;
    }
    return 1.0;
}

/**
 * @brief Returns the signed premium/exposure direction for credit spread options.
 */
inline double credit_option_direction_sign(const std::string& direction) {
    const auto lowered = lower_copy(direction);
    if (lowered == "short" || lowered == "sell" || lowered == "written") {
        return -1.0;
    }
    return 1.0;
}

/**
 * @brief Converts CDS protection direction into buyer-positive protection exposure.
 */
inline double credit_protection_direction_sign(const std::string& direction) {
    const auto lowered = lower_copy(direction);
    if (lowered == "sell" || lowered == "sell_protection" || lowered == "seller" || lowered == "short" ||
        lowered == "short_protection") {
        return -1.0;
    }
    return 1.0;
}

/**
 * @brief Checks whether a credit option pays when spreads tighten.
 */
inline bool is_credit_put_option(const std::string& option_type) {
    const auto lowered = lower_copy(option_type);
    return lowered == "put" || lowered == "receiver";
}

/**
 * @brief Converts generic long/short option direction into a position sign.
 */
inline double long_short_direction_sign(const std::string& direction) {
    const auto lowered = lower_copy(direction);
    if (lowered == "short" || lowered == "sell" || lowered == "sold" || lowered == "written") {
        return -1.0;
    }
    return 1.0;
}

/**
 * @brief Checks whether an option exercise-style label requests American exercise.
 */
inline bool is_american_exercise(const std::string& exercise_style) {
    return lower_copy(exercise_style) == "american";
}

/**
 * @brief Builds a canonical six-letter FX pair from base and quote currency labels.
 */
inline std::string fx_pair(const std::string& base_currency, const std::string& quote_currency) {
    return base_currency + quote_currency;
}

/**
 * @brief Extracts an IBOR tenor such as 3M or 6M from an index-family label.
 */
inline QuantLib::Period index_tenor_from_family(const std::string& index_family) {
    const auto separator = index_family.find('_');
    if (separator != std::string::npos && separator + 1 < index_family.size()) {
        try {
            return market::CurveBuilder::parse_tenor(index_family.substr(separator + 1));
        } catch (const std::invalid_argument&) {
            return QuantLib::Period(3, QuantLib::Months);
        }
    }
    return QuantLib::Period(3, QuantLib::Months);
}

/**
 * @brief Resolves the registered rates convention for a currency and index family.
 */
inline conventions::RatesConvention rates_convention(domain::Currency currency, const std::string& index_family) {
    return conventions::MarketConventionRegistry::instance().get_rates_convention(currency, index_family);
}

/**
 * @brief Fetches the discount curve selected by the pricing context.
 */
inline QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve(const analytics::PricingContext& context,
                                                                              domain::Currency currency) {
    return context.market_state().get_curve(context.get_discount_curve_id(currency));
}

/**
 * @brief Fetches the forecast curve, using the discount curve when no forecast curve is configured.
 */
inline QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure>
forecast_curve(const analytics::PricingContext& context, domain::Currency currency, const std::string& index_family) {
    auto curve = context.market_state().get_curve(context.get_forecast_curve_id(currency, index_family));
    return curve ? curve : discount_curve(context, currency);
}

/**
 * @brief Adds historical IBOR fixings for any of the supplied index aliases.
 */
inline void apply_ibor_fixings(const QuantLib::ext::shared_ptr<QuantLib::IborIndex>& index,
                               const analytics::PricingContext& context,
                               const std::vector<std::string>& aliases) {
    for (const auto& [index_name, date_fixings] : context.market_state().fixings()) {
        if (std::find(aliases.begin(), aliases.end(), index_name) == aliases.end()) {
            continue;
        }
        for (const auto& [date, value] : date_fixings) {
            index->addFixing(date, value, true);
        }
    }
}

/**
 * @brief Adds historical overnight fixings for any of the supplied index aliases.
 */
inline void apply_overnight_fixings(const QuantLib::ext::shared_ptr<QuantLib::OvernightIndex>& index,
                                    const analytics::PricingContext& context,
                                    const std::vector<std::string>& aliases) {
    for (const auto& [index_name, date_fixings] : context.market_state().fixings()) {
        if (std::find(aliases.begin(), aliases.end(), index_name) == aliases.end()) {
            continue;
        }
        for (const auto& [date, value] : date_fixings) {
            index->addFixing(date, value, true);
        }
    }
}

/**
 * @brief Creates a standard QuantLib schedule with matching accrual and payment adjustment.
 */
inline QuantLib::Schedule make_schedule(const QuantLib::Date& start,
                                        const QuantLib::Date& maturity,
                                        QuantLib::Frequency frequency,
                                        const QuantLib::Calendar& calendar,
                                        QuantLib::BusinessDayConvention bdc,
                                        QuantLib::DateGeneration::Rule rule = QuantLib::DateGeneration::Forward) {
    return QuantLib::Schedule(start, maturity, QuantLib::Period(frequency), calendar, bdc, bdc, rule, false);
}

/**
 * @brief Creates an IBOR index and attaches relevant historical fixings.
 */
inline QuantLib::ext::shared_ptr<QuantLib::IborIndex>
make_ibor_index(domain::Currency currency,
                const std::string& index_family,
                const QuantLib::Handle<QuantLib::YieldTermStructure>& forecast_handle,
                const analytics::PricingContext& context,
                const std::string& trade_alias) {
    auto index =
        market::CurveBuilder::create_ibor_index(currency, index_tenor_from_family(index_family), forecast_handle);
    apply_ibor_fixings(index, context, {index_family, trade_alias, "IBOR_3M"});
    return index;
}

/**
 * @brief Creates an overnight index and attaches relevant historical fixings.
 */
inline QuantLib::ext::shared_ptr<QuantLib::OvernightIndex>
make_overnight_index(domain::Currency currency,
                     const QuantLib::Handle<QuantLib::YieldTermStructure>& forecast_handle,
                     const analytics::PricingContext& context,
                     const std::string& trade_alias) {
    auto index = market::CurveBuilder::create_overnight_index(currency, forecast_handle);
    apply_overnight_fixings(index, context, {"OIS", trade_alias});
    return index;
}

/**
 * @brief Reads volatility from a market quote, then trade input, then a model default.
 */
inline double quote_or_default_volatility(const analytics::PricingContext& context,
                                          const std::string& quote_id,
                                          double trade_volatility,
                                          double fallback) {
    if (!quote_id.empty()) {
        if (auto quote = context.market_state().get_quote_handle(quote_id)) {
            return quote->value();
        }
    }
    return trade_volatility > 0.0 ? trade_volatility : fallback;
}

/**
 * @brief Converts either a futures price or futures-implied rate into a futures price.
 */
inline double futures_price_from_quote(double value) {
    return value > 1.0 ? value : 100.0 * (1.0 - value);
}

/**
 * @brief Standard normal cumulative distribution function.
 */
inline double normal_cdf(double value) {
    return 0.5 * std::erfc(-value / std::sqrt(2.0));
}

/**
 * @brief Standard normal probability density function.
 */
inline double normal_pdf(double value) {
    static constexpr double inv_sqrt_two_pi = 0.3989422804014327;
    return inv_sqrt_two_pi * std::exp(-0.5 * value * value);
}

/**
 * @brief Returns a discount factor from a curve, or one when discounting is unavailable.
 */
inline double discount_factor_or_one(const QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure>& curve,
                                     const QuantLib::Date& date) {
    return curve ? curve->discount(date) : 1.0;
}

/**
 * @brief Credit spread curve node backed by a live market quote handle.
 */
struct CreditSpreadNode {
    /** @brief Node maturity expressed as year fraction from valuation date. */
    double years = 0.0;
    /** @brief Live spread quote handle used for revaluation and scenario shocks. */
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> quote;
};

/**
 * @brief Checks whether a quote instrument type belongs to an accepted type set.
 */
inline bool contains_quote_type(domain::QuoteInstrumentType type,
                                std::initializer_list<domain::QuoteInstrumentType> accepted_types) {
    return std::find(accepted_types.begin(), accepted_types.end(), type) != accepted_types.end();
}

/**
 * @brief Converts tenor text to years, returning the supplied default when absent or invalid.
 */
inline double tenor_years_or_fallback(const std::string& tenor, double fallback_years) {
    if (tenor.empty()) {
        return fallback_years;
    }
    try {
        return std::max(market::CurveBuilder::tenor_to_years(tenor), 1.0e-8);
    } catch (const std::invalid_argument&) {
        return fallback_years;
    }
}

/**
 * @brief Checks whether a credit quote can contribute to an underlier spread curve.
 */
inline bool quote_matches_credit_underlier(const domain::MarketQuote& quote,
                                           const std::string& underlier,
                                           std::initializer_list<domain::QuoteInstrumentType> accepted_types) {
    if (!contains_quote_type(quote.instrument_type, accepted_types)) {
        return false;
    }
    if (quote.quote_type == domain::QuoteType::Volatility ||
        quote.instrument_type == domain::QuoteInstrumentType::RecoveryRate) {
        return false;
    }
    return underlier.empty() || quote.underlier == underlier;
}

/**
 * @brief Checks whether a quote matches underlier, tenor, and instrument-type criteria.
 */
inline bool quote_matches_underlier_and_tenor(const domain::MarketQuote& quote,
                                              const std::string& underlier,
                                              const std::string& tenor,
                                              std::initializer_list<domain::QuoteInstrumentType> accepted_types) {
    if (!contains_quote_type(quote.instrument_type, accepted_types)) {
        return false;
    }
    if (!underlier.empty() && quote.underlier != underlier && quote.id != underlier) {
        return false;
    }
    return tenor.empty() || quote.tenor == tenor || quote.expiry == tenor;
}

/**
 * @brief Finds a live quote handle by explicit id or by underlier/tenor/type metadata.
 */
inline QuantLib::ext::shared_ptr<QuantLib::SimpleQuote>
find_quote_handle(const analytics::PricingContext& context,
                  const std::string& quote_id,
                  const std::string& underlier,
                  const std::string& tenor,
                  std::initializer_list<domain::QuoteInstrumentType> accepted_types) {

    if (!quote_id.empty()) {
        if (auto quote = context.market_state().get_quote_handle(quote_id)) {
            return quote;
        }
    }

    const auto snapshot = context.market_state().capture_snapshot();
    for (const auto& quote : snapshot.quotes) {
        if (quote_matches_underlier_and_tenor(quote, underlier, tenor, accepted_types)) {
            if (auto handle = context.market_state().get_quote_handle(quote.id)) {
                return handle;
            }
        }
    }
    return nullptr;
}

/**
 * @brief Returns a matching market quote handle or a constant quote with the supplied default
 * value.
 */
inline QuantLib::ext::shared_ptr<QuantLib::SimpleQuote>
quote_or_constant(const analytics::PricingContext& context,
                  const std::string& quote_id,
                  const std::string& underlier,
                  std::initializer_list<domain::QuoteInstrumentType> accepted_types,
                  double fallback) {

    auto quote = find_quote_handle(context, quote_id, underlier, {}, accepted_types);
    return quote ? quote : QuantLib::ext::make_shared<QuantLib::SimpleQuote>(fallback);
}

/**
 * @brief Builds a sorted spread curve from primary and underlier-matched credit quotes.
 */
inline std::vector<CreditSpreadNode>
make_credit_spread_curve(const analytics::PricingContext& context,
                         const std::string& underlier,
                         const std::string& primary_quote_id,
                         double fallback_spread,
                         std::initializer_list<domain::QuoteInstrumentType> accepted_types,
                         double fallback_years) {

    std::vector<CreditSpreadNode> nodes;
    std::vector<std::string> added_quote_ids;
    const auto snapshot = context.market_state().capture_snapshot();

    auto add_quote_node = [&](const domain::MarketQuote& quote) {
        if (std::find(added_quote_ids.begin(), added_quote_ids.end(), quote.id) != added_quote_ids.end()) {
            return;
        }
        auto handle = context.market_state().get_quote_handle(quote.id);
        if (!handle) {
            return;
        }
        nodes.push_back(CreditSpreadNode{tenor_years_or_fallback(quote.tenor, fallback_years), handle});
        added_quote_ids.push_back(quote.id);
    };

    if (!primary_quote_id.empty()) {
        for (const auto& quote : snapshot.quotes) {
            if (quote.id == primary_quote_id) {
                add_quote_node(quote);
                break;
            }
        }
    }

    for (const auto& quote : snapshot.quotes) {
        if (quote.id == primary_quote_id) {
            continue;
        }
        if (quote_matches_credit_underlier(quote, underlier, accepted_types)) {
            add_quote_node(quote);
        }
    }

    if (nodes.empty()) {
        nodes.push_back(
            CreditSpreadNode{std::max(fallback_years, 1.0e-8),
                             QuantLib::ext::make_shared<QuantLib::SimpleQuote>(std::max(fallback_spread, 0.0))});
    }

    std::sort(nodes.begin(), nodes.end(), [](const auto& lhs, const auto& rhs) { return lhs.years < rhs.years; });
    return nodes;
}

/**
 * @brief Resolves a live recovery quote for an underlier or creates a constant recovery input.
 */
inline QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> make_recovery_quote(const analytics::PricingContext& context,
                                                                            const std::string& underlier,
                                                                            const std::string& recovery_quote_id,
                                                                            double recovery_rate) {

    if (!recovery_quote_id.empty()) {
        if (auto quote = context.market_state().get_quote_handle(recovery_quote_id)) {
            return quote;
        }
    }

    const auto snapshot = context.market_state().capture_snapshot();
    for (const auto& quote : snapshot.quotes) {
        if (quote.instrument_type == domain::QuoteInstrumentType::RecoveryRate &&
            (underlier.empty() || quote.underlier == underlier)) {
            if (auto handle = context.market_state().get_quote_handle(quote.id)) {
                return handle;
            }
        }
    }

    return QuantLib::ext::make_shared<QuantLib::SimpleQuote>(recovery_rate);
}

/**
 * @brief Clamps recovery to a numerically stable range for hazard-rate calculations.
 */
inline double bounded_recovery(double recovery) {
    return std::min(std::max(recovery, 0.0), 0.95);
}

/**
 * @brief Linearly interpolates the credit spread curve at a maturity in years.
 */
inline double credit_spread_at(const std::vector<CreditSpreadNode>& nodes, double years) {
    if (nodes.empty()) {
        return 0.0;
    }
    if (years <= nodes.front().years) {
        return std::max(nodes.front().quote->value(), 0.0);
    }
    if (years >= nodes.back().years) {
        return std::max(nodes.back().quote->value(), 0.0);
    }

    for (std::size_t i = 1; i < nodes.size(); ++i) {
        if (years <= nodes[i].years) {
            const double left_years = nodes[i - 1].years;
            const double right_years = nodes[i].years;
            const double weight = (years - left_years) / std::max(right_years - left_years, 1.0e-8);
            const double left_spread = nodes[i - 1].quote->value();
            const double right_spread = nodes[i].quote->value();
            return std::max(left_spread + weight * (right_spread - left_spread), 0.0);
        }
    }
    return std::max(nodes.back().quote->value(), 0.0);
}

/**
 * @brief Integrates the interpolated credit spread curve over the maturity horizon.
 */
inline double integrated_credit_spread(const std::vector<CreditSpreadNode>& nodes, double years) {
    if (years <= 0.0) {
        return 0.0;
    }

    constexpr int integration_steps = 24;
    const double dt = years / static_cast<double>(integration_steps);
    double integral = 0.0;
    for (int step = 0; step < integration_steps; ++step) {
        const double midpoint = (static_cast<double>(step) + 0.5) * dt;
        integral += credit_spread_at(nodes, midpoint) * dt;
    }
    return integral;
}

/**
 * @brief Converts integrated credit spread into a spread discount factor.
 */
inline double spread_discount_factor(const std::vector<CreditSpreadNode>& nodes, double years) {
    return std::exp(-integrated_credit_spread(nodes, years));
}

/**
 * @brief Converts integrated spread and recovery into a simple survival probability.
 */
inline double survival_probability(const std::vector<CreditSpreadNode>& nodes, double recovery, double years) {
    const double loss_given_default = std::max(1.0 - bounded_recovery(recovery), 1.0e-8);
    return std::exp(-integrated_credit_spread(nodes, years) / loss_given_default);
}

/**
 * @brief Resolves an FX forward rate from outright quote, points quote, or interest-rate parity.
 */
inline double forward_rate_from_quotes(const QuantLib::ext::shared_ptr<QuantLib::SimpleQuote>& spot_quote,
                                       const QuantLib::ext::shared_ptr<QuantLib::SimpleQuote>& forward_quote,
                                       const QuantLib::ext::shared_ptr<QuantLib::SimpleQuote>& forward_points_quote,
                                       const QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure>& domestic_curve,
                                       const QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure>& foreign_curve,
                                       const QuantLib::Date& maturity_date) {
    if (forward_quote) {
        return forward_quote->value();
    }

    const double spot = spot_quote->value();
    if (forward_points_quote) {
        return spot + forward_points_quote->value();
    }

    const double domestic_df = discount_factor_or_one(domestic_curve, maturity_date);
    const double foreign_df = discount_factor_or_one(foreign_curve, maturity_date);
    return domestic_df > 0.0 ? spot * foreign_df / domestic_df : spot;
}

/**
 * @brief Computes the fixed par rate of a swap over the remaining fixed schedule.
 */
inline double par_swap_rate(const QuantLib::Date& start,
                            const QuantLib::Date& maturity,
                            const QuantLib::Schedule& fixed_schedule,
                            const QuantLib::DayCounter& fixed_day_count,
                            const QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure>& curve) {
    if (!curve) {
        return 0.0;
    }

    double annuity = 0.0;
    const auto& dates = fixed_schedule.dates();
    for (std::size_t i = 1; i < dates.size(); ++i) {
        if (dates[i] <= start) {
            continue;
        }
        const auto accrual = fixed_day_count.yearFraction(dates[i - 1], dates[i]);
        annuity += accrual * curve->discount(dates[i]);
    }
    if (annuity <= 0.0) {
        return curve->forwardRate(start, maturity, fixed_day_count, QuantLib::Simple).rate();
    }
    return (curve->discount(start) - curve->discount(maturity)) / annuity;
}

/**
 * @brief Computes exercise-date swap annuity for Bermudan swaption intrinsic values.
 */
inline double swap_annuity_at_exercise(const QuantLib::Date& exercise_date,
                                       const QuantLib::Schedule& fixed_schedule,
                                       const QuantLib::DayCounter& fixed_day_count,
                                       const QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure>& curve,
                                       double notional) {
    if (!curve) {
        return 0.0;
    }

    const double exercise_df = curve->discount(exercise_date);
    if (exercise_df <= 0.0) {
        return 0.0;
    }

    double annuity = 0.0;
    const auto& dates = fixed_schedule.dates();
    for (std::size_t i = 1; i < dates.size(); ++i) {
        if (dates[i] <= exercise_date) {
            continue;
        }
        const auto accrual = fixed_day_count.yearFraction(dates[i - 1], dates[i]);
        annuity += accrual * curve->discount(dates[i]) / exercise_df;
    }
    return notional * annuity;
}

} // namespace qrp::instruments
