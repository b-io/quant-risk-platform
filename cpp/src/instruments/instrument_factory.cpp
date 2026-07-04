// Implements translation from domain trades into QuantLib instruments and pricing engines.

#include <qrp/instruments/instrument_factory.hpp>

#include <qrp/analytics/lsmc/lsmc_engine.hpp>
#include <qrp/analytics/product_pricing_registry.hpp>
#include <qrp/analytics/simulation/stochastic_process.hpp>
#include <qrp/conventions/market_convention_registry.hpp>
#include <qrp/market/market_snapshot.hpp>

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

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

/*
Design note (see docs/design/ARCHITECTURE.md and docs/design/CURVE_BOOTSTRAP_DESIGN.md):
- Rates products resolve OIS discount curves and IBOR/overnight forecast curves through PricingContext.
- QuantLib instruments are used where a stable product implementation exists.
- Deposits and exchange-traded short-rate futures are represented as small local QuantLib::Instrument
  subclasses because QuantLib's production support for those products mainly lives in curve helpers.
- Bermudan swaptions are priced by a deterministic, one-factor LSMC approximation over the forward swap rate.
  This is intentionally simple for Phase 3 and keeps the exercise-policy path separate from the later
  production-grade callable/LSMC integration phase.
*/
namespace qrp::instruments {
namespace {

std::string lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string normalize_index_family(const std::string& index_name) {
    const auto lowered = lower_copy(index_name);
    if (lowered.empty()) {
        return "IBOR_3M";
    }
    if (lowered.find("ois") != std::string::npos ||
        lowered.find("sofr") != std::string::npos ||
        lowered.find("estr") != std::string::npos ||
        lowered.find("sonia") != std::string::npos ||
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

domain::Frequency parse_frequency_string(const std::string& value, domain::Frequency fallback) {
    const auto lowered = lower_copy(value);
    if (lowered == "annual" || lowered == "yearly") return domain::Frequency::Annual;
    if (lowered == "monthly") return domain::Frequency::Monthly;
    if (lowered == "once") return domain::Frequency::Once;
    if (lowered == "quarterly") return domain::Frequency::Quarterly;
    if (lowered == "semiannual" || lowered == "semi_annually" || lowered == "semi-annual") {
        return domain::Frequency::Semiannual;
    }
    return fallback;
}

double deposit_direction_sign(const std::string& direction) {
    const auto lowered = lower_copy(direction);
    if (lowered == "borrow" || lowered == "borrower" || lowered == "short") {
        return -1.0;
    }
    return 1.0;
}

QuantLib::Position::Type fra_position_type(const std::string& direction) {
    const auto lowered = lower_copy(direction);
    if (lowered == "short" || lowered == "sell" || lowered == "lend" || lowered == "lender") {
        return QuantLib::Position::Short;
    }
    return QuantLib::Position::Long;
}

QuantLib::VanillaSwap::Type swap_type_from_direction(const std::string& direction) {
    const auto lowered = lower_copy(direction);
    if (lowered == "pay_fixed" || lowered == "payer" || lowered == "pay") {
        return QuantLib::VanillaSwap::Payer;
    }
    return QuantLib::VanillaSwap::Receiver;
}

double fx_direction_sign(const std::string& direction) {
    const auto lowered = lower_copy(direction);
    if (lowered == "pay_base" || lowered == "sell" || lowered == "sell_base" || lowered == "short") {
        return -1.0;
    }
    return 1.0;
}

bool is_put_option(const std::string& option_type) {
    const auto lowered = lower_copy(option_type);
    return lowered == "put" || lowered == "put_base";
}

std::string fx_pair(const std::string& base_currency, const std::string& quote_currency) {
    return base_currency + quote_currency;
}

QuantLib::Period index_tenor_from_family(const std::string& index_family) {
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

conventions::RatesConvention rates_convention(domain::Currency currency, const std::string& index_family) {
    return conventions::MarketConventionRegistry::instance().get_rates_convention(currency, index_family);
}

QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve(
    const analytics::PricingContext& context,
    domain::Currency currency) {
    return context.market_state().get_curve(context.get_discount_curve_id(currency));
}

QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> forecast_curve(
    const analytics::PricingContext& context,
    domain::Currency currency,
    const std::string& index_family) {
    auto curve = context.market_state().get_curve(context.get_forecast_curve_id(currency, index_family));
    return curve ? curve : discount_curve(context, currency);
}

void apply_ibor_fixings(
    const QuantLib::ext::shared_ptr<QuantLib::IborIndex>& index,
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

void apply_overnight_fixings(
    const QuantLib::ext::shared_ptr<QuantLib::OvernightIndex>& index,
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

QuantLib::Schedule make_schedule(
    const QuantLib::Date& start,
    const QuantLib::Date& maturity,
    QuantLib::Frequency frequency,
    const QuantLib::Calendar& calendar,
    QuantLib::BusinessDayConvention bdc,
    QuantLib::DateGeneration::Rule rule = QuantLib::DateGeneration::Forward) {
    return QuantLib::Schedule(start, maturity, QuantLib::Period(frequency), calendar, bdc, bdc, rule, false);
}

QuantLib::ext::shared_ptr<QuantLib::IborIndex> make_ibor_index(
    domain::Currency currency,
    const std::string& index_family,
    const QuantLib::Handle<QuantLib::YieldTermStructure>& forecast_handle,
    const analytics::PricingContext& context,
    const std::string& trade_alias) {
    auto index = market::CurveBuilder::create_ibor_index(currency, index_tenor_from_family(index_family), forecast_handle);
    apply_ibor_fixings(index, context, {index_family, trade_alias, "IBOR_3M"});
    return index;
}

QuantLib::ext::shared_ptr<QuantLib::OvernightIndex> make_overnight_index(
    domain::Currency currency,
    const QuantLib::Handle<QuantLib::YieldTermStructure>& forecast_handle,
    const analytics::PricingContext& context,
    const std::string& trade_alias) {
    auto index = market::CurveBuilder::create_overnight_index(currency, forecast_handle);
    apply_overnight_fixings(index, context, {"OIS", trade_alias});
    return index;
}

double quote_or_default_volatility(
    const analytics::PricingContext& context,
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

double futures_price_from_quote(double value) {
    return value > 1.0 ? value : 100.0 * (1.0 - value);
}

double normal_cdf(double value) {
    return 0.5 * std::erfc(-value / std::sqrt(2.0));
}

double discount_factor_or_one(
    const QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure>& curve,
    const QuantLib::Date& date) {
    return curve ? curve->discount(date) : 1.0;
}

double forward_rate_from_quotes(
    const QuantLib::ext::shared_ptr<QuantLib::SimpleQuote>& spot_quote,
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

double par_swap_rate(
    const QuantLib::Date& start,
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

double swap_annuity_at_exercise(
    const QuantLib::Date& exercise_date,
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

class DepositInstrument final : public QuantLib::Instrument {
public:
    DepositInstrument(
        double notional,
        double deposit_rate,
        double direction_sign,
        QuantLib::Date start_date,
        QuantLib::Date maturity_date,
        QuantLib::DayCounter day_count,
        QuantLib::Handle<QuantLib::YieldTermStructure> discount_handle)
        : notional_(notional),
          deposit_rate_(deposit_rate),
          direction_sign_(direction_sign),
          start_date_(std::move(start_date)),
          maturity_date_(std::move(maturity_date)),
          day_count_(std::move(day_count)),
          discount_handle_(std::move(discount_handle)) {}

    bool isExpired() const override {
        return maturity_date_ <= QuantLib::Settings::instance().evaluationDate();
    }

protected:
    void performCalculations() const override {
        const auto accrual = day_count_.yearFraction(start_date_, maturity_date_);
        const auto start_df = discount_handle_->discount(start_date_);
        const auto maturity_df = discount_handle_->discount(maturity_date_);
        const auto maturity_amount = notional_ * (1.0 + deposit_rate_ * accrual);
        NPV_ = direction_sign_ * (maturity_amount * maturity_df - notional_ * start_df);
        errorEstimate_ = 0.0;
        valuationDate_ = QuantLib::Settings::instance().evaluationDate();
    }

private:
    double notional_;
    double deposit_rate_;
    double direction_sign_;
    QuantLib::Date start_date_;
    QuantLib::Date maturity_date_;
    QuantLib::DayCounter day_count_;
    QuantLib::Handle<QuantLib::YieldTermStructure> discount_handle_;
};

class InterestRateFutureInstrument final : public QuantLib::Instrument {
public:
    InterestRateFutureInstrument(
        QuantLib::Date start_date,
        QuantLib::Date maturity_date,
        QuantLib::DayCounter day_count,
        QuantLib::Handle<QuantLib::YieldTermStructure> forecast_handle,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> market_quote)
        : start_date_(std::move(start_date)),
          maturity_date_(std::move(maturity_date)),
          day_count_(std::move(day_count)),
          forecast_handle_(std::move(forecast_handle)),
          market_quote_(std::move(market_quote)) {}

    bool isExpired() const override {
        return start_date_ <= QuantLib::Settings::instance().evaluationDate();
    }

protected:
    void performCalculations() const override {
        if (market_quote_) {
            NPV_ = futures_price_from_quote(market_quote_->value());
        } else {
            const auto forward_rate = forecast_handle_->forwardRate(
                start_date_,
                maturity_date_,
                day_count_,
                QuantLib::Simple).rate();
            NPV_ = 100.0 * (1.0 - forward_rate);
        }
        errorEstimate_ = 0.0;
        valuationDate_ = QuantLib::Settings::instance().evaluationDate();
    }

private:
    QuantLib::Date start_date_;
    QuantLib::Date maturity_date_;
    QuantLib::DayCounter day_count_;
    QuantLib::Handle<QuantLib::YieldTermStructure> forecast_handle_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> market_quote_;
};

class FxSpotInstrument final : public QuantLib::Instrument {
public:
    FxSpotInstrument(
        double notional,
        double reference_rate,
        double direction_sign,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> spot_quote)
        : notional_(notional),
          reference_rate_(reference_rate),
          direction_sign_(direction_sign),
          spot_quote_(std::move(spot_quote)) {
        registerWith(spot_quote_);
    }

    bool isExpired() const override { return false; }

protected:
    void performCalculations() const override {
        NPV_ = direction_sign_ * notional_ * (spot_quote_->value() - reference_rate_);
        errorEstimate_ = 0.0;
        valuationDate_ = QuantLib::Settings::instance().evaluationDate();
    }

private:
    double notional_;
    double reference_rate_;
    double direction_sign_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> spot_quote_;
};

class FxForwardInstrument final : public QuantLib::Instrument {
public:
    FxForwardInstrument(
        double notional,
        double contract_forward,
        double direction_sign,
        QuantLib::Date maturity_date,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> spot_quote,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> forward_quote,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> forward_points_quote,
        QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> domestic_curve,
        QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> foreign_curve)
        : notional_(notional),
          contract_forward_(contract_forward),
          direction_sign_(direction_sign),
          maturity_date_(std::move(maturity_date)),
          spot_quote_(std::move(spot_quote)),
          forward_quote_(std::move(forward_quote)),
          forward_points_quote_(std::move(forward_points_quote)),
          domestic_curve_(std::move(domestic_curve)),
          foreign_curve_(std::move(foreign_curve)) {
        registerWith(spot_quote_);
        if (forward_quote_) registerWith(forward_quote_);
        if (forward_points_quote_) registerWith(forward_points_quote_);
        if (domestic_curve_) registerWith(domestic_curve_);
        if (foreign_curve_) registerWith(foreign_curve_);
    }

    bool isExpired() const override {
        return maturity_date_ <= QuantLib::Settings::instance().evaluationDate();
    }

protected:
    void performCalculations() const override {
        const double market_forward = forward_rate_from_quotes(
            spot_quote_,
            forward_quote_,
            forward_points_quote_,
            domestic_curve_,
            foreign_curve_,
            maturity_date_);
        const double discount = discount_factor_or_one(domestic_curve_, maturity_date_);
        NPV_ = direction_sign_ * notional_ * (market_forward - contract_forward_) * discount;
        errorEstimate_ = 0.0;
        valuationDate_ = QuantLib::Settings::instance().evaluationDate();
    }

private:
    double notional_;
    double contract_forward_;
    double direction_sign_;
    QuantLib::Date maturity_date_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> spot_quote_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> forward_quote_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> forward_points_quote_;
    QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> domestic_curve_;
    QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> foreign_curve_;
};

class FxSwapInstrument final : public QuantLib::Instrument {
public:
    FxSwapInstrument(
        double notional,
        double near_rate,
        double far_rate,
        double direction_sign,
        QuantLib::Date near_date,
        QuantLib::Date far_date,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> spot_quote,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> near_forward_quote,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> near_forward_points_quote,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> far_forward_quote,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> far_forward_points_quote,
        QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> domestic_curve,
        QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> foreign_curve)
        : notional_(notional),
          near_rate_(near_rate),
          far_rate_(far_rate),
          direction_sign_(direction_sign),
          near_date_(std::move(near_date)),
          far_date_(std::move(far_date)),
          spot_quote_(std::move(spot_quote)),
          near_forward_quote_(std::move(near_forward_quote)),
          near_forward_points_quote_(std::move(near_forward_points_quote)),
          far_forward_quote_(std::move(far_forward_quote)),
          far_forward_points_quote_(std::move(far_forward_points_quote)),
          domestic_curve_(std::move(domestic_curve)),
          foreign_curve_(std::move(foreign_curve)) {
        registerWith(spot_quote_);
        if (near_forward_quote_) registerWith(near_forward_quote_);
        if (near_forward_points_quote_) registerWith(near_forward_points_quote_);
        if (far_forward_quote_) registerWith(far_forward_quote_);
        if (far_forward_points_quote_) registerWith(far_forward_points_quote_);
        if (domestic_curve_) registerWith(domestic_curve_);
        if (foreign_curve_) registerWith(foreign_curve_);
    }

    bool isExpired() const override {
        return far_date_ <= QuantLib::Settings::instance().evaluationDate();
    }

protected:
    void performCalculations() const override {
        const double near_forward = forward_rate_from_quotes(
            spot_quote_,
            near_forward_quote_,
            near_forward_points_quote_,
            domestic_curve_,
            foreign_curve_,
            near_date_);
        const double far_forward = forward_rate_from_quotes(
            spot_quote_,
            far_forward_quote_,
            far_forward_points_quote_,
            domestic_curve_,
            foreign_curve_,
            far_date_);

        const double near_discount = discount_factor_or_one(domestic_curve_, near_date_);
        const double far_discount = discount_factor_or_one(domestic_curve_, far_date_);
        const double near_leg = (near_forward - near_rate_) * near_discount;
        const double far_leg = (far_forward - far_rate_) * far_discount;
        NPV_ = direction_sign_ * notional_ * (near_leg - far_leg);
        errorEstimate_ = 0.0;
        valuationDate_ = QuantLib::Settings::instance().evaluationDate();
    }

private:
    double notional_;
    double near_rate_;
    double far_rate_;
    double direction_sign_;
    QuantLib::Date near_date_;
    QuantLib::Date far_date_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> spot_quote_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> near_forward_quote_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> near_forward_points_quote_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> far_forward_quote_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> far_forward_points_quote_;
    QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> domestic_curve_;
    QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> foreign_curve_;
};

class FxOptionInstrument final : public QuantLib::Instrument {
public:
    FxOptionInstrument(
        double notional,
        double strike_rate,
        double direction_sign,
        bool is_put,
        QuantLib::Date expiry_date,
        QuantLib::Date settlement_date,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> spot_quote,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> volatility_quote,
        QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> domestic_curve,
        QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> foreign_curve)
        : notional_(notional),
          strike_rate_(strike_rate),
          direction_sign_(direction_sign),
          is_put_(is_put),
          expiry_date_(std::move(expiry_date)),
          settlement_date_(std::move(settlement_date)),
          spot_quote_(std::move(spot_quote)),
          volatility_quote_(std::move(volatility_quote)),
          domestic_curve_(std::move(domestic_curve)),
          foreign_curve_(std::move(foreign_curve)) {
        registerWith(spot_quote_);
        registerWith(volatility_quote_);
        if (domestic_curve_) registerWith(domestic_curve_);
        if (foreign_curve_) registerWith(foreign_curve_);
    }

    bool isExpired() const override {
        return expiry_date_ <= QuantLib::Settings::instance().evaluationDate();
    }

protected:
    void performCalculations() const override {
        const auto valuation_date = QuantLib::Settings::instance().evaluationDate();
        const QuantLib::Actual365Fixed day_count;
        const double time_to_expiry = day_count.yearFraction(valuation_date, expiry_date_);
        const double spot = spot_quote_->value();
        const double domestic_df = discount_factor_or_one(domestic_curve_, settlement_date_);
        const double foreign_df = discount_factor_or_one(foreign_curve_, settlement_date_);

        if (time_to_expiry <= 0.0 || strike_rate_ <= 0.0 || spot <= 0.0) {
            const double intrinsic = is_put_
                ? std::max(strike_rate_ - spot, 0.0)
                : std::max(spot - strike_rate_, 0.0);
            NPV_ = direction_sign_ * notional_ * intrinsic * domestic_df;
            errorEstimate_ = 0.0;
            valuationDate_ = valuation_date;
            return;
        }

        const double volatility = std::max(volatility_quote_->value(), 1.0e-8);
        const double forward = domestic_df > 0.0 ? spot * foreign_df / domestic_df : spot;
        const double std_dev = volatility * std::sqrt(time_to_expiry);
        const double d1 = (std::log(forward / strike_rate_) + 0.5 * volatility * volatility * time_to_expiry) / std_dev;
        const double d2 = d1 - std_dev;
        const double unit_value = is_put_
            ? strike_rate_ * domestic_df * normal_cdf(-d2) - spot * foreign_df * normal_cdf(-d1)
            : spot * foreign_df * normal_cdf(d1) - strike_rate_ * domestic_df * normal_cdf(d2);

        NPV_ = direction_sign_ * notional_ * unit_value;
        errorEstimate_ = 0.0;
        valuationDate_ = valuation_date;
    }

private:
    double notional_;
    double strike_rate_;
    double direction_sign_;
    bool is_put_;
    QuantLib::Date expiry_date_;
    QuantLib::Date settlement_date_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> spot_quote_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> volatility_quote_;
    QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> domestic_curve_;
    QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> foreign_curve_;
};

class MeanRevertingSwapRateProcess final : public analytics::simulation::StochasticProcess {
public:
    MeanRevertingSwapRateProcess(double initial_rate, double mean_reversion, double volatility)
        : initial_rate_(initial_rate),
          mean_reversion_(mean_reversion),
          volatility_(volatility) {}

    std::size_t dimension() const override { return 1; }

    void simulatePath(
        const analytics::simulation::TimeGrid& time_grid,
        std::mt19937& generator,
        analytics::simulation::MarketPath& output_path) const override {
        std::normal_distribution<double> normal(0.0, 1.0);
        double rate = initial_rate_;
        output_path(0, 0) = rate;

        for (std::size_t i = 1; i < time_grid.size(); ++i) {
            const double dt = time_grid.dt(i);
            const double dw = normal(generator) * std::sqrt(std::max(dt, 0.0));
            rate += mean_reversion_ * (initial_rate_ - rate) * dt + volatility_ * dw;
            output_path(i, 0) = std::max(rate, -0.02);
        }
    }

private:
    double initial_rate_;
    double mean_reversion_;
    double volatility_;
};

class BermudanSwaptionDecisionProblem final : public analytics::dynamic_programming::DecisionProblem {
public:
    BermudanSwaptionDecisionProblem(
        double strike_rate,
        bool payer,
        std::vector<double> exercise_annuities)
        : strike_rate_(strike_rate),
          payer_(payer),
          exercise_annuities_(std::move(exercise_annuities)) {}

    std::vector<analytics::dynamic_programming::Action> feasibleActions(
        const analytics::dynamic_programming::State&,
        std::size_t time_index) const override {
        if (time_index == 0 || time_index >= exercise_annuities_.size() || exercise_annuities_[time_index] <= 0.0) {
            return {{0, "Continue", {}}};
        }
        return {
            {0, "Continue", {}},
            {1, "Exercise", {}}
        };
    }

    double immediateCashflow(
        const analytics::dynamic_programming::State& state,
        const analytics::dynamic_programming::Action& action,
        std::size_t time_index) const override {
        if (action.id != 1) {
            return 0.0;
        }
        return exercise_payoff(state.market_variables[0], time_index);
    }

    analytics::dynamic_programming::State nextState(
        const analytics::dynamic_programming::State&,
        const analytics::dynamic_programming::Action&,
        const std::vector<double>& market_variables_next,
        std::size_t) const override {
        return {market_variables_next, {}};
    }

    bool isTerminalAction(
        const analytics::dynamic_programming::State&,
        const analytics::dynamic_programming::Action& action,
        std::size_t) const override {
        return action.id == 1;
    }

    std::vector<double> regressionFeatures(
        const analytics::dynamic_programming::State& state,
        std::size_t) const override {
        const double rate = state.market_variables[0];
        return {1.0, rate, rate * rate};
    }

    double terminalValue(const analytics::dynamic_programming::State& state) const override {
        return exercise_payoff(state.market_variables[0], exercise_annuities_.size() - 1);
    }

private:
    double exercise_payoff(double swap_rate, std::size_t time_index) const {
        const double intrinsic = payer_ ? swap_rate - strike_rate_ : strike_rate_ - swap_rate;
        return std::max(intrinsic, 0.0) * exercise_annuities_[time_index];
    }

    double strike_rate_;
    bool payer_;
    std::vector<double> exercise_annuities_;
};

class BermudanSwaptionLsmcInstrument final : public QuantLib::Instrument {
public:
    BermudanSwaptionLsmcInstrument(
        double notional,
        double fixed_rate,
        bool payer,
        double mean_reversion,
        double volatility,
        QuantLib::Date start_date,
        QuantLib::Date maturity_date,
        std::vector<QuantLib::Date> exercise_dates,
        QuantLib::Schedule fixed_schedule,
        QuantLib::DayCounter fixed_day_count,
        QuantLib::Handle<QuantLib::YieldTermStructure> discount_handle)
        : notional_(notional),
          fixed_rate_(fixed_rate),
          payer_(payer),
          mean_reversion_(mean_reversion),
          volatility_(volatility),
          start_date_(std::move(start_date)),
          maturity_date_(std::move(maturity_date)),
          exercise_dates_(std::move(exercise_dates)),
          fixed_schedule_(std::move(fixed_schedule)),
          fixed_day_count_(std::move(fixed_day_count)),
          discount_handle_(std::move(discount_handle)) {}

    bool isExpired() const override {
        return exercise_dates_.empty() ||
               exercise_dates_.back() <= QuantLib::Settings::instance().evaluationDate();
    }

protected:
    void performCalculations() const override {
        const auto valuation_date = QuantLib::Settings::instance().evaluationDate();
        const QuantLib::Actual365Fixed time_day_count;

        std::vector<double> times{0.0};
        std::vector<double> annuities{0.0};
        for (const auto& exercise_date : exercise_dates_) {
            if (exercise_date <= valuation_date || exercise_date > maturity_date_) {
                continue;
            }
            times.push_back(time_day_count.yearFraction(valuation_date, exercise_date));
            annuities.push_back(swap_annuity_at_exercise(
                exercise_date,
                fixed_schedule_,
                fixed_day_count_,
                discount_handle_.currentLink(),
                notional_));
        }

        if (times.size() <= 1) {
            NPV_ = 0.0;
            errorEstimate_ = 0.0;
            valuationDate_ = valuation_date;
            return;
        }

        const double initial_swap_rate = std::max(
            par_swap_rate(
                start_date_,
                maturity_date_,
                fixed_schedule_,
                fixed_day_count_,
                discount_handle_.currentLink()),
            1.0e-6);
        const double discount_rate = discount_handle_->zeroRate(
            times.back(),
            QuantLib::Continuous,
            QuantLib::Annual).rate();

        analytics::simulation::TimeGrid time_grid(times);
        MeanRevertingSwapRateProcess process(initial_swap_rate, mean_reversion_, volatility_);
        BermudanSwaptionDecisionProblem problem(fixed_rate_, payer_, annuities);

        analytics::lsmc::LsmcConfig config;
        config.discount_rate = discount_rate;
        config.num_paths = 4096;
        config.seed = 42;

        analytics::lsmc::LsmcEngine engine(config);
        analytics::dynamic_programming::State initial_state{{initial_swap_rate}, {}};
        const auto result = engine.run(time_grid, process, problem, initial_state);

        NPV_ = result.value;
        errorEstimate_ = result.standard_error;
        valuationDate_ = valuation_date;
    }

private:
    double notional_;
    double fixed_rate_;
    bool payer_;
    double mean_reversion_;
    double volatility_;
    QuantLib::Date start_date_;
    QuantLib::Date maturity_date_;
    std::vector<QuantLib::Date> exercise_dates_;
    QuantLib::Schedule fixed_schedule_;
    QuantLib::DayCounter fixed_day_count_;
    QuantLib::Handle<QuantLib::YieldTermStructure> discount_handle_;
};

QuantLib::ext::shared_ptr<QuantLib::VanillaSwap> make_vanilla_swap(
    double notional,
    const std::string& direction,
    const std::string& currency,
    const std::string& start_date,
    const std::string& maturity_date,
    double fixed_rate,
    const std::string& floating_index,
    const analytics::PricingContext& context,
    bool attach_engine) {
    const auto start = market::CurveBuilder::parse_date(start_date);
    const auto maturity = market::CurveBuilder::parse_date(maturity_date);
    const auto currency_code = domain::from_string(currency);
    const auto index_family = normalize_index_family(floating_index);
    const auto convention = rates_convention(currency_code, index_family);

    auto discount = discount_curve(context, currency_code);
    auto forecast = forecast_curve(context, currency_code, index_family);
    if (!discount || !forecast) {
        return nullptr;
    }

    QuantLib::Handle<QuantLib::YieldTermStructure> discount_handle(discount);
    QuantLib::Handle<QuantLib::YieldTermStructure> forecast_handle(forecast);
    auto index = make_ibor_index(currency_code, index_family, forecast_handle, context, floating_index);

    const auto calendar = market::CurveBuilder::parse_calendar(convention.calendar);
    const auto fixed_bdc = market::CurveBuilder::parse_business_day_convention(convention.fixed_leg_bdc);
    const auto floating_bdc = market::CurveBuilder::parse_business_day_convention(convention.floating_leg_bdc);
    const auto fixed_frequency = market::CurveBuilder::parse_frequency(convention.fixed_leg_frequency);
    const auto floating_frequency = market::CurveBuilder::parse_frequency(convention.floating_leg_frequency);
    const auto fixed_day_count = market::CurveBuilder::parse_day_count(convention.fixed_leg_day_count);
    const auto floating_day_count = market::CurveBuilder::parse_day_count(convention.floating_leg_day_count);
    const auto rule = market::CurveBuilder::parse_date_generation(convention.date_generation);

    auto swap = QuantLib::ext::make_shared<QuantLib::VanillaSwap>(
        swap_type_from_direction(direction),
        notional,
        make_schedule(start, maturity, fixed_frequency, calendar, fixed_bdc, rule),
        fixed_rate,
        fixed_day_count,
        make_schedule(start, maturity, floating_frequency, calendar, floating_bdc, rule),
        index,
        0.0,
        floating_day_count);

    if (attach_engine) {
        swap->setPricingEngine(QuantLib::ext::make_shared<QuantLib::DiscountingSwapEngine>(discount_handle));
    }
    return swap;
}

} // namespace

QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_instrument(
    const domain::Trade& trade,
    const analytics::PricingContext& context) {
    return analytics::ProductPricingRegistry::create_instrument(trade, context);
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_deposit(
    const domain::DepositTrade& trade,
    const analytics::PricingContext& context) {
    const auto currency_code = domain::from_string(trade.currency);
    const auto curve = discount_curve(context, currency_code);
    if (!curve) {
        return nullptr;
    }

    const auto convention = rates_convention(currency_code, "OIS");
    QuantLib::Handle<QuantLib::YieldTermStructure> discount_handle(curve);
    return QuantLib::ext::make_shared<DepositInstrument>(
        trade.notional,
        trade.deposit_rate,
        deposit_direction_sign(trade.direction),
        market::CurveBuilder::parse_date(trade.start_date),
        market::CurveBuilder::parse_date(trade.maturity_date),
        market::CurveBuilder::parse_day_count(convention.day_count),
        discount_handle);
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_fra(
    const domain::FraTrade& trade,
    const analytics::PricingContext& context) {
    const auto currency_code = domain::from_string(trade.currency);
    const auto index_family = normalize_index_family(trade.floating_index);
    auto discount = discount_curve(context, currency_code);
    auto forecast = forecast_curve(context, currency_code, index_family);
    if (!discount || !forecast) {
        return nullptr;
    }

    QuantLib::Handle<QuantLib::YieldTermStructure> discount_handle(discount);
    QuantLib::Handle<QuantLib::YieldTermStructure> forecast_handle(forecast);
    auto index = make_ibor_index(currency_code, index_family, forecast_handle, context, trade.floating_index);

    return QuantLib::ext::make_shared<QuantLib::ForwardRateAgreement>(
        index,
        market::CurveBuilder::parse_date(trade.start_date),
        market::CurveBuilder::parse_date(trade.maturity_date),
        fra_position_type(trade.direction),
        trade.strike_rate,
        trade.notional,
        discount_handle);
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_interest_rate_future(
    const domain::InterestRateFutureTrade& trade,
    const analytics::PricingContext& context) {
    const auto currency_code = domain::from_string(trade.currency);
    const auto index_family = normalize_index_family(trade.floating_index);
    auto forecast = forecast_curve(context, currency_code, index_family);
    if (!forecast) {
        return nullptr;
    }

    const auto convention = rates_convention(currency_code, index_family);
    QuantLib::Handle<QuantLib::YieldTermStructure> forecast_handle(forecast);
    auto quote = trade.future_quote_id.empty()
        ? nullptr
        : context.market_state().get_quote_handle(trade.future_quote_id);

    return QuantLib::ext::make_shared<InterestRateFutureInstrument>(
        market::CurveBuilder::parse_date(trade.start_date),
        market::CurveBuilder::parse_date(trade.maturity_date),
        market::CurveBuilder::parse_day_count(convention.day_count),
        forecast_handle,
        quote);
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_swap(
    const domain::VanillaSwapTrade& trade,
    const analytics::PricingContext& context) {
    return make_vanilla_swap(
        trade.notional,
        trade.direction,
        trade.currency,
        trade.start_date,
        trade.maturity_date,
        trade.fixed_rate,
        trade.floating_index,
        context,
        true);
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_ois_swap(
    const domain::OisSwapTrade& trade,
    const analytics::PricingContext& context) {
    const auto start = market::CurveBuilder::parse_date(trade.start_date);
    const auto maturity = market::CurveBuilder::parse_date(trade.maturity_date);
    const auto currency_code = domain::from_string(trade.currency);
    auto discount = discount_curve(context, currency_code);
    if (!discount) {
        return nullptr;
    }

    QuantLib::Handle<QuantLib::YieldTermStructure> discount_handle(discount);
    const auto convention = rates_convention(currency_code, "OIS");
    auto overnight_index = make_overnight_index(currency_code, discount_handle, context, trade.overnight_index);

    const auto calendar = market::CurveBuilder::parse_calendar(convention.calendar);
    const auto bdc = market::CurveBuilder::parse_business_day_convention(convention.business_day_convention);
    const auto fixed_frequency = market::CurveBuilder::parse_frequency(convention.fixed_leg_frequency);
    const auto fixed_day_count = market::CurveBuilder::parse_day_count(convention.fixed_leg_day_count);
    const auto rule = market::CurveBuilder::parse_date_generation(convention.date_generation);

    auto swap = QuantLib::ext::make_shared<QuantLib::OvernightIndexedSwap>(
        swap_type_from_direction(trade.direction),
        trade.notional,
        make_schedule(start, maturity, fixed_frequency, calendar, bdc, rule),
        trade.fixed_rate,
        fixed_day_count,
        overnight_index,
        trade.spread);
    swap->setPricingEngine(QuantLib::ext::make_shared<QuantLib::DiscountingSwapEngine>(discount_handle));
    return swap;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_bond(
    const domain::FixedRateBondTrade& trade,
    const analytics::PricingContext& context) {
    const auto start = market::CurveBuilder::parse_date(trade.start_date);
    const auto maturity = market::CurveBuilder::parse_date(trade.maturity_date);
    const auto currency_code = domain::from_string(trade.currency);
    auto curve = discount_curve(context, currency_code);
    if (!curve) {
        return nullptr;
    }

    const auto convention = rates_convention(currency_code, "OIS");
    const auto calendar = market::CurveBuilder::parse_calendar(convention.calendar);
    const auto bdc = market::CurveBuilder::parse_business_day_convention(convention.business_day_convention);
    const auto frequency = market::CurveBuilder::parse_frequency(
        parse_frequency_string(trade.frequency, domain::Frequency::Annual));
    const auto day_count = market::CurveBuilder::parse_day_count(domain::DayCount::Thirty360);

    auto bond = QuantLib::ext::make_shared<QuantLib::FixedRateBond>(
        0,
        trade.notional,
        make_schedule(start, maturity, frequency, calendar, bdc, QuantLib::DateGeneration::Backward),
        std::vector<QuantLib::Rate>{trade.coupon_rate},
        day_count,
        bdc,
        100.0,
        start);
    bond->setPricingEngine(QuantLib::ext::make_shared<QuantLib::DiscountingBondEngine>(
        QuantLib::Handle<QuantLib::YieldTermStructure>(curve)));
    return bond;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_floating_rate_note(
    const domain::FloatingRateNoteTrade& trade,
    const analytics::PricingContext& context) {
    const auto start = market::CurveBuilder::parse_date(trade.start_date);
    const auto maturity = market::CurveBuilder::parse_date(trade.maturity_date);
    const auto currency_code = domain::from_string(trade.currency);
    const auto index_family = normalize_index_family(trade.floating_index);
    auto discount = discount_curve(context, currency_code);
    auto forecast = forecast_curve(context, currency_code, index_family);
    if (!discount || !forecast) {
        return nullptr;
    }

    const auto convention = rates_convention(currency_code, index_family);
    QuantLib::Handle<QuantLib::YieldTermStructure> forecast_handle(forecast);
    auto index = make_ibor_index(currency_code, index_family, forecast_handle, context, trade.floating_index);

    const auto calendar = market::CurveBuilder::parse_calendar(convention.calendar);
    const auto bdc = market::CurveBuilder::parse_business_day_convention(convention.business_day_convention);
    const auto frequency = market::CurveBuilder::parse_frequency(
        parse_frequency_string(trade.frequency, convention.floating_leg_frequency));
    const auto day_count = market::CurveBuilder::parse_day_count(convention.floating_leg_day_count);

    auto bond = QuantLib::ext::make_shared<QuantLib::FloatingRateBond>(
        0,
        trade.notional,
        make_schedule(start, maturity, frequency, calendar, bdc, QuantLib::DateGeneration::Backward),
        index,
        day_count,
        bdc,
        static_cast<QuantLib::Natural>(convention.settlement_days),
        std::vector<double>{1.0},
        std::vector<double>{trade.spread},
        std::vector<double>{},
        std::vector<double>{},
        false,
        100.0,
        start);
    QuantLib::setCouponPricer(
        bond->cashflows(),
        QuantLib::ext::make_shared<QuantLib::BlackIborCouponPricer>());
    bond->setPricingEngine(QuantLib::ext::make_shared<QuantLib::DiscountingBondEngine>(
        QuantLib::Handle<QuantLib::YieldTermStructure>(discount)));
    return bond;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_cap_floor(
    const domain::CapFloorTrade& trade,
    const analytics::PricingContext& context) {
    const auto start = market::CurveBuilder::parse_date(trade.start_date);
    const auto maturity = market::CurveBuilder::parse_date(trade.maturity_date);
    const auto currency_code = domain::from_string(trade.currency);
    const auto index_family = normalize_index_family(trade.floating_index);
    auto discount = discount_curve(context, currency_code);
    auto forecast = forecast_curve(context, currency_code, index_family);
    if (!discount || !forecast) {
        return nullptr;
    }

    const auto convention = rates_convention(currency_code, index_family);
    const auto calendar = market::CurveBuilder::parse_calendar(convention.calendar);
    const auto bdc = market::CurveBuilder::parse_business_day_convention(convention.floating_leg_bdc);
    const auto frequency = market::CurveBuilder::parse_frequency(convention.floating_leg_frequency);
    const auto day_count = market::CurveBuilder::parse_day_count(convention.floating_leg_day_count);
    const auto rule = market::CurveBuilder::parse_date_generation(convention.date_generation);

    QuantLib::Handle<QuantLib::YieldTermStructure> forecast_handle(forecast);
    auto index = make_ibor_index(currency_code, index_family, forecast_handle, context, trade.floating_index);
    QuantLib::Leg floating_leg = QuantLib::IborLeg(make_schedule(start, maturity, frequency, calendar, bdc, rule), index)
        .withNotionals(trade.notional)
        .withPaymentDayCounter(day_count)
        .withPaymentAdjustment(bdc);
    QuantLib::setCouponPricer(
        floating_leg,
        QuantLib::ext::make_shared<QuantLib::BlackIborCouponPricer>());

    QuantLib::ext::shared_ptr<QuantLib::CapFloor> instrument;
    const auto cap_floor_type = lower_copy(trade.cap_floor_type);
    if (cap_floor_type == "floor") {
        instrument = QuantLib::ext::make_shared<QuantLib::Floor>(
            floating_leg,
            std::vector<QuantLib::Rate>{trade.strike_rate});
    } else {
        instrument = QuantLib::ext::make_shared<QuantLib::Cap>(
            floating_leg,
            std::vector<QuantLib::Rate>{trade.strike_rate});
    }

    const double volatility = quote_or_default_volatility(context, trade.volatility_quote_id, trade.volatility, 0.20);
    instrument->setPricingEngine(QuantLib::ext::make_shared<QuantLib::BlackCapFloorEngine>(
        QuantLib::Handle<QuantLib::YieldTermStructure>(discount),
        volatility,
        QuantLib::Actual365Fixed()));
    return instrument;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_european_swaption(
    const domain::EuropeanSwaptionTrade& trade,
    const analytics::PricingContext& context) {
    auto underlying = make_vanilla_swap(
        trade.notional,
        trade.direction,
        trade.currency,
        trade.start_date,
        trade.maturity_date,
        trade.fixed_rate,
        trade.floating_index,
        context,
        false);
    if (!underlying) {
        return nullptr;
    }

    const auto currency_code = domain::from_string(trade.currency);
    auto discount = discount_curve(context, currency_code);
    if (!discount) {
        return nullptr;
    }

    const auto expiry = trade.option_expiry_date.empty() ? trade.start_date : trade.option_expiry_date;
    auto swaption = QuantLib::ext::make_shared<QuantLib::Swaption>(
        underlying,
        QuantLib::ext::make_shared<QuantLib::EuropeanExercise>(market::CurveBuilder::parse_date(expiry)));
    const double volatility = quote_or_default_volatility(context, trade.volatility_quote_id, trade.volatility, 0.20);
    swaption->setPricingEngine(QuantLib::ext::make_shared<QuantLib::BlackSwaptionEngine>(
        QuantLib::Handle<QuantLib::YieldTermStructure>(discount),
        volatility,
        QuantLib::Actual365Fixed()));
    return swaption;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_bermudan_swaption(
    const domain::BermudanSwaptionTrade& trade,
    const analytics::PricingContext& context) {
    const auto start = market::CurveBuilder::parse_date(trade.start_date);
    const auto maturity = market::CurveBuilder::parse_date(trade.maturity_date);
    const auto currency_code = domain::from_string(trade.currency);
    const auto index_family = normalize_index_family(trade.floating_index);
    auto discount = discount_curve(context, currency_code);
    if (!discount) {
        return nullptr;
    }

    const auto convention = rates_convention(currency_code, index_family);
    const auto calendar = market::CurveBuilder::parse_calendar(convention.calendar);
    const auto bdc = market::CurveBuilder::parse_business_day_convention(convention.fixed_leg_bdc);
    const auto frequency = market::CurveBuilder::parse_frequency(convention.fixed_leg_frequency);
    const auto fixed_day_count = market::CurveBuilder::parse_day_count(convention.fixed_leg_day_count);
    const auto rule = market::CurveBuilder::parse_date_generation(convention.date_generation);
    auto fixed_schedule = make_schedule(start, maturity, frequency, calendar, bdc, rule);

    std::vector<QuantLib::Date> exercise_dates;
    for (const auto& exercise_date : trade.exercise_dates) {
        exercise_dates.push_back(market::CurveBuilder::parse_date(exercise_date));
    }
    if (exercise_dates.empty()) {
        exercise_dates.push_back(start);
    }
    std::sort(exercise_dates.begin(), exercise_dates.end());

    const double volatility = quote_or_default_volatility(context, trade.volatility_quote_id, trade.volatility, 0.01);
    return QuantLib::ext::make_shared<BermudanSwaptionLsmcInstrument>(
        trade.notional,
        trade.fixed_rate,
        swap_type_from_direction(trade.direction) == QuantLib::VanillaSwap::Payer,
        trade.mean_reversion,
        volatility,
        start,
        maturity,
        exercise_dates,
        fixed_schedule,
        fixed_day_count,
        QuantLib::Handle<QuantLib::YieldTermStructure>(discount));
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_fx_spot(
    const domain::FxSpotTrade& trade,
    const analytics::PricingContext& context) {
    const std::string quote_id = trade.spot_quote_id.empty()
        ? fx_pair(trade.base_currency, trade.quote_currency)
        : trade.spot_quote_id;
    auto spot_quote = context.market_state().get_quote_handle(quote_id);
    if (!spot_quote) {
        return nullptr;
    }

    return QuantLib::ext::make_shared<FxSpotInstrument>(
        trade.notional,
        trade.reference_rate,
        fx_direction_sign(trade.direction),
        spot_quote);
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_fx_forward(
    const domain::FxForwardTrade& trade,
    const analytics::PricingContext& context) {
    const std::string quote_id = trade.spot_quote_id.empty()
        ? fx_pair(trade.base_currency, trade.quote_currency)
        : trade.spot_quote_id;
    auto spot_quote = context.market_state().get_quote_handle(quote_id);
    if (!spot_quote) {
        return nullptr;
    }

    const auto quote_currency = domain::from_string(trade.quote_currency);
    const auto base_currency = domain::from_string(trade.base_currency);
    auto forward_quote = trade.forward_quote_id.empty()
        ? nullptr
        : context.market_state().get_quote_handle(trade.forward_quote_id);
    auto forward_points_quote = trade.forward_points_quote_id.empty()
        ? nullptr
        : context.market_state().get_quote_handle(trade.forward_points_quote_id);

    return QuantLib::ext::make_shared<FxForwardInstrument>(
        trade.notional,
        trade.forward_rate,
        fx_direction_sign(trade.direction),
        market::CurveBuilder::parse_date(trade.maturity_date),
        spot_quote,
        forward_quote,
        forward_points_quote,
        discount_curve(context, quote_currency),
        discount_curve(context, base_currency));
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_fx_swap(
    const domain::FxSwapTrade& trade,
    const analytics::PricingContext& context) {
    const std::string quote_id = trade.spot_quote_id.empty()
        ? fx_pair(trade.base_currency, trade.quote_currency)
        : trade.spot_quote_id;
    auto spot_quote = context.market_state().get_quote_handle(quote_id);
    if (!spot_quote) {
        return nullptr;
    }

    const auto quote_currency = domain::from_string(trade.quote_currency);
    const auto base_currency = domain::from_string(trade.base_currency);
    auto near_forward_quote = trade.near_forward_quote_id.empty()
        ? nullptr
        : context.market_state().get_quote_handle(trade.near_forward_quote_id);
    auto near_forward_points_quote = trade.near_forward_points_quote_id.empty()
        ? nullptr
        : context.market_state().get_quote_handle(trade.near_forward_points_quote_id);
    auto far_forward_quote = trade.far_forward_quote_id.empty()
        ? nullptr
        : context.market_state().get_quote_handle(trade.far_forward_quote_id);
    auto far_forward_points_quote = trade.far_forward_points_quote_id.empty()
        ? nullptr
        : context.market_state().get_quote_handle(trade.far_forward_points_quote_id);

    return QuantLib::ext::make_shared<FxSwapInstrument>(
        trade.notional,
        trade.near_rate,
        trade.far_rate,
        fx_direction_sign(trade.direction),
        market::CurveBuilder::parse_date(trade.start_date),
        market::CurveBuilder::parse_date(trade.maturity_date),
        spot_quote,
        near_forward_quote,
        near_forward_points_quote,
        far_forward_quote,
        far_forward_points_quote,
        discount_curve(context, quote_currency),
        discount_curve(context, base_currency));
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_ndf(
    const domain::NdfTrade& trade,
    const analytics::PricingContext& context) {
    const std::string quote_id = trade.spot_quote_id.empty()
        ? fx_pair(trade.base_currency, trade.quote_currency)
        : trade.spot_quote_id;
    auto spot_quote = context.market_state().get_quote_handle(quote_id);
    if (!spot_quote) {
        return nullptr;
    }

    const auto quote_currency = domain::from_string(trade.quote_currency);
    const auto base_currency = domain::from_string(trade.base_currency);
    auto forward_quote = !trade.fixing_quote_id.empty()
        ? context.market_state().get_quote_handle(trade.fixing_quote_id)
        : nullptr;
    if (!forward_quote && !trade.forward_quote_id.empty()) {
        forward_quote = context.market_state().get_quote_handle(trade.forward_quote_id);
    }
    auto forward_points_quote = trade.forward_points_quote_id.empty()
        ? nullptr
        : context.market_state().get_quote_handle(trade.forward_points_quote_id);

    return QuantLib::ext::make_shared<FxForwardInstrument>(
        trade.notional,
        trade.forward_rate,
        fx_direction_sign(trade.direction),
        market::CurveBuilder::parse_date(trade.maturity_date),
        spot_quote,
        forward_quote,
        forward_points_quote,
        discount_curve(context, quote_currency),
        discount_curve(context, base_currency));
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_fx_option(
    const domain::FxOptionTrade& trade,
    const analytics::PricingContext& context) {
    const std::string quote_id = trade.spot_quote_id.empty()
        ? fx_pair(trade.base_currency, trade.quote_currency)
        : trade.spot_quote_id;
    auto spot_quote = context.market_state().get_quote_handle(quote_id);
    if (!spot_quote) {
        return nullptr;
    }

    auto volatility_quote = trade.volatility_quote_id.empty()
        ? nullptr
        : context.market_state().get_quote_handle(trade.volatility_quote_id);
    if (!volatility_quote) {
        volatility_quote = QuantLib::ext::make_shared<QuantLib::SimpleQuote>(
            trade.volatility > 0.0 ? trade.volatility : 0.10);
    }

    const auto quote_currency = domain::from_string(trade.quote_currency);
    const auto base_currency = domain::from_string(trade.base_currency);
    const auto expiry_date = market::CurveBuilder::parse_date(trade.expiry_date);
    const auto settlement_date = trade.settlement_date.empty()
        ? expiry_date
        : market::CurveBuilder::parse_date(trade.settlement_date);

    return QuantLib::ext::make_shared<FxOptionInstrument>(
        trade.notional,
        trade.strike_rate,
        fx_direction_sign(trade.direction),
        is_put_option(trade.option_type),
        expiry_date,
        settlement_date,
        spot_quote,
        volatility_quote,
        discount_curve(context, quote_currency),
        discount_curve(context, base_currency));
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_equity_spot(
    const domain::EquitySpotTrade& trade,
    const analytics::PricingContext& context) {
    auto quote = context.market_state().get_quote_handle(trade.underlier);
    if (!quote) {
        return nullptr;
    }
    return QuantLib::ext::make_shared<QuantLib::Stock>(QuantLib::Handle<QuantLib::Quote>(quote));
}

} // namespace qrp::instruments
