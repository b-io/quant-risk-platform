// Implements translation from domain trades into QuantLib instruments and pricing engines.

#include <qrp/instruments/instrument_factory.hpp>
#include <qrp/analytics/product_pricing_registry.hpp>
#include <qrp/conventions/market_convention_registry.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <ql/instruments/vanillaswap.hpp>
#include <ql/instruments/bonds/fixedratebond.hpp>
#include <ql/instruments/forward.hpp>
#include <ql/instruments/stock.hpp>
#include <ql/pricingengines/swap/discountingswapengine.hpp>
#include <ql/pricingengines/bond/discountingbondengine.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/time/daycounters/actual360.hpp>
#include <ql/time/daycounters/thirty360.hpp>
#include <ql/indexes/ibor/usdlibor.hpp>
#include <ql/indexes/ibor/euribor.hpp>
#include <ql/indexes/ibor/gbplibor.hpp>
#include <ql/indexes/ibor/chflibor.hpp>
#include <stdexcept>

/**
 * @brief InstrumentFactory is the central factory for translating domain-level Trades into QuantLib Instruments.
 *
 * Design choices (see docs/design/ARCHITECTURE.md):
 * 1. Abstraction: It decouples the engine's internal representation (domain::Trade) from QuantLib's pricing classes.
 * 2. Context-Aware: Uses PricingContext to resolve the correct curves (discounting/forecasting) based on conventions.
 * 3. QuantLib usage:
 *    - VanillaSwap: Standard floating vs fixed interest rate swap.
 *    - FixedRateBond: Standard bond with fixed coupons.
 *    - DiscountingSwapEngine / DiscountingBondEngine: Preferred for deterministic valuation over Monte Carlo or Tree engines.
 * 4. Multi-curve: Supports OIS discounting and IBOR forecasting by linking different YieldTermStructure handles to different legs.
 */
namespace qrp::instruments {

/**
 * @brief Dispatches trade creation to specialized creators based on trade type.
 */
QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_instrument(
    const domain::Trade& trade,
    const analytics::PricingContext& context) {
    return analytics::ProductPricingRegistry::create_instrument(trade, context);
}

/**
 * @brief Creates a QuantLib::VanillaSwap.
 * 
 * Tradeoffs: 
 * - Using VanillaSwap is simpler but less flexible than the generic Swap class. 
 * - It covers 90% of vanilla rates trading needs while providing a clean interface for NPV and fair rate.
 */
QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_swap(
    const domain::VanillaSwapTrade& trade,
    const analytics::PricingContext& context) {

    QuantLib::Date start = market::CurveBuilder::parse_date(trade.start_date);
    QuantLib::Date maturity = market::CurveBuilder::parse_date(trade.maturity_date);

    QuantLib::VanillaSwap::Type type = (trade.direction == "pay_fixed") ?
        QuantLib::VanillaSwap::Payer : QuantLib::VanillaSwap::Receiver;

    double fixed_rate = trade.fixed_rate;
    std::string float_index_name = trade.floating_index;
    
    // Normalize index name for convention lookup
    std::string lookup_index = float_index_name;
    if (lookup_index == "USD_LIBOR_3M" || lookup_index == "EURIBOR_3M") lookup_index = "IBOR_3M";
    if (lookup_index == "USD_OIS" || lookup_index == "SOFR") lookup_index = "OIS";

    domain::Currency cc = domain::from_string(trade.currency);
    auto& registry = conventions::MarketConventionRegistry::instance();
    auto conv = registry.get_rates_convention(cc, lookup_index);

    // Resolve curve handles from the pricing context.
    auto discount_curve_id = context.get_discount_curve_id(cc);
    auto forecast_curve_id = context.get_forecast_curve_id(cc, lookup_index);

    auto discount_curve = context.market_state().get_curve(discount_curve_id);
    auto forecast_curve = context.market_state().get_curve(forecast_curve_id);

    if (!discount_curve) {
        return nullptr;
    }
    // Fallback: If no specific forecast curve exists, we use the discount curve (single-curve pricing).
    if (!forecast_curve) forecast_curve = discount_curve;
    
    QuantLib::Handle<QuantLib::YieldTermStructure> discounting_term_structure(discount_curve);
    QuantLib::Handle<QuantLib::YieldTermStructure> forecasting_term_structure(forecast_curve);

    // Determine the index tenor from the convention (e.g., "IBOR_3M" -> 3 months).
    size_t underscore_pos = conv.index_family.find("_");
    QuantLib::Period index_tenor(3, QuantLib::Months);
    if (underscore_pos != std::string::npos && underscore_pos + 1 < conv.index_family.size()) {
        try {
            index_tenor = market::CurveBuilder::parse_tenor(conv.index_family.substr(underscore_pos + 1));
        } catch (const std::invalid_argument&) {
            index_tenor = QuantLib::Period(3, QuantLib::Months);
        }
    }

    // Create the floating rate index linked to the forecast curve.
    auto index = market::CurveBuilder::create_ibor_index(cc, index_tenor, forecasting_term_structure);

    // Apply fixings from context if any
    for (const auto& [index_name, date_fixings] : context.market_state().fixings()) {
        // Accept convention-level, generic, and trade-level aliases used by imported market data.
        if (index_name == conv.index_family || index_name == "IBOR_3M" || index_name == trade.floating_index) {
            for (const auto& [date, value] : date_fixings) {
                index->addFixing(date, value, true); // forceOverwrite=true
            }
        }
    }

    auto calendar = market::CurveBuilder::parse_calendar(conv.calendar);
    auto bdc = market::CurveBuilder::parse_business_day_convention(conv.business_day_convention);
    auto fixed_freq = market::CurveBuilder::parse_frequency(conv.fixed_leg_frequency);
    auto fixed_dc = market::CurveBuilder::parse_day_count(conv.fixed_leg_day_count);
    auto float_freq = market::CurveBuilder::parse_frequency(conv.floating_leg_frequency);
    auto float_dc = market::CurveBuilder::parse_day_count(conv.day_count);

    // Build schedules for fixed and floating legs.
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

    // Link the swap to the discounting engine.
    swap->setPricingEngine(QuantLib::ext::make_shared<QuantLib::DiscountingSwapEngine>(discounting_term_structure));
    return swap;
}

/**
 * @brief Creates a QuantLib::FixedRateBond.
 */
QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_bond(
    const domain::FixedRateBondTrade& trade,
    const analytics::PricingContext& context) {

    QuantLib::Date start = market::CurveBuilder::parse_date(trade.start_date);
    QuantLib::Date maturity = market::CurveBuilder::parse_date(trade.maturity_date);

    double coupon_rate = trade.coupon_rate;

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

/**
 * @brief Creates an FX Forward.
 */
QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_fx_forward(
    const domain::FxForwardTrade& trade,
    const analytics::PricingContext& context) {
    
    // FX spot quote IDs use the concatenated base/quote pair convention, e.g. EURUSD.
    std::string pair = trade.base_currency + trade.quote_currency;
    auto spot_quote = context.market_state().get_quote_handle(pair);
    
    if (!spot_quote) return nullptr;
    
    // We return a Stock instrument tied to the FX spot.
    // The NPV will be handled by ValuationService/RiskService by applying the forward rate offset and notional.
    return QuantLib::ext::make_shared<QuantLib::Stock>(QuantLib::Handle<QuantLib::Quote>(spot_quote));
}

/**
 * @brief Creates an equity spot instrument linked to the underlier quote.
 */
QuantLib::ext::shared_ptr<QuantLib::Instrument> InstrumentFactory::create_equity_spot(
    const domain::EquitySpotTrade& trade,
    const analytics::PricingContext& context) {
    
    auto quote = context.market_state().get_quote_handle(trade.underlier);
    if (!quote) return nullptr;
    
    return QuantLib::ext::make_shared<QuantLib::Stock>(QuantLib::Handle<QuantLib::Quote>(quote));
}

} // namespace qrp::instruments
