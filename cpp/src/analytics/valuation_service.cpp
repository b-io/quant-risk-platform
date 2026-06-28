// Implements portfolio and trade valuation orchestration over QuantLib instruments.

#include <qrp/analytics/valuation_service.hpp>
#include <qrp/analytics/product_pricing_registry.hpp>

namespace qrp::analytics {

ProductSupportProfile ValuationService::support_profile(const domain::Trade& trade) {
    return ProductPricingRegistry::support_profile(trade);
}

InstrumentPricingProfile ValuationService::pricing_profile(const domain::Trade& trade) {
    return ProductPricingRegistry::pricing_profile(trade);
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
        const auto support = support_profile(trade);
        auto ql_instrument = ProductPricingRegistry::create_instrument(trade, context);

        ValuationResult res;
        res.trade_id = trade.id;
        res.currency = trade.currency;
        res.asset_class = support.asset_class;
        res.product_type = support.product_type;
        res.support_status = support.status;
        res.model_name = support.model_name;
        res.status_message = support.reason;
        res.tags["asset_class"] = domain::to_string(support.asset_class);
        res.tags["book"] = trade.book;
        res.tags["model"] = support.model_name;
        res.tags["product_type"] = domain::to_string(support.product_type);
        res.tags["status"] = domain::to_string(support.status);
        res.tags["strategy"] = trade.strategy;

        if (ql_instrument) {
            res.npv = price_instrument(trade, *ql_instrument);
            results.push_back(res);
        } else {
            res.npv = 0.0;
            res.support_status = support.status == domain::SupportStatus::Unsupported
                ? domain::SupportStatus::Unsupported
                : domain::SupportStatus::Failed;
            res.status_message = "Instrument construction failed: " + support.reason;
            res.tags["error"] = "Instrument construction failed";
            res.tags["status"] = domain::to_string(res.support_status);
            results.push_back(res);
        }
    }
    return results;
}

} // namespace qrp::analytics
