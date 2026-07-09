// Implements rates instrument construction.

#include <qrp/analytics/exercise_policy.hpp>
#include <qrp/analytics/lsmc/lsmc_engine.hpp>
#include <qrp/analytics/simulation/stochastic_process.hpp>
#include <qrp/instruments/instrument_factory.hpp>
#include <qrp/instruments/instrument_factory_common.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace qrp::instruments {
namespace {

class DepositInstrument final : public QuantLib::Instrument {
public:
    DepositInstrument(double notional,
                      double deposit_rate,
                      double direction_sign,
                      QuantLib::Date start_date,
                      QuantLib::Date maturity_date,
                      QuantLib::DayCounter day_count,
                      QuantLib::Handle<QuantLib::YieldTermStructure> discount_handle)
        : notional_(notional), deposit_rate_(deposit_rate), direction_sign_(direction_sign),
          start_date_(std::move(start_date)), maturity_date_(std::move(maturity_date)),
          day_count_(std::move(day_count)), discount_handle_(std::move(discount_handle)) {}

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
    InterestRateFutureInstrument(QuantLib::Date start_date,
                                 QuantLib::Date maturity_date,
                                 QuantLib::DayCounter day_count,
                                 QuantLib::Handle<QuantLib::YieldTermStructure> forecast_handle,
                                 QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> market_quote)
        : start_date_(std::move(start_date)), maturity_date_(std::move(maturity_date)),
          day_count_(std::move(day_count)), forecast_handle_(std::move(forecast_handle)),
          market_quote_(std::move(market_quote)) {}

    bool isExpired() const override {
        return start_date_ <= QuantLib::Settings::instance().evaluationDate();
    }

protected:
    void performCalculations() const override {
        if (market_quote_) {
            NPV_ = futures_price_from_quote(market_quote_->value());
        } else {
            const auto forward_rate =
                forecast_handle_->forwardRate(start_date_, maturity_date_, day_count_, QuantLib::Simple).rate();
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

class MeanRevertingSwapRateProcess final : public analytics::simulation::StochasticProcess {
public:
    MeanRevertingSwapRateProcess(double initial_rate, double mean_reversion, double volatility)
        : initial_rate_(initial_rate), mean_reversion_(mean_reversion), volatility_(volatility) {}

    std::size_t dimension() const override {
        return 1;
    }

    void simulatePath(const analytics::simulation::TimeGrid& time_grid,
                      std::mt19937& generator,
                      analytics::simulation::MarketPath& output_path) const override {
        double rate = initial_rate_;
        output_path(0, 0) = rate;

        for (std::size_t i = 1; i < time_grid.size(); ++i) {
            const double dt = time_grid.dt(i);
            const double dw = standard_normal(generator) * std::sqrt(std::max(dt, 0.0));
            rate += mean_reversion_ * (initial_rate_ - rate) * dt + volatility_ * dw;
            output_path(i, 0) = std::max(rate, -0.02);
        }
    }

private:
    static double open_unit_uniform(std::mt19937& generator) {
        constexpr double denominator =
            static_cast<double>(std::mt19937::max()) - static_cast<double>(std::mt19937::min()) + 2.0;
        return (static_cast<double>(generator() - std::mt19937::min()) + 1.0) / denominator;
    }

    static double standard_normal(std::mt19937& generator) {
        constexpr double two_pi = 6.283185307179586476925286766559;
        const double u1 = open_unit_uniform(generator);
        const double u2 = open_unit_uniform(generator);
        return std::sqrt(-2.0 * std::log(u1)) * std::cos(two_pi * u2);
    }

    double initial_rate_;
    double mean_reversion_;
    double volatility_;
};

class BermudanSwaptionExercisePolicy final : public analytics::exercise::ExercisePolicy {
public:
    BermudanSwaptionExercisePolicy(double strike_rate, bool payer, std::vector<double> exercise_annuities)
        : strike_rate_(strike_rate), payer_(payer), exercise_annuities_(std::move(exercise_annuities)) {}

    bool canExercise(const analytics::dynamic_programming::State& state, std::size_t time_index) const override {
        return !state.market_variables.empty() && time_index > 0U && time_index < exercise_annuities_.size() &&
               exercise_annuities_[time_index] > 0.0;
    }

    double exerciseValue(const analytics::dynamic_programming::State& state, std::size_t time_index) const override {
        if (!canExercise(state, time_index)) {
            return 0.0;
        }
        return exercise_payoff(state.market_variables[0], time_index);
    }

    std::vector<double> regressionFeatures(const analytics::dynamic_programming::State& state,
                                           std::size_t) const override {
        const double rate = state.market_variables[0];
        return {1.0, rate, rate * rate};
    }

    std::vector<std::string> regressionFeatureNames(std::size_t) const override {
        return {"1", "swap_rate", "swap_rate^2"};
    }

    double terminalValue(const analytics::dynamic_programming::State& state, std::size_t) const override {
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
    BermudanSwaptionLsmcInstrument(double notional,
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
        : notional_(notional), fixed_rate_(fixed_rate), payer_(payer), mean_reversion_(mean_reversion),
          volatility_(volatility), start_date_(std::move(start_date)), maturity_date_(std::move(maturity_date)),
          exercise_dates_(std::move(exercise_dates)), fixed_schedule_(std::move(fixed_schedule)),
          fixed_day_count_(std::move(fixed_day_count)), discount_handle_(std::move(discount_handle)) {}

    bool isExpired() const override {
        return exercise_dates_.empty() || exercise_dates_.back() <= QuantLib::Settings::instance().evaluationDate();
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
            annuities.push_back(swap_annuity_at_exercise(exercise_date,
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

        const double initial_swap_rate = std::max(par_swap_rate(start_date_,
                                                                maturity_date_,
                                                                fixed_schedule_,
                                                                fixed_day_count_,
                                                                discount_handle_.currentLink()),
                                                  1.0e-6);
        const double discount_rate =
            discount_handle_->zeroRate(times.back(), QuantLib::Continuous, QuantLib::Annual).rate();

        analytics::simulation::TimeGrid time_grid(times);
        MeanRevertingSwapRateProcess process(initial_swap_rate, mean_reversion_, volatility_);
        auto policy = std::make_shared<BermudanSwaptionExercisePolicy>(fixed_rate_, payer_, annuities);
        analytics::exercise::ExercisePolicyDecisionProblem problem(policy, times.size() - 1U);

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

QuantLib::ext::shared_ptr<QuantLib::VanillaSwap> make_vanilla_swap(double notional,
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

QuantLib::ext::shared_ptr<QuantLib::Instrument>
RatesInstrumentFactory::create_deposit(const domain::DepositTrade& trade, const analytics::PricingContext& context) {
    const auto currency_code = domain::from_string(trade.currency);
    const auto curve = discount_curve(context, currency_code);
    if (!curve) {
        return nullptr;
    }

    const auto convention = rates_convention(currency_code, "OIS");
    QuantLib::Handle<QuantLib::YieldTermStructure> discount_handle(curve);
    return QuantLib::ext::make_shared<DepositInstrument>(trade.notional,
                                                         trade.deposit_rate,
                                                         deposit_direction_sign(trade.direction),
                                                         market::CurveBuilder::parse_date(trade.start_date),
                                                         market::CurveBuilder::parse_date(trade.maturity_date),
                                                         market::CurveBuilder::parse_day_count(convention.day_count),
                                                         discount_handle);
}

QuantLib::ext::shared_ptr<QuantLib::Instrument>
RatesInstrumentFactory::create_fra(const domain::FraTrade& trade, const analytics::PricingContext& context) {
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

QuantLib::ext::shared_ptr<QuantLib::Instrument>
RatesInstrumentFactory::create_interest_rate_future(const domain::InterestRateFutureTrade& trade,
                                                    const analytics::PricingContext& context) {
    const auto currency_code = domain::from_string(trade.currency);
    const auto index_family = normalize_index_family(trade.floating_index);
    auto forecast = forecast_curve(context, currency_code, index_family);
    if (!forecast) {
        return nullptr;
    }

    const auto convention = rates_convention(currency_code, index_family);
    QuantLib::Handle<QuantLib::YieldTermStructure> forecast_handle(forecast);
    auto quote =
        trade.future_quote_id.empty() ? nullptr : context.market_state().get_quote_handle(trade.future_quote_id);

    return QuantLib::ext::make_shared<InterestRateFutureInstrument>(
        market::CurveBuilder::parse_date(trade.start_date),
        market::CurveBuilder::parse_date(trade.maturity_date),
        market::CurveBuilder::parse_day_count(convention.day_count),
        forecast_handle,
        quote);
}

QuantLib::ext::shared_ptr<QuantLib::Instrument>
RatesInstrumentFactory::create_swap(const domain::VanillaSwapTrade& trade, const analytics::PricingContext& context) {
    return make_vanilla_swap(trade.notional,
                             trade.direction,
                             trade.currency,
                             trade.start_date,
                             trade.maturity_date,
                             trade.fixed_rate,
                             trade.floating_index,
                             context,
                             true);
}

QuantLib::ext::shared_ptr<QuantLib::Instrument>
RatesInstrumentFactory::create_ois_swap(const domain::OisSwapTrade& trade, const analytics::PricingContext& context) {
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

QuantLib::ext::shared_ptr<QuantLib::Instrument>
RatesInstrumentFactory::create_bond(const domain::FixedRateBondTrade& trade, const analytics::PricingContext& context) {
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
    const auto frequency =
        market::CurveBuilder::parse_frequency(parse_frequency_string(trade.frequency, domain::Frequency::Annual));
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

QuantLib::ext::shared_ptr<QuantLib::Instrument>
RatesInstrumentFactory::create_floating_rate_note(const domain::FloatingRateNoteTrade& trade,
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
    QuantLib::setCouponPricer(bond->cashflows(), QuantLib::ext::make_shared<QuantLib::BlackIborCouponPricer>());
    bond->setPricingEngine(QuantLib::ext::make_shared<QuantLib::DiscountingBondEngine>(
        QuantLib::Handle<QuantLib::YieldTermStructure>(discount)));
    return bond;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument>
RatesInstrumentFactory::create_cap_floor(const domain::CapFloorTrade& trade, const analytics::PricingContext& context) {
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
    QuantLib::Leg floating_leg =
        QuantLib::IborLeg(make_schedule(start, maturity, frequency, calendar, bdc, rule), index)
            .withNotionals(trade.notional)
            .withPaymentDayCounter(day_count)
            .withPaymentAdjustment(bdc);
    QuantLib::setCouponPricer(floating_leg, QuantLib::ext::make_shared<QuantLib::BlackIborCouponPricer>());

    QuantLib::ext::shared_ptr<QuantLib::CapFloor> instrument;
    const auto cap_floor_type = lower_copy(trade.cap_floor_type);
    if (cap_floor_type == "floor") {
        instrument =
            QuantLib::ext::make_shared<QuantLib::Floor>(floating_leg, std::vector<QuantLib::Rate>{trade.strike_rate});
    } else {
        instrument =
            QuantLib::ext::make_shared<QuantLib::Cap>(floating_leg, std::vector<QuantLib::Rate>{trade.strike_rate});
    }

    const double volatility = quote_or_default_volatility(context, trade.volatility_quote_id, trade.volatility, 0.20);
    instrument->setPricingEngine(QuantLib::ext::make_shared<QuantLib::BlackCapFloorEngine>(
        QuantLib::Handle<QuantLib::YieldTermStructure>(discount),
        volatility,
        QuantLib::Actual365Fixed()));
    return instrument;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument>
RatesInstrumentFactory::create_european_swaption(const domain::EuropeanSwaptionTrade& trade,
                                                 const analytics::PricingContext& context) {
    auto underlying = make_vanilla_swap(trade.notional,
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

QuantLib::ext::shared_ptr<QuantLib::Instrument>
RatesInstrumentFactory::create_bermudan_swaption(const domain::BermudanSwaptionTrade& trade,
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

} // namespace qrp::instruments
