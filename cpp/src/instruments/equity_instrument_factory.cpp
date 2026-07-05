// Implements equity instrument construction.

#include <qrp/instruments/instrument_factory.hpp>

#include <qrp/instruments/instrument_factory_common.hpp>


namespace qrp::instruments {
namespace {

class EquityForwardInstrument final : public QuantLib::Instrument {
public:
    EquityForwardInstrument(
        double quantity,
        double contract_forward,
        double direction_sign,
        QuantLib::Date maturity_date,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> spot_quote,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> dividend_yield_quote,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> borrow_rate_quote,
        QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve)
        : quantity_(quantity),
          contract_forward_(contract_forward),
          direction_sign_(direction_sign),
          maturity_date_(std::move(maturity_date)),
          spot_quote_(std::move(spot_quote)),
          dividend_yield_quote_(std::move(dividend_yield_quote)),
          borrow_rate_quote_(std::move(borrow_rate_quote)),
          discount_curve_(std::move(discount_curve)) {
        registerWith(spot_quote_);
        registerWith(dividend_yield_quote_);
        registerWith(borrow_rate_quote_);
        if (discount_curve_) registerWith(discount_curve_);
    }

    bool isExpired() const override {
        return maturity_date_ <= QuantLib::Settings::instance().evaluationDate();
    }

protected:
    void performCalculations() const override {
        const auto valuation_date = QuantLib::Settings::instance().evaluationDate();
        const QuantLib::Actual365Fixed day_count;
        const double time_to_maturity = std::max(day_count.yearFraction(valuation_date, maturity_date_), 0.0);
        const double discount = discount_factor_or_one(discount_curve_, maturity_date_);
        const double rate = time_to_maturity > 0.0 && discount > 0.0
            ? -std::log(discount) / time_to_maturity
            : 0.0;
        const double carry = rate + borrow_rate_quote_->value() - dividend_yield_quote_->value();
        const double forward = spot_quote_->value() * std::exp(carry * time_to_maturity);

        NPV_ = direction_sign_ * quantity_ * (forward - contract_forward_) * discount;
        errorEstimate_ = 0.0;
        valuationDate_ = valuation_date;
    }

private:
    double quantity_;
    double contract_forward_;
    double direction_sign_;
    QuantLib::Date maturity_date_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> spot_quote_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> dividend_yield_quote_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> borrow_rate_quote_;
    QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve_;
};

class EquityFutureInstrument final : public QuantLib::Instrument {
public:
    EquityFutureInstrument(
        double quantity,
        double contract_size,
        double reference_price,
        double direction_sign,
        QuantLib::Date maturity_date,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> future_quote,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> spot_quote,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> dividend_yield_quote,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> borrow_rate_quote,
        QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve)
        : quantity_(quantity),
          contract_size_(contract_size),
          reference_price_(reference_price),
          direction_sign_(direction_sign),
          maturity_date_(std::move(maturity_date)),
          future_quote_(std::move(future_quote)),
          spot_quote_(std::move(spot_quote)),
          dividend_yield_quote_(std::move(dividend_yield_quote)),
          borrow_rate_quote_(std::move(borrow_rate_quote)),
          discount_curve_(std::move(discount_curve)) {
        if (future_quote_) registerWith(future_quote_);
        if (spot_quote_) registerWith(spot_quote_);
        registerWith(dividend_yield_quote_);
        registerWith(borrow_rate_quote_);
        if (discount_curve_) registerWith(discount_curve_);
    }

    bool isExpired() const override {
        return maturity_date_ <= QuantLib::Settings::instance().evaluationDate();
    }

protected:
    void performCalculations() const override {
        const double market_future = future_quote_ ? future_quote_->value() : theoretical_future();
        const double discount = discount_factor_or_one(discount_curve_, maturity_date_);
        NPV_ = direction_sign_ * quantity_ * contract_size_ * (market_future - reference_price_) * discount;
        errorEstimate_ = 0.0;
        valuationDate_ = QuantLib::Settings::instance().evaluationDate();
    }

private:
    double theoretical_future() const {
        if (!spot_quote_) {
            return reference_price_;
        }
        const auto valuation_date = QuantLib::Settings::instance().evaluationDate();
        const QuantLib::Actual365Fixed day_count;
        const double time_to_maturity = std::max(day_count.yearFraction(valuation_date, maturity_date_), 0.0);
        const double discount = discount_factor_or_one(discount_curve_, maturity_date_);
        const double rate = time_to_maturity > 0.0 && discount > 0.0
            ? -std::log(discount) / time_to_maturity
            : 0.0;
        const double carry = rate + borrow_rate_quote_->value() - dividend_yield_quote_->value();
        return spot_quote_->value() * std::exp(carry * time_to_maturity);
    }

    double quantity_;
    double contract_size_;
    double reference_price_;
    double direction_sign_;
    QuantLib::Date maturity_date_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> future_quote_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> spot_quote_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> dividend_yield_quote_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> borrow_rate_quote_;
    QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve_;
};

class EquityOptionInstrument final : public QuantLib::Instrument {
public:
    EquityOptionInstrument(
        double quantity,
        double strike_price,
        double direction_sign,
        bool is_put,
        bool american,
        QuantLib::Date expiry_date,
        QuantLib::Date settlement_date,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> spot_quote,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> dividend_yield_quote,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> borrow_rate_quote,
        QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> volatility_quote,
        QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve)
        : quantity_(quantity),
          strike_price_(strike_price),
          direction_sign_(direction_sign),
          is_put_(is_put),
          american_(american),
          expiry_date_(std::move(expiry_date)),
          settlement_date_(std::move(settlement_date)),
          spot_quote_(std::move(spot_quote)),
          dividend_yield_quote_(std::move(dividend_yield_quote)),
          borrow_rate_quote_(std::move(borrow_rate_quote)),
          volatility_quote_(std::move(volatility_quote)),
          discount_curve_(std::move(discount_curve)) {
        registerWith(spot_quote_);
        registerWith(dividend_yield_quote_);
        registerWith(borrow_rate_quote_);
        registerWith(volatility_quote_);
        if (discount_curve_) registerWith(discount_curve_);
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
        const double discount = discount_factor_or_one(discount_curve_, settlement_date_);
        const double rate = time_to_expiry > 0.0 && discount > 0.0
            ? -std::log(discount) / time_to_expiry
            : 0.0;
        const double dividend_yield = dividend_yield_quote_->value();
        const double borrow_rate = borrow_rate_quote_->value();
        const double volatility = volatility_quote_->value();

        double unit_value = 0.0;
        if (american_) {
            unit_value = binomial_value(spot, rate, dividend_yield - borrow_rate, volatility, time_to_expiry);
        } else {
            unit_value = european_value(spot, rate, dividend_yield - borrow_rate, volatility, time_to_expiry);
        }

        NPV_ = direction_sign_ * quantity_ * unit_value;
        errorEstimate_ = 0.0;
        valuationDate_ = valuation_date;
    }

private:
    double intrinsic(double spot) const {
        return is_put_
            ? std::max(strike_price_ - spot, 0.0)
            : std::max(spot - strike_price_, 0.0);
    }

    double european_value(double spot, double rate, double effective_yield, double volatility, double time_to_expiry) const {
        if (time_to_expiry <= 0.0 || strike_price_ <= 0.0 || spot <= 0.0 || volatility <= 0.0) {
            return intrinsic(spot);
        }

        const double std_dev = std::max(volatility, 1.0e-8) * std::sqrt(time_to_expiry);
        const double d1 = (std::log(spot / strike_price_) +
                           (rate - effective_yield + 0.5 * volatility * volatility) * time_to_expiry) / std_dev;
        const double d2 = d1 - std_dev;
        const double df = std::exp(-rate * time_to_expiry);
        const double yield_df = std::exp(-effective_yield * time_to_expiry);
        return is_put_
            ? strike_price_ * df * normal_cdf(-d2) - spot * yield_df * normal_cdf(-d1)
            : spot * yield_df * normal_cdf(d1) - strike_price_ * df * normal_cdf(d2);
    }

    double binomial_value(double spot, double rate, double effective_yield, double volatility, double time_to_expiry) const {
        if (time_to_expiry <= 0.0 || strike_price_ <= 0.0 || spot <= 0.0 || volatility <= 0.0) {
            return intrinsic(spot);
        }

        constexpr int steps = 200;
        const double dt = time_to_expiry / static_cast<double>(steps);
        const double up = std::exp(std::max(volatility, 1.0e-8) * std::sqrt(dt));
        const double down = 1.0 / up;
        const double growth = std::exp((rate - effective_yield) * dt);
        const double probability = std::min(std::max((growth - down) / (up - down), 0.0), 1.0);
        const double step_discount = std::exp(-rate * dt);

        std::vector<double> values(static_cast<std::size_t>(steps) + 1U);
        for (int i = 0; i <= steps; ++i) {
            const double node_spot = spot * std::pow(up, i) * std::pow(down, steps - i);
            values[static_cast<std::size_t>(i)] = intrinsic(node_spot);
        }

        for (int step = steps - 1; step >= 0; --step) {
            for (int i = 0; i <= step; ++i) {
                const double continuation = step_discount *
                    (probability * values[static_cast<std::size_t>(i + 1)] +
                     (1.0 - probability) * values[static_cast<std::size_t>(i)]);
                const double node_spot = spot * std::pow(up, i) * std::pow(down, step - i);
                values[static_cast<std::size_t>(i)] = std::max(continuation, intrinsic(node_spot));
            }
        }
        return values.front();
    }

    double quantity_;
    double strike_price_;
    double direction_sign_;
    bool is_put_;
    bool american_;
    QuantLib::Date expiry_date_;
    QuantLib::Date settlement_date_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> spot_quote_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> dividend_yield_quote_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> borrow_rate_quote_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> volatility_quote_;
    QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve_;
};


} // namespace

QuantLib::ext::shared_ptr<QuantLib::Instrument> EquityInstrumentFactory::create_equity_spot(
    const domain::EquitySpotTrade& trade,
    const analytics::PricingContext& context) {
    auto quote = find_quote_handle(
        context,
        {},
        trade.underlier,
        "SPOT",
        {domain::QuoteInstrumentType::EquitySpot, domain::QuoteInstrumentType::Future});
    if (!quote) {
        return nullptr;
    }
    return QuantLib::ext::make_shared<QuantLib::Stock>(QuantLib::Handle<QuantLib::Quote>(quote));
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> EquityInstrumentFactory::create_equity_forward(
    const domain::EquityForwardTrade& trade,
    const analytics::PricingContext& context) {
    if (trade.maturity_date.empty()) {
        return nullptr;
    }

    auto spot_quote = find_quote_handle(
        context,
        trade.spot_quote_id,
        trade.underlier,
        "SPOT",
        {domain::QuoteInstrumentType::EquitySpot});
    if (!spot_quote) {
        return nullptr;
    }

    auto dividend_quote = quote_or_constant(
        context,
        trade.dividend_yield_quote_id,
        trade.underlier,
        {domain::QuoteInstrumentType::DividendYield},
        trade.dividend_yield);
    auto borrow_quote = quote_or_constant(
        context,
        trade.borrow_rate_quote_id,
        trade.underlier,
        {domain::QuoteInstrumentType::BorrowRate},
        trade.borrow_rate);
    const auto currency_code = domain::from_string(trade.currency);

    return QuantLib::ext::make_shared<EquityForwardInstrument>(
        trade.quantity,
        trade.forward_price,
        long_short_direction_sign(trade.direction),
        market::CurveBuilder::parse_date(trade.maturity_date),
        spot_quote,
        dividend_quote,
        borrow_quote,
        discount_curve(context, currency_code));
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> EquityInstrumentFactory::create_equity_future(
    const domain::EquityFutureTrade& trade,
    const analytics::PricingContext& context) {
    if (trade.maturity_date.empty()) {
        return nullptr;
    }

    auto future_quote = trade.future_quote_id.empty()
        ? nullptr
        : context.market_state().get_quote_handle(trade.future_quote_id);
    auto spot_quote = find_quote_handle(
        context,
        trade.spot_quote_id,
        trade.underlier,
        "SPOT",
        {domain::QuoteInstrumentType::EquitySpot});
    if (!future_quote && !spot_quote) {
        return nullptr;
    }

    auto dividend_quote = quote_or_constant(
        context,
        trade.dividend_yield_quote_id,
        trade.underlier,
        {domain::QuoteInstrumentType::DividendYield},
        trade.dividend_yield);
    auto borrow_quote = quote_or_constant(
        context,
        trade.borrow_rate_quote_id,
        trade.underlier,
        {domain::QuoteInstrumentType::BorrowRate},
        trade.borrow_rate);
    const auto currency_code = domain::from_string(trade.currency);

    return QuantLib::ext::make_shared<EquityFutureInstrument>(
        trade.quantity,
        trade.contract_size,
        trade.reference_price,
        long_short_direction_sign(trade.direction),
        market::CurveBuilder::parse_date(trade.maturity_date),
        future_quote,
        spot_quote,
        dividend_quote,
        borrow_quote,
        discount_curve(context, currency_code));
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> EquityInstrumentFactory::create_equity_option(
    const domain::EquityOptionTrade& trade,
    const analytics::PricingContext& context) {
    if (trade.expiry_date.empty()) {
        return nullptr;
    }

    auto spot_quote = find_quote_handle(
        context,
        trade.spot_quote_id,
        trade.underlier,
        "SPOT",
        {domain::QuoteInstrumentType::EquitySpot});
    if (!spot_quote) {
        return nullptr;
    }

    auto dividend_quote = quote_or_constant(
        context,
        trade.dividend_yield_quote_id,
        trade.underlier,
        {domain::QuoteInstrumentType::DividendYield},
        trade.dividend_yield);
    auto borrow_quote = quote_or_constant(
        context,
        trade.borrow_rate_quote_id,
        trade.underlier,
        {domain::QuoteInstrumentType::BorrowRate},
        trade.borrow_rate);
    auto volatility_quote = quote_or_constant(
        context,
        trade.volatility_quote_id,
        trade.underlier,
        {domain::QuoteInstrumentType::EquityVol},
        trade.volatility > 0.0 ? trade.volatility : 0.20);
    const auto expiry_date = market::CurveBuilder::parse_date(trade.expiry_date);
    const auto settlement_date = trade.settlement_date.empty()
        ? expiry_date
        : market::CurveBuilder::parse_date(trade.settlement_date);
    const auto currency_code = domain::from_string(trade.currency);

    return QuantLib::ext::make_shared<EquityOptionInstrument>(
        trade.quantity,
        trade.strike_price,
        long_short_direction_sign(trade.direction),
        is_put_option(trade.option_type),
        is_american_exercise(trade.exercise_style),
        expiry_date,
        settlement_date,
        spot_quote,
        dividend_quote,
        borrow_quote,
        volatility_quote,
        discount_curve(context, currency_code));
}

} // namespace qrp::instruments
