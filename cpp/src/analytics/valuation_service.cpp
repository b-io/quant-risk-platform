#include <qrp/analytics/valuation_service.hpp>
#include <qrp/instruments/instrument_factory.hpp>

namespace qrp::analytics {

InstrumentPricingProfile ValuationService::pricing_profile(const domain::Trade& trade) {
    InstrumentPricingProfile profile;

    switch (trade.trade_type) {
        case domain::TradeType::EquitySpot: {
            const auto& eq_trade = dynamic_cast<const domain::EquitySpotTrade&>(trade);
            profile.multiplier = eq_trade.quantity;
            profile.additive_npv = -eq_trade.reference_price * eq_trade.quantity;
            break;
        }
        case domain::TradeType::FxForward: {
            const auto& fx_trade = dynamic_cast<const domain::FxForwardTrade&>(trade);
            profile.multiplier = fx_trade.notional;
            profile.additive_npv = -fx_trade.forward_rate * fx_trade.notional;
            break;
        }
        case domain::TradeType::VanillaSwap:
        case domain::TradeType::FixedRateBond:
        case domain::TradeType::Unknown:
            break;
    }

    return profile;
}

double ValuationService::price_instrument(
    const domain::Trade& trade,
    const QuantLib::Instrument& instrument) {

    const auto profile = pricing_profile(trade);
    return instrument.NPV() * profile.multiplier + profile.additive_npv;
}

std::vector<ValuationResult> ValuationService::price_portfolio(
    const domain::Portfolio& portfolio,
    const PricingContext& context) {

    std::vector<ValuationResult> results;
    for (const auto& trade_ptr : portfolio.trades) {
        const auto& trade = *trade_ptr;
        auto ql_instrument = instruments::InstrumentFactory::create_instrument(trade, context);
        if (ql_instrument) {
            ValuationResult res;
            res.trade_id = trade.id;
            res.npv = price_instrument(trade, *ql_instrument);
            res.currency = trade.currency;
            res.tags["book"] = trade.book;
            res.tags["strategy"] = trade.strategy;
            results.push_back(res);
        } else {
            // Log or handle failed instrument creation
            ValuationResult res;
            res.trade_id = trade.id;
            res.npv = 0.0;
            res.currency = trade.currency;
            res.tags["error"] = "Instrument construction failed";
            res.tags["status"] = "failed";
            results.push_back(res);
        }
    }
    return results;
}

} // namespace qrp::analytics
