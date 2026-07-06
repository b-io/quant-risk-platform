// Implements credit instrument construction.

#include <qrp/instruments/instrument_factory.hpp>
#include <qrp/instruments/instrument_factory_common.hpp>

namespace qrp::instruments {
namespace {

class CreditBondInstrument final : public QuantLib::Instrument {
public:
    CreditBondInstrument(double notional,
                         double coupon_rate,
                         double direction_sign,
                         QuantLib::Date maturity_date,
                         QuantLib::Schedule schedule,
                         QuantLib::DayCounter day_count,
                         QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve,
                         std::vector<CreditSpreadNode> spread_nodes)
        : notional_(notional), coupon_rate_(coupon_rate), direction_sign_(direction_sign),
          maturity_date_(std::move(maturity_date)), schedule_(std::move(schedule)), day_count_(std::move(day_count)),
          discount_curve_(std::move(discount_curve)), spread_nodes_(std::move(spread_nodes)) {
        registerWith(discount_curve_);
        for (const auto& node : spread_nodes_) {
            registerWith(node.quote);
        }
    }

    bool isExpired() const override {
        return maturity_date_ <= QuantLib::Settings::instance().evaluationDate();
    }

protected:
    void performCalculations() const override {
        const auto valuation_date = QuantLib::Settings::instance().evaluationDate();
        const auto& dates = schedule_.dates();

        double value = 0.0;
        for (std::size_t i = 1; i < dates.size(); ++i) {
            const auto& payment_date = dates[i];
            if (payment_date <= valuation_date) {
                continue;
            }

            const double accrual = day_count_.yearFraction(dates[i - 1], payment_date);
            const double time_to_payment = day_count_.yearFraction(valuation_date, payment_date);
            double cashflow = notional_ * coupon_rate_ * accrual;
            if (i + 1 == dates.size()) {
                cashflow += notional_;
            }
            value += cashflow * discount_factor_or_one(discount_curve_, payment_date) *
                     spread_discount_factor(spread_nodes_, time_to_payment);
        }

        NPV_ = direction_sign_ * value;
        errorEstimate_ = 0.0;
        valuationDate_ = valuation_date;
    }

private:
    double notional_;
    double coupon_rate_;
    double direction_sign_;
    QuantLib::Date maturity_date_;
    QuantLib::Schedule schedule_;
    QuantLib::DayCounter day_count_;
    QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve_;
    std::vector<CreditSpreadNode> spread_nodes_;
};

class CdsInstrument final : public QuantLib::Instrument {
public:
    CdsInstrument(double notional,
                  double coupon_rate,
                  double direction_sign,
                  QuantLib::Date maturity_date,
                  QuantLib::Schedule schedule,
                  QuantLib::DayCounter day_count,
                  QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve,
                  std::vector<CreditSpreadNode> spread_nodes,
                  QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> recovery_quote)
        : notional_(notional), coupon_rate_(coupon_rate), direction_sign_(direction_sign),
          maturity_date_(std::move(maturity_date)), schedule_(std::move(schedule)), day_count_(std::move(day_count)),
          discount_curve_(std::move(discount_curve)), spread_nodes_(std::move(spread_nodes)),
          recovery_quote_(std::move(recovery_quote)) {
        registerWith(discount_curve_);
        registerWith(recovery_quote_);
        for (const auto& node : spread_nodes_) {
            registerWith(node.quote);
        }
    }

    bool isExpired() const override {
        return maturity_date_ <= QuantLib::Settings::instance().evaluationDate();
    }

protected:
    void performCalculations() const override {
        const auto valuation_date = QuantLib::Settings::instance().evaluationDate();
        const double recovery = bounded_recovery(recovery_quote_->value());
        const auto& dates = schedule_.dates();

        double premium_annuity = 0.0;
        double protection_leg = 0.0;
        for (std::size_t i = 1; i < dates.size(); ++i) {
            const auto& accrual_start = dates[i - 1];
            const auto& payment_date = dates[i];
            if (payment_date <= valuation_date) {
                continue;
            }

            const double accrual = day_count_.yearFraction(accrual_start, payment_date);
            const double previous_time = std::max(day_count_.yearFraction(valuation_date, accrual_start), 0.0);
            const double payment_time = std::max(day_count_.yearFraction(valuation_date, payment_date), 0.0);
            const double previous_survival = survival_probability(spread_nodes_, recovery, previous_time);
            const double payment_survival = survival_probability(spread_nodes_, recovery, payment_time);
            const double discount = discount_factor_or_one(discount_curve_, payment_date);

            premium_annuity += accrual * discount * payment_survival;
            protection_leg += discount * std::max(previous_survival - payment_survival, 0.0);
        }

        NPV_ = direction_sign_ * notional_ * ((1.0 - recovery) * protection_leg - coupon_rate_ * premium_annuity);
        errorEstimate_ = 0.0;
        valuationDate_ = valuation_date;
    }

private:
    double notional_;
    double coupon_rate_;
    double direction_sign_;
    QuantLib::Date maturity_date_;
    QuantLib::Schedule schedule_;
    QuantLib::DayCounter day_count_;
    QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve_;
    std::vector<CreditSpreadNode> spread_nodes_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> recovery_quote_;
};

class CreditSpreadOptionInstrument final : public QuantLib::Instrument {
public:
    CreditSpreadOptionInstrument(double notional,
                                 double strike_spread,
                                 double direction_sign,
                                 double index_factor,
                                 bool is_put,
                                 QuantLib::Date expiry_date,
                                 QuantLib::Date maturity_date,
                                 QuantLib::Schedule schedule,
                                 QuantLib::DayCounter day_count,
                                 QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve,
                                 std::vector<CreditSpreadNode> spread_nodes,
                                 QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> recovery_quote,
                                 QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> volatility_quote)
        : notional_(notional), strike_spread_(strike_spread), direction_sign_(direction_sign),
          index_factor_(index_factor), is_put_(is_put), expiry_date_(std::move(expiry_date)),
          maturity_date_(std::move(maturity_date)), schedule_(std::move(schedule)), day_count_(std::move(day_count)),
          discount_curve_(std::move(discount_curve)), spread_nodes_(std::move(spread_nodes)),
          recovery_quote_(std::move(recovery_quote)), volatility_quote_(std::move(volatility_quote)) {
        registerWith(discount_curve_);
        registerWith(recovery_quote_);
        registerWith(volatility_quote_);
        for (const auto& node : spread_nodes_) {
            registerWith(node.quote);
        }
    }

    bool isExpired() const override {
        return expiry_date_ <= QuantLib::Settings::instance().evaluationDate();
    }

protected:
    void performCalculations() const override {
        const auto valuation_date = QuantLib::Settings::instance().evaluationDate();
        const double recovery = bounded_recovery(recovery_quote_->value());
        const double maturity_time = std::max(day_count_.yearFraction(valuation_date, maturity_date_), 1.0e-8);
        const double forward_spread = std::max(credit_spread_at(spread_nodes_, maturity_time), 1.0e-8);
        const double strike = std::max(strike_spread_, 1.0e-8);
        const double annuity = risky_annuity(valuation_date, recovery);

        const double expiry_time = day_count_.yearFraction(valuation_date, expiry_date_);
        double unit_value = 0.0;
        if (expiry_time <= 0.0 || volatility_quote_->value() <= 0.0) {
            unit_value = is_put_ ? std::max(strike - forward_spread, 0.0) : std::max(forward_spread - strike, 0.0);
        } else {
            const double volatility = std::max(volatility_quote_->value(), 1.0e-8);
            const double std_dev = volatility * std::sqrt(expiry_time);
            const double d1 =
                (std::log(forward_spread / strike) + 0.5 * volatility * volatility * expiry_time) / std_dev;
            const double d2 = d1 - std_dev;
            unit_value = is_put_ ? strike * normal_cdf(-d2) - forward_spread * normal_cdf(-d1)
                                 : forward_spread * normal_cdf(d1) - strike * normal_cdf(d2);
        }

        NPV_ = direction_sign_ * notional_ * index_factor_ * annuity * unit_value;
        errorEstimate_ = 0.0;
        valuationDate_ = valuation_date;
    }

private:
    double risky_annuity(const QuantLib::Date& valuation_date, double recovery) const {
        const auto& dates = schedule_.dates();
        double annuity = 0.0;
        for (std::size_t i = 1; i < dates.size(); ++i) {
            const auto& payment_date = dates[i];
            if (payment_date <= valuation_date) {
                continue;
            }
            const double accrual = day_count_.yearFraction(dates[i - 1], payment_date);
            const double payment_time = std::max(day_count_.yearFraction(valuation_date, payment_date), 0.0);
            annuity += accrual * discount_factor_or_one(discount_curve_, payment_date) *
                       survival_probability(spread_nodes_, recovery, payment_time);
        }
        return annuity;
    }

    double notional_;
    double strike_spread_;
    double direction_sign_;
    double index_factor_;
    bool is_put_;
    QuantLib::Date expiry_date_;
    QuantLib::Date maturity_date_;
    QuantLib::Schedule schedule_;
    QuantLib::DayCounter day_count_;
    QuantLib::ext::shared_ptr<QuantLib::YieldTermStructure> discount_curve_;
    std::vector<CreditSpreadNode> spread_nodes_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> recovery_quote_;
    QuantLib::ext::shared_ptr<QuantLib::SimpleQuote> volatility_quote_;
};

QuantLib::Schedule make_credit_schedule(const std::string& start_date,
                                        const std::string& maturity_date,
                                        domain::Currency currency,
                                        const std::string& frequency) {
    const auto convention = rates_convention(currency, "OIS");
    const auto calendar = market::CurveBuilder::parse_calendar(convention.calendar);
    const auto bdc = market::CurveBuilder::parse_business_day_convention(convention.business_day_convention);
    const auto parsed_frequency =
        market::CurveBuilder::parse_frequency(parse_frequency_string(frequency, domain::Frequency::Quarterly));
    return make_schedule(market::CurveBuilder::parse_date(start_date),
                         market::CurveBuilder::parse_date(maturity_date),
                         parsed_frequency,
                         calendar,
                         bdc,
                         QuantLib::DateGeneration::Backward);
}

} // namespace

QuantLib::ext::shared_ptr<QuantLib::Instrument>
CreditInstrumentFactory::create_credit_bond(const domain::CreditBondTrade& trade,
                                            const analytics::PricingContext& context) {
    const auto currency_code = domain::from_string(trade.currency);
    auto discount = discount_curve(context, currency_code);
    if (!discount) {
        return nullptr;
    }

    const auto maturity_date = market::CurveBuilder::parse_date(trade.maturity_date);
    const QuantLib::Actual365Fixed day_count;
    const double maturity_years =
        std::max(day_count.yearFraction(context.market_state().valuation_date(), maturity_date), 1.0e-8);
    auto spread_nodes = make_credit_spread_curve(context,
                                                 trade.issuer,
                                                 trade.spread_quote_id,
                                                 trade.credit_spread,
                                                 {domain::QuoteInstrumentType::BondSpread,
                                                  domain::QuoteInstrumentType::CDS,
                                                  domain::QuoteInstrumentType::CreditSpread},
                                                 maturity_years);

    return QuantLib::ext::make_shared<CreditBondInstrument>(
        trade.notional,
        trade.coupon_rate,
        credit_bond_direction_sign(trade.direction),
        maturity_date,
        make_credit_schedule(trade.start_date, trade.maturity_date, currency_code, trade.frequency),
        day_count,
        discount,
        std::move(spread_nodes));
}

QuantLib::ext::shared_ptr<QuantLib::Instrument>
CreditInstrumentFactory::create_cds(const domain::CdsTrade& trade, const analytics::PricingContext& context) {
    const auto currency_code = domain::from_string(trade.currency);
    auto discount = discount_curve(context, currency_code);
    if (!discount) {
        return nullptr;
    }

    const auto maturity_date = market::CurveBuilder::parse_date(trade.maturity_date);
    const QuantLib::Actual365Fixed day_count;
    const double maturity_years =
        std::max(day_count.yearFraction(context.market_state().valuation_date(), maturity_date), 1.0e-8);
    auto spread_nodes =
        make_credit_spread_curve(context,
                                 trade.issuer,
                                 trade.spread_quote_id,
                                 trade.coupon_rate,
                                 {domain::QuoteInstrumentType::CDS, domain::QuoteInstrumentType::CreditSpread},
                                 maturity_years);
    auto recovery_quote = make_recovery_quote(context, trade.issuer, trade.recovery_quote_id, trade.recovery_rate);

    return QuantLib::ext::make_shared<CdsInstrument>(
        trade.notional,
        trade.coupon_rate,
        credit_protection_direction_sign(trade.direction),
        maturity_date,
        make_credit_schedule(trade.start_date, trade.maturity_date, currency_code, trade.frequency),
        day_count,
        discount,
        std::move(spread_nodes),
        recovery_quote);
}

QuantLib::ext::shared_ptr<QuantLib::Instrument>
CreditInstrumentFactory::create_cds_index(const domain::CdsIndexTrade& trade,
                                          const analytics::PricingContext& context) {
    const auto currency_code = domain::from_string(trade.currency);
    auto discount = discount_curve(context, currency_code);
    if (!discount) {
        return nullptr;
    }

    const auto maturity_date = market::CurveBuilder::parse_date(trade.maturity_date);
    const QuantLib::Actual365Fixed day_count;
    const double maturity_years =
        std::max(day_count.yearFraction(context.market_state().valuation_date(), maturity_date), 1.0e-8);
    auto spread_nodes = make_credit_spread_curve(context,
                                                 trade.index_name,
                                                 trade.spread_quote_id,
                                                 trade.coupon_rate,
                                                 {domain::QuoteInstrumentType::CDS,
                                                  domain::QuoteInstrumentType::CreditIndex,
                                                  domain::QuoteInstrumentType::CreditSpread},
                                                 maturity_years);
    auto recovery_quote = make_recovery_quote(context, trade.index_name, trade.recovery_quote_id, trade.recovery_rate);

    return QuantLib::ext::make_shared<CdsInstrument>(
        trade.notional * trade.index_factor,
        trade.coupon_rate,
        credit_protection_direction_sign(trade.direction),
        maturity_date,
        make_credit_schedule(trade.start_date, trade.maturity_date, currency_code, trade.frequency),
        day_count,
        discount,
        std::move(spread_nodes),
        recovery_quote);
}

QuantLib::ext::shared_ptr<QuantLib::Instrument>
CreditInstrumentFactory::create_cds_option(const domain::CdsOptionTrade& trade,
                                           const analytics::PricingContext& context) {
    const auto currency_code = domain::from_string(trade.currency);
    auto discount = discount_curve(context, currency_code);
    if (!discount || trade.expiry_date.empty()) {
        return nullptr;
    }

    const auto maturity_date = market::CurveBuilder::parse_date(trade.maturity_date);
    const QuantLib::Actual365Fixed day_count;
    const double maturity_years =
        std::max(day_count.yearFraction(context.market_state().valuation_date(), maturity_date), 1.0e-8);
    auto spread_nodes =
        make_credit_spread_curve(context,
                                 trade.issuer,
                                 trade.spread_quote_id,
                                 trade.strike_spread,
                                 {domain::QuoteInstrumentType::CDS, domain::QuoteInstrumentType::CreditSpread},
                                 maturity_years);
    auto recovery_quote = make_recovery_quote(context, trade.issuer, trade.recovery_quote_id, trade.recovery_rate);
    auto volatility_quote = trade.volatility_quote_id.empty()
                                ? nullptr
                                : context.market_state().get_quote_handle(trade.volatility_quote_id);
    if (!volatility_quote) {
        volatility_quote =
            QuantLib::ext::make_shared<QuantLib::SimpleQuote>(trade.volatility > 0.0 ? trade.volatility : 0.35);
    }

    return QuantLib::ext::make_shared<CreditSpreadOptionInstrument>(
        trade.notional,
        trade.strike_spread,
        credit_option_direction_sign(trade.direction),
        1.0,
        is_credit_put_option(trade.option_type),
        market::CurveBuilder::parse_date(trade.expiry_date),
        maturity_date,
        make_credit_schedule(trade.expiry_date, trade.maturity_date, currency_code, trade.frequency),
        day_count,
        discount,
        std::move(spread_nodes),
        recovery_quote,
        volatility_quote);
}

QuantLib::ext::shared_ptr<QuantLib::Instrument>
CreditInstrumentFactory::create_credit_index_option(const domain::CreditIndexOptionTrade& trade,
                                                    const analytics::PricingContext& context) {
    const auto currency_code = domain::from_string(trade.currency);
    auto discount = discount_curve(context, currency_code);
    if (!discount || trade.expiry_date.empty()) {
        return nullptr;
    }

    const auto maturity_date = market::CurveBuilder::parse_date(trade.maturity_date);
    const QuantLib::Actual365Fixed day_count;
    const double maturity_years =
        std::max(day_count.yearFraction(context.market_state().valuation_date(), maturity_date), 1.0e-8);
    auto spread_nodes = make_credit_spread_curve(context,
                                                 trade.index_name,
                                                 trade.spread_quote_id,
                                                 trade.strike_spread,
                                                 {domain::QuoteInstrumentType::CDS,
                                                  domain::QuoteInstrumentType::CreditIndex,
                                                  domain::QuoteInstrumentType::CreditSpread},
                                                 maturity_years);
    auto recovery_quote = make_recovery_quote(context, trade.index_name, trade.recovery_quote_id, trade.recovery_rate);
    auto volatility_quote = trade.volatility_quote_id.empty()
                                ? nullptr
                                : context.market_state().get_quote_handle(trade.volatility_quote_id);
    if (!volatility_quote) {
        volatility_quote =
            QuantLib::ext::make_shared<QuantLib::SimpleQuote>(trade.volatility > 0.0 ? trade.volatility : 0.30);
    }

    return QuantLib::ext::make_shared<CreditSpreadOptionInstrument>(
        trade.notional,
        trade.strike_spread,
        credit_option_direction_sign(trade.direction),
        trade.index_factor,
        is_credit_put_option(trade.option_type),
        market::CurveBuilder::parse_date(trade.expiry_date),
        maturity_date,
        make_credit_schedule(trade.expiry_date, trade.maturity_date, currency_code, trade.frequency),
        day_count,
        discount,
        std::move(spread_nodes),
        recovery_quote,
        volatility_quote);
}

} // namespace qrp::instruments
