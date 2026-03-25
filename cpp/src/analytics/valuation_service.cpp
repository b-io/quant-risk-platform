#include <qrp/analytics/valuation_service.hpp>
#include <qrp/instruments/instrument_factory.hpp>

namespace qrp::analytics {

std::vector<ValuationResult> ValuationService::price_portfolio(
    const domain::Portfolio& portfolio,
    const PricingContext& context) {

    std::vector<ValuationResult> results;
    for (const auto& trade : portfolio.trades) {
        auto ql_instrument = instruments::InstrumentFactory::create_instrument(trade, context);
        if (ql_instrument) {
            ValuationResult res;
            res.trade_id = trade.id;
            res.npv = ql_instrument->NPV();
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
