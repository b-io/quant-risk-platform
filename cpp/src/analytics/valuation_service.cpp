#include <qrp/analytics/valuation_service.hpp>
#include <qrp/instruments/instrument_factory.hpp>

namespace qrp::analytics {

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
            double quantity = 1.0;
            double ref_price = 0.0;
            if (trade.type == "equity_spot") {
                const auto& eq_trade = dynamic_cast<const domain::EquitySpotTrade&>(trade);
                quantity = eq_trade.quantity;
                ref_price = eq_trade.reference_price;
                // NPV for EquitySpot = (CurrentPrice - ReferencePrice) * Quantity
                res.npv = (ql_instrument->NPV() - ref_price) * quantity;
            } else if (trade.type == "fx_forward") {
                const auto& fx_trade = dynamic_cast<const domain::FxForwardTrade&>(trade);
                double spot = ql_instrument->NPV();
                // Simple NPV = (Spot - ForwardRate) * Notional
                // Note: Simplified as we are not applying discounting to FX in this MVP stage
                res.npv = (spot - fx_trade.forward_rate) * fx_trade.notional;
            } else {
                res.npv = ql_instrument->NPV() * quantity;
            }
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
            res.tags["status"] = "failed";
            results.push_back(res);
        }
    }
    return results;
}

} // namespace qrp::analytics
