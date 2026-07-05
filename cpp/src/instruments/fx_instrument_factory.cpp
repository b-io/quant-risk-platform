// Implements FX instrument construction.

#include <qrp/instruments/instrument_factory.hpp>

#include <qrp/instruments/instrument_factory_common.hpp>


namespace qrp::instruments {
namespace {

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


} // namespace

QuantLib::ext::shared_ptr<QuantLib::Instrument> FxInstrumentFactory::create_fx_spot(
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

QuantLib::ext::shared_ptr<QuantLib::Instrument> FxInstrumentFactory::create_fx_forward(
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

QuantLib::ext::shared_ptr<QuantLib::Instrument> FxInstrumentFactory::create_fx_swap(
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

QuantLib::ext::shared_ptr<QuantLib::Instrument> FxInstrumentFactory::create_ndf(
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

QuantLib::ext::shared_ptr<QuantLib::Instrument> FxInstrumentFactory::create_fx_option(
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


} // namespace qrp::instruments
