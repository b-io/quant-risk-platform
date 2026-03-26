#include <qrp/instruments/instrument_factory.hpp>
#include <qrp/conventions/market_convention_registry.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <ql/instruments/vanillaswap.hpp>
#include <ql/instruments/bonds/fixedratebond.hpp>
#include <ql/pricingengines/swap/discountingswapengine.hpp>
#include <ql/pricingengines/bond/discountingbondengine.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/time/daycounters/actual360.hpp>
#include <ql/time/daycounters/thirty360.hpp>
#include <ql/indexes/ibor/usdlibor.hpp>
#include <ql/indexes/ibor/euribor.hpp>
#include <ql/indexes/ibor/gbplibor.hpp>
#include <ql/indexes/ibor/chflibor.hpp>

namespace qrp::instruments {

QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_instrument(
    const domain::Trade& trade,
    const analytics::PricingContext& context) {

    if (trade.type == "vanilla_swap") {
        return create_swap(trade, context);
    } else if (trade.type == "fixed_rate_bond") {
        return create_bond(trade, context);
    }
    return nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_swap(
    const domain::Trade& trade,
    const analytics::PricingContext& context) {

    QuantLib::Date start = market::CurveBuilder::parse_date(trade.start_date);
    QuantLib::Date maturity = market::CurveBuilder::parse_date(trade.maturity_date);

    QuantLib::VanillaSwap::Type type = (trade.direction == "pay_fixed") ?
        QuantLib::VanillaSwap::Payer : QuantLib::VanillaSwap::Receiver;

    double fixed_rate = trade.details.at("fixed_rate").get<double>();
    std::string float_index_name = trade.details.contains("floating_index") ? 
        trade.details.at("floating_index").get<std::string>() : "IBOR_3M";

    domain::Currency cc = domain::from_string(trade.currency);
    auto& registry = conventions::MarketConventionRegistry::instance();
    auto conv = registry.get_rates_convention(cc, float_index_name);

    auto discount_curve_id = context.get_discount_curve_id(cc);
    auto forecast_curve_id = context.get_forecast_curve_id(cc, float_index_name);

    auto discount_curve = context.market_state().get_curve(discount_curve_id);
    auto forecast_curve = context.market_state().get_curve(forecast_curve_id);

    if (!discount_curve) return nullptr;
    // Fallback if specific forecast curve doesn't exist
    if (!forecast_curve) forecast_curve = discount_curve;
    
    QuantLib::Handle<QuantLib::YieldTermStructure> discounting_term_structure(discount_curve);
    QuantLib::Handle<QuantLib::YieldTermStructure> forecasting_term_structure(forecast_curve);

    QuantLib::Period index_tenor = market::CurveBuilder::parse_tenor(conv.index_family.substr(conv.index_family.find("_") + 1));
    if (index_tenor == QuantLib::Period(0, QuantLib::Days)) index_tenor = QuantLib::Period(3, QuantLib::Months);

    auto index = market::CurveBuilder::create_ibor_index(cc, index_tenor, forecasting_term_structure);

    auto calendar = market::CurveBuilder::parse_calendar(conv.calendar);
    auto bdc = market::CurveBuilder::parse_business_day_convention(conv.business_day_convention);
    auto fixed_freq = market::CurveBuilder::parse_frequency(conv.fixed_leg_frequency);
    auto fixed_dc = market::CurveBuilder::parse_day_count(conv.fixed_leg_day_count);
    auto float_freq = market::CurveBuilder::parse_frequency(conv.floating_leg_frequency);
    auto float_dc = market::CurveBuilder::parse_day_count(conv.day_count);

    auto swap = QuantLib::ext::shared_ptr<QuantLib::VanillaSwap>(new QuantLib::VanillaSwap(
        type, 
        trade.notional,
        QuantLib::Schedule(start, maturity, QuantLib::Period(fixed_freq), calendar,
                           bdc, bdc, QuantLib::DateGeneration::Forward, false),
        fixed_rate, 
        fixed_dc,
        QuantLib::Schedule(start, maturity, QuantLib::Period(float_freq), calendar,
                           bdc, bdc, QuantLib::DateGeneration::Forward, false),
        index, 
        0.0, // spread: usually zero for vanilla swaps
        float_dc));

    swap->setPricingEngine(QuantLib::ext::make_shared<QuantLib::DiscountingSwapEngine>(discounting_term_structure));
    return swap;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_bond(
    const domain::Trade& trade,
    const analytics::PricingContext& context) {

    QuantLib::Date start = market::CurveBuilder::parse_date(trade.start_date);
    QuantLib::Date maturity = market::CurveBuilder::parse_date(trade.maturity_date);

    double coupon_rate = trade.details.at("coupon_rate").get<double>();

    domain::Currency cc = domain::from_string(trade.currency);
    auto& registry = conventions::MarketConventionRegistry::instance();
    auto conv = registry.get_rates_convention(cc, "OIS"); // Default OIS for bond discounting if not specified

    auto curve_id = context.get_discount_curve_id(cc);
    auto curve = context.market_state().get_curve(curve_id);
    if (!curve) return nullptr;
    
    QuantLib::Handle<QuantLib::YieldTermStructure> discounting_term_structure(curve);

    auto calendar = market::CurveBuilder::parse_calendar(conv.calendar);
    auto bdc = market::CurveBuilder::parse_business_day_convention(conv.business_day_convention);
    auto freq = market::CurveBuilder::parse_frequency(domain::Frequency::Annual); // Bond default
    auto dc = market::CurveBuilder::parse_day_count(domain::DayCount::Thirty360);

    auto schedule = QuantLib::Schedule(start, maturity, QuantLib::Period(freq), calendar,
                                        bdc, bdc,
                                        QuantLib::DateGeneration::Backward, false);

    auto bond = QuantLib::ext::shared_ptr<QuantLib::FixedRateBond>(new QuantLib::FixedRateBond(
        0, trade.notional, schedule, std::vector<QuantLib::Rate>{coupon_rate},
        dc));

    bond->setPricingEngine(QuantLib::ext::make_shared<QuantLib::DiscountingBondEngine>(discounting_term_structure));
    return bond;
}

} // namespace qrp::instruments
