// Implements commodity instrument construction.

#include <qrp/instruments/instrument_factory.hpp>
#include <qrp/instruments/instrument_factory_common.hpp>

namespace qrp::instruments {
namespace {

class PriceMtmInstrument final : public QuantLib::Instrument {
public:
    PriceMtmInstrument(double quantity,
                       double contract_size,
                       double reference_price,
                       double direction_sign,
                       QuantLib::Date maturity_date,
                       QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> price_quote,
                       QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve,
                       bool expires)
        : quantity_(quantity), contract_size_(contract_size), reference_price_(reference_price),
          direction_sign_(direction_sign), maturity_date_(std::move(maturity_date)),
          price_quote_(std::move(price_quote)), discount_curve_(std::move(discount_curve)), expires_(expires) {
        registerWith(price_quote_);
        if (discount_curve_)
            registerWith(discount_curve_);
    }

    bool isExpired() const override {
        return expires_ && maturity_date_ <= QuantLib::Settings::instance().evaluationDate();
    }

protected:
    void performCalculations() const override {
        const double discount = expires_ ? discount_factor_or_one(discount_curve_, maturity_date_) : 1.0;
        NPV_ = direction_sign_ * quantity_ * contract_size_ * (price_quote_->value() - reference_price_) * discount;
        errorEstimate_ = 0.0;
        valuationDate_ = QuantLib::Settings::instance().evaluationDate();
    }

private:
    double quantity_;
    double contract_size_;
    double reference_price_;
    double direction_sign_;
    QuantLib::Date maturity_date_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> price_quote_;
    QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve_;
    bool expires_;
};

class StripMtmInstrument final : public QuantLib::Instrument {
public:
    StripMtmInstrument(double quantity,
                       double contract_size,
                       double reference_price,
                       double direction_sign,
                       QuantLib::Date maturity_date,
                       std::vector<QuantLib::ext::shared_ptr<QuantLib::SimpleQuote>> price_quotes,
                       std::vector<double> weights,
                       QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve)
        : quantity_(quantity), contract_size_(contract_size), reference_price_(reference_price),
          direction_sign_(direction_sign), maturity_date_(std::move(maturity_date)),
          price_quotes_(std::move(price_quotes)), weights_(std::move(weights)),
          discount_curve_(std::move(discount_curve)) {
        for (const auto& quote : price_quotes_) {
            registerWith(quote);
        }
        if (discount_curve_)
            registerWith(discount_curve_);
    }

    bool isExpired() const override {
        return maturity_date_ <= QuantLib::Settings::instance().evaluationDate();
    }

protected:
    void performCalculations() const override {
        double weighted_sum = 0.0;
        double total_weight = 0.0;
        for (std::size_t i = 0; i < price_quotes_.size(); ++i) {
            const double weight = i < weights_.size() ? weights_[i] : 1.0;
            weighted_sum += weight * price_quotes_[i]->value();
            total_weight += weight;
        }

        const double strip_price = total_weight > 0.0 ? weighted_sum / total_weight : 0.0;
        const double discount = discount_factor_or_one(discount_curve_, maturity_date_);
        NPV_ = direction_sign_ * quantity_ * contract_size_ * (strip_price - reference_price_) * discount;
        errorEstimate_ = 0.0;
        valuationDate_ = QuantLib::Settings::instance().evaluationDate();
    }

private:
    double quantity_;
    double contract_size_;
    double reference_price_;
    double direction_sign_;
    QuantLib::Date maturity_date_;
    std::vector<QuantLib::ext::shared_ptr<QuantLib::SimpleQuote>> price_quotes_;
    std::vector<double> weights_;
    QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve_;
};

class BlackFutureOptionInstrument final : public QuantLib::Instrument {
public:
    BlackFutureOptionInstrument(double quantity,
                                double contract_size,
                                double strike_price,
                                double direction_sign,
                                bool is_put,
                                QuantLib::Date expiry_date,
                                QuantLib::Date settlement_date,
                                QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> future_quote,
                                QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> volatility_quote,
                                QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve)
        : quantity_(quantity), contract_size_(contract_size), strike_price_(strike_price),
          direction_sign_(direction_sign), is_put_(is_put), expiry_date_(std::move(expiry_date)),
          settlement_date_(std::move(settlement_date)), future_quote_(std::move(future_quote)),
          volatility_quote_(std::move(volatility_quote)), discount_curve_(std::move(discount_curve)) {
        registerWith(future_quote_);
        registerWith(volatility_quote_);
        if (discount_curve_)
            registerWith(discount_curve_);
    }

    bool isExpired() const override {
        return expiry_date_ <= QuantLib::Settings::instance().evaluationDate();
    }

protected:
    void performCalculations() const override {
        const auto valuation_date = QuantLib::Settings::instance().evaluationDate();
        const QuantLib::Actual365Fixed day_count;
        const double time_to_expiry = day_count.yearFraction(valuation_date, expiry_date_);
        const double future_price = future_quote_->value();
        const double discount = discount_factor_or_one(discount_curve_, settlement_date_);

        double unit_value = 0.0;
        if (time_to_expiry <= 0.0 || strike_price_ <= 0.0 || volatility_quote_->value() <= 0.0) {
            const double intrinsic_value =
                is_put_ ? std::max(strike_price_ - future_price, 0.0) : std::max(future_price - strike_price_, 0.0);
            unit_value = discount * intrinsic_value;
        } else {
            const double volatility = std::max(volatility_quote_->value(), 1.0e-8);
            const double std_dev = volatility * std::sqrt(time_to_expiry);
            const double d1 = (std::log(std::max(future_price, 1.0e-8) / strike_price_) +
                               0.5 * volatility * volatility * time_to_expiry) /
                              std_dev;
            const double d2 = d1 - std_dev;
            unit_value = is_put_ ? discount * (strike_price_ * normal_cdf(-d2) - future_price * normal_cdf(-d1))
                                 : discount * (future_price * normal_cdf(d1) - strike_price_ * normal_cdf(d2));
        }

        NPV_ = direction_sign_ * quantity_ * contract_size_ * unit_value;
        errorEstimate_ = 0.0;
        valuationDate_ = valuation_date;
    }

private:
    double quantity_;
    double contract_size_;
    double strike_price_;
    double direction_sign_;
    bool is_put_;
    QuantLib::Date expiry_date_;
    QuantLib::Date settlement_date_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> future_quote_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> volatility_quote_;
    QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve_;
};

class CalendarSpreadOptionInstrument final : public QuantLib::Instrument {
public:
    CalendarSpreadOptionInstrument(double quantity,
                                   double contract_size,
                                   double strike_spread,
                                   double direction_sign,
                                   bool is_put,
                                   QuantLib::Date expiry_date,
                                   QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> near_quote,
                                   QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> far_quote,
                                   QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> volatility_quote,
                                   QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve)
        : quantity_(quantity), contract_size_(contract_size), strike_spread_(strike_spread),
          direction_sign_(direction_sign), is_put_(is_put), expiry_date_(std::move(expiry_date)),
          near_quote_(std::move(near_quote)), far_quote_(std::move(far_quote)),
          volatility_quote_(std::move(volatility_quote)), discount_curve_(std::move(discount_curve)) {
        registerWith(near_quote_);
        registerWith(far_quote_);
        registerWith(volatility_quote_);
        if (discount_curve_)
            registerWith(discount_curve_);
    }

    bool isExpired() const override {
        return expiry_date_ <= QuantLib::Settings::instance().evaluationDate();
    }

protected:
    void performCalculations() const override {
        const auto valuation_date = QuantLib::Settings::instance().evaluationDate();
        const QuantLib::Actual365Fixed day_count;
        const double time_to_expiry = std::max(day_count.yearFraction(valuation_date, expiry_date_), 0.0);
        const double spread = far_quote_->value() - near_quote_->value();
        const double discount = discount_factor_or_one(discount_curve_, expiry_date_);
        const double scale = 0.5 * (std::abs(far_quote_->value()) + std::abs(near_quote_->value()));
        const double normal_vol = std::max(volatility_quote_->value() * std::max(scale, 1.0), 1.0e-8);
        const double std_dev = normal_vol * std::sqrt(std::max(time_to_expiry, 1.0e-8));

        double unit_value = 0.0;
        if (time_to_expiry <= 0.0 || volatility_quote_->value() <= 0.0) {
            unit_value = is_put_ ? std::max(strike_spread_ - spread, 0.0) : std::max(spread - strike_spread_, 0.0);
        } else {
            const double d = (spread - strike_spread_) / std_dev;
            unit_value = is_put_ ? (strike_spread_ - spread) * normal_cdf(-d) + std_dev * normal_pdf(d)
                                 : (spread - strike_spread_) * normal_cdf(d) + std_dev * normal_pdf(d);
        }

        NPV_ = direction_sign_ * quantity_ * contract_size_ * discount * unit_value;
        errorEstimate_ = 0.0;
        valuationDate_ = valuation_date;
    }

private:
    double quantity_;
    double contract_size_;
    double strike_spread_;
    double direction_sign_;
    bool is_put_;
    QuantLib::Date expiry_date_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> near_quote_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> far_quote_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> volatility_quote_;
    QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve_;
};

class CommoditySwingInstrument final : public QuantLib::Instrument {
public:
    CommoditySwingInstrument(double min_quantity,
                             double max_quantity,
                             double strike_price,
                             double volatility,
                             double direction_sign,
                             QuantLib::Date maturity_date,
                             std::vector<QuantLib::ext::shared_ptr<QuantLib::SimpleQuote>> forward_quotes,
                             QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve)
        : min_quantity_(min_quantity), max_quantity_(max_quantity), strike_price_(strike_price),
          volatility_(volatility), direction_sign_(direction_sign), maturity_date_(std::move(maturity_date)),
          forward_quotes_(std::move(forward_quotes)), discount_curve_(std::move(discount_curve)) {
        for (const auto& quote : forward_quotes_) {
            registerWith(quote);
        }
        if (discount_curve_)
            registerWith(discount_curve_);
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
        const double optional_quantity = std::max(max_quantity_ - min_quantity_, 0.0);

        double average_forward = 0.0;
        double average_intrinsic = 0.0;
        for (const auto& quote : forward_quotes_) {
            const double forward = quote->value();
            average_forward += forward;
            average_intrinsic += std::max(forward - strike_price_, 0.0);
        }
        average_forward /= static_cast<double>(forward_quotes_.size());
        average_intrinsic /= static_cast<double>(forward_quotes_.size());

        const double committed_value = min_quantity_ * (average_forward - strike_price_);
        const double optionality_uplift = optional_quantity * 0.5 * std::max(volatility_, 0.0) *
                                          std::max(average_forward, 1.0) * std::sqrt(std::max(time_to_maturity, 0.0));
        const double flexible_value = optional_quantity * average_intrinsic + optionality_uplift;

        NPV_ = direction_sign_ * discount * (committed_value + flexible_value);
        errorEstimate_ = 0.0;
        valuationDate_ = valuation_date;
    }

private:
    double min_quantity_;
    double max_quantity_;
    double strike_price_;
    double volatility_;
    double direction_sign_;
    QuantLib::Date maturity_date_;
    std::vector<QuantLib::ext::shared_ptr<QuantLib::SimpleQuote>> forward_quotes_;
    QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve_;
};

} // namespace

QuantLib::ext::shared_ptr<QuantLib::Instrument>
CommodityInstrumentFactory::create_commodity_spot(const domain::CommoditySpotTrade& trade,
                                                  const analytics::PricingContext& context) {
    auto quote =
        find_quote_handle(context,
                          trade.spot_quote_id,
                          trade.underlier,
                          "SPOT",
                          {domain::QuoteInstrumentType::CommoditySpot, domain::QuoteInstrumentType::CommodityForward});
    if (!quote) {
        return nullptr;
    }

    return QuantLib::ext::make_shared<PriceMtmInstrument>(trade.quantity,
                                                          1.0,
                                                          trade.reference_price,
                                                          long_short_direction_sign(trade.direction),
                                                          QuantLib::Date(),
                                                          quote,
                                                          nullptr,
                                                          false);
}

QuantLib::ext::shared_ptr<QuantLib::Instrument>
CommodityInstrumentFactory::create_commodity_forward(const domain::CommodityForwardTrade& trade,
                                                     const analytics::PricingContext& context) {
    if (trade.maturity_date.empty()) {
        return nullptr;
    }

    auto quote = find_quote_handle(
        context,
        trade.forward_quote_id,
        trade.underlier,
        trade.tenor,
        {domain::QuoteInstrumentType::CommodityForward, domain::QuoteInstrumentType::CommodityFuture});
    if (!quote) {
        quote = find_quote_handle(context,
                                  trade.spot_quote_id,
                                  trade.underlier,
                                  "SPOT",
                                  {domain::QuoteInstrumentType::CommoditySpot});
    }
    if (!quote) {
        return nullptr;
    }

    const auto currency_code = domain::from_string(trade.currency);
    return QuantLib::ext::make_shared<PriceMtmInstrument>(trade.quantity,
                                                          1.0,
                                                          trade.contract_price,
                                                          long_short_direction_sign(trade.direction),
                                                          market::CurveBuilder::parse_date(trade.maturity_date),
                                                          quote,
                                                          discount_curve(context, currency_code),
                                                          true);
}

QuantLib::ext::shared_ptr<QuantLib::Instrument>
CommodityInstrumentFactory::create_commodity_future(const domain::CommodityFutureTrade& trade,
                                                    const analytics::PricingContext& context) {
    if (trade.maturity_date.empty()) {
        return nullptr;
    }

    auto quote = find_quote_handle(context,
                                   trade.future_quote_id,
                                   trade.underlier,
                                   trade.tenor,
                                   {domain::QuoteInstrumentType::CommodityFuture, domain::QuoteInstrumentType::Future});
    if (!quote) {
        return nullptr;
    }

    return QuantLib::ext::make_shared<PriceMtmInstrument>(trade.quantity,
                                                          trade.contract_size,
                                                          trade.reference_price,
                                                          long_short_direction_sign(trade.direction),
                                                          market::CurveBuilder::parse_date(trade.maturity_date),
                                                          quote,
                                                          nullptr,
                                                          true);
}

QuantLib::ext::shared_ptr<QuantLib::Instrument>
CommodityInstrumentFactory::create_commodity_future_strip(const domain::CommodityFutureStripTrade& trade,
                                                          const analytics::PricingContext& context) {
    if (trade.maturity_date.empty() || trade.future_quote_ids.empty()) {
        return nullptr;
    }

    std::vector<QuantLib::ext::shared_ptr<QuantLib::SimpleQuote>> quotes;
    for (const auto& quote_id : trade.future_quote_ids) {
        if (auto quote = context.market_state().get_quote_handle(quote_id)) {
            quotes.push_back(quote);
        }
    }
    if (quotes.empty()) {
        return nullptr;
    }

    const auto currency_code = domain::from_string(trade.currency);
    return QuantLib::ext::make_shared<StripMtmInstrument>(trade.quantity,
                                                          trade.contract_size,
                                                          trade.reference_price,
                                                          long_short_direction_sign(trade.direction),
                                                          market::CurveBuilder::parse_date(trade.maturity_date),
                                                          std::move(quotes),
                                                          trade.weights,
                                                          discount_curve(context, currency_code));
}

QuantLib::ext::shared_ptr<QuantLib::Instrument>
CommodityInstrumentFactory::create_commodity_future_option(const domain::CommodityFutureOptionTrade& trade,
                                                           const analytics::PricingContext& context) {
    if (trade.expiry_date.empty()) {
        return nullptr;
    }

    auto future_quote = find_quote_handle(context,
                                          trade.future_quote_id,
                                          trade.underlier,
                                          trade.tenor,
                                          {domain::QuoteInstrumentType::CommodityFuture,
                                           domain::QuoteInstrumentType::CommodityForward,
                                           domain::QuoteInstrumentType::Future});
    if (!future_quote) {
        return nullptr;
    }

    auto volatility_quote = quote_or_constant(context,
                                              trade.volatility_quote_id,
                                              trade.underlier,
                                              {domain::QuoteInstrumentType::CommodityVol},
                                              trade.volatility > 0.0 ? trade.volatility : 0.30);
    const auto expiry_date = market::CurveBuilder::parse_date(trade.expiry_date);
    const auto settlement_date =
        trade.maturity_date.empty() ? expiry_date : market::CurveBuilder::parse_date(trade.maturity_date);
    const auto currency_code = domain::from_string(trade.currency);

    return QuantLib::ext::make_shared<BlackFutureOptionInstrument>(trade.quantity,
                                                                   trade.contract_size,
                                                                   trade.strike_price,
                                                                   long_short_direction_sign(trade.direction),
                                                                   is_put_option(trade.option_type),
                                                                   expiry_date,
                                                                   settlement_date,
                                                                   future_quote,
                                                                   volatility_quote,
                                                                   discount_curve(context, currency_code));
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> CommodityInstrumentFactory::create_commodity_calendar_spread_option(
    const domain::CommodityCalendarSpreadOptionTrade& trade,
    const analytics::PricingContext& context) {
    if (trade.expiry_date.empty()) {
        return nullptr;
    }

    auto near_quote = context.market_state().get_quote_handle(trade.near_future_quote_id);
    auto far_quote = context.market_state().get_quote_handle(trade.far_future_quote_id);
    if (!near_quote || !far_quote) {
        return nullptr;
    }

    auto volatility_quote = quote_or_constant(context,
                                              trade.volatility_quote_id,
                                              trade.underlier,
                                              {domain::QuoteInstrumentType::CommodityVol},
                                              trade.volatility > 0.0 ? trade.volatility : 0.20);
    const auto currency_code = domain::from_string(trade.currency);

    return QuantLib::ext::make_shared<CalendarSpreadOptionInstrument>(
        trade.quantity,
        trade.contract_size,
        trade.strike_spread,
        long_short_direction_sign(trade.direction),
        is_put_option(trade.option_type),
        market::CurveBuilder::parse_date(trade.expiry_date),
        near_quote,
        far_quote,
        volatility_quote,
        discount_curve(context, currency_code));
}

QuantLib::ext::shared_ptr<QuantLib::Instrument>
CommodityInstrumentFactory::create_commodity_swing(const domain::CommoditySwingTrade& trade,
                                                   const analytics::PricingContext& context) {
    if (trade.maturity_date.empty()) {
        return nullptr;
    }

    std::vector<QuantLib::ext::shared_ptr<QuantLib::SimpleQuote>> quotes;
    for (const auto& quote_id : trade.forward_quote_ids) {
        if (auto quote = context.market_state().get_quote_handle(quote_id)) {
            quotes.push_back(quote);
        }
    }
    if (quotes.empty()) {
        if (auto quote = find_quote_handle(
                context,
                {},
                trade.underlier,
                {},
                {domain::QuoteInstrumentType::CommodityForward, domain::QuoteInstrumentType::CommodityFuture})) {
            quotes.push_back(quote);
        }
    }
    if (quotes.empty()) {
        return nullptr;
    }

    const auto currency_code = domain::from_string(trade.currency);
    return QuantLib::ext::make_shared<CommoditySwingInstrument>(trade.min_quantity,
                                                                trade.max_quantity,
                                                                trade.strike_price,
                                                                trade.volatility,
                                                                long_short_direction_sign(trade.direction),
                                                                market::CurveBuilder::parse_date(trade.maturity_date),
                                                                std::move(quotes),
                                                                discount_curve(context, currency_code));
}

} // namespace qrp::instruments
