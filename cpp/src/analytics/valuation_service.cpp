#include <qrp/analytics/valuation_service.hpp>
#include <qrp/instruments/instrument_factory.hpp>

namespace qrp::analytics {

ProductSupportProfile ValuationService::support_profile(const domain::Trade& trade) {
    ProductSupportProfile profile;
    profile.asset_class = trade.asset_class_type == domain::AssetClass::Unknown
        ? domain::parse_asset_class(trade.asset_class)
        : trade.asset_class_type;
    profile.product_type = trade.product_type == domain::ProductType::Unknown
        ? domain::product_type_from_trade_type(trade.trade_type)
        : trade.product_type;
    if (profile.asset_class == domain::AssetClass::Unknown) {
        profile.asset_class = domain::asset_class_from_product_type(profile.product_type);
    }

    switch (profile.product_type) {
        case domain::ProductType::CallableBond:
        case domain::ProductType::CapFloor:
        case domain::ProductType::Cds:
        case domain::ProductType::CdsOption:
        case domain::ProductType::CommodityFuture:
        case domain::ProductType::CommodityFutureOption:
        case domain::ProductType::CommoditySwing:
        case domain::ProductType::CreditBond:
        case domain::ProductType::CrossCurrencySwap:
        case domain::ProductType::Deposit:
            profile.status = domain::SupportStatus::Unsupported;
            profile.model_name = "unsupported";
            profile.reason = "Product is declared in the taxonomy but no pricing model is registered yet";
            break;
        case domain::ProductType::EquityOption:
            profile.status = domain::SupportStatus::Unsupported;
            profile.model_name = "unsupported";
            profile.reason = "Equity option product is declared; vanilla/American option models are not registered yet";
            break;
        case domain::ProductType::EquitySpot:
            profile.status = domain::SupportStatus::PartiallySupported;
            profile.model_name = "QuantLib::Stock with trade-level quantity/reference adjustment";
            profile.reason = "Equity spot exposure is supported; dividends, borrow, and equity option models are not yet included";
            break;
        case domain::ProductType::FixedRateBond:
            profile.status = domain::SupportStatus::Supported;
            profile.model_name = "QuantLib::FixedRateBond/DiscountingBondEngine";
            profile.reason = "Fixed-rate bond pricing is supported for configured discount curves";
            break;
        case domain::ProductType::Fra:
        case domain::ProductType::Future:
            profile.status = domain::SupportStatus::Unsupported;
            profile.model_name = "unsupported";
            profile.reason = "Product is declared in the taxonomy but no pricing model is registered yet";
            break;
        case domain::ProductType::FxForward:
            profile.status = domain::SupportStatus::PartiallySupported;
            profile.model_name = "QuantLib::Stock with forward-rate offset";
            profile.reason = "Simple FX forward exposure is supported; full domestic/foreign curve forward valuation is not yet included";
            break;
        case domain::ProductType::FxOption:
        case domain::ProductType::OisSwap:
        case domain::ProductType::Swaption:
            profile.status = domain::SupportStatus::Unsupported;
            profile.model_name = "unsupported";
            profile.reason = "Product is declared in the taxonomy but no pricing model is registered yet";
            break;
        case domain::ProductType::VanillaSwap:
            profile.status = domain::SupportStatus::Supported;
            profile.model_name = "QuantLib::VanillaSwap/DiscountingSwapEngine";
            profile.reason = "Vanilla fixed-float swap pricing is supported for configured rates curves";
            break;
        case domain::ProductType::Unknown:
            profile.status = domain::SupportStatus::Unsupported;
            profile.model_name = "unsupported";
            profile.reason = "Unknown product type";
            break;
    }

    return profile;
}

InstrumentPricingProfile ValuationService::pricing_profile(const domain::Trade& trade) {
    InstrumentPricingProfile profile;

    switch (trade.trade_type) {
        case domain::TradeType::EquitySpot: {
            const auto& eq_trade = dynamic_cast<const domain::EquitySpotTrade&>(trade);
            profile.multiplier = eq_trade.quantity;
            profile.additive_npv = -eq_trade.reference_price * eq_trade.quantity;
            break;
        }
        case domain::TradeType::FixedRateBond:
            break;
        case domain::TradeType::FxForward: {
            const auto& fx_trade = dynamic_cast<const domain::FxForwardTrade&>(trade);
            profile.multiplier = fx_trade.notional;
            profile.additive_npv = -fx_trade.forward_rate * fx_trade.notional;
            break;
        }
        case domain::TradeType::VanillaSwap:
            break;
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
        const auto support = support_profile(trade);
        auto ql_instrument = instruments::InstrumentFactory::create_instrument(trade, context);

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
