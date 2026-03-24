#pragma once
#include <qrp/domain/portfolio.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <qrp/instruments/instrument_factory.hpp>
#include <map>

namespace qrp::analytics {

struct ValuationResult {
    std::string trade_id;
    double npv;
    std::string currency;
    std::map<std::string, std::string> tags;
};

class ValuationService {
public:
    static std::vector<ValuationResult> price_portfolio(
        const domain::Portfolio& portfolio,
        const market::MarketSnapshot& market) {

        std::vector<ValuationResult> results;
        for (const auto& trade : portfolio.trades) {
            auto ql_instrument = instruments::InstrumentFactory::create_instrument(trade, market);
            if (ql_instrument) {
                ValuationResult res;
                res.trade_id = trade.id;
                res.npv = ql_instrument->NPV();
                res.currency = trade.currency;
                res.tags["book"] = trade.book;
                res.tags["strategy"] = trade.strategy;
                results.push_back(res);
            }
        }
        return results;
    }
};

} // namespace qrp::analytics
