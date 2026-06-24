#include <qrp/analytics/product_pricing_registry.hpp>
#include <qrp/instruments/instrument_factory.hpp>
#include <map>
#include <utility>

namespace qrp::analytics {
namespace {

domain::AssetClass resolve_asset_class(const domain::Trade& trade, domain::ProductType product_type) {
    if (trade.asset_class_type != domain::AssetClass::Unknown) {
        return trade.asset_class_type;
    }
    auto parsed = domain::parse_asset_class(trade.asset_class);
    if (parsed != domain::AssetClass::Unknown) {
        return parsed;
    }
    return domain::asset_class_from_product_type(product_type);
}

domain::ProductType resolve_product_type(const domain::Trade& trade) {
    if (trade.product_type != domain::ProductType::Unknown) {
        return trade.product_type;
    }
    return domain::product_type_from_trade_type(trade.trade_type);
}

CashflowExtractionResult no_realized_cashflows(
    const domain::Trade& trade,
    const domain::MarketSnapshot& previous_market,
    const domain::MarketSnapshot& current_market) {
    static_cast<void>(previous_market);
    static_cast<void>(current_market);

    CashflowExtractionResult result;
    result.extraction_supported = false;
    result.model_name = "NoRealizedCashflowExtractor";
    result.support_status = domain::SupportStatus::Unsupported;
    result.status_message = "No realized cashflow event source is configured for this product";
    result.tags["trade_id"] = trade.id;
    return result;
}

InstrumentPricingProfile default_pricing_profile(const domain::Trade&) {
    return {};
}

InstrumentPricingProfile equity_spot_pricing_profile(const domain::Trade& trade) {
    InstrumentPricingProfile profile;
    if (const auto* equity = dynamic_cast<const domain::EquitySpotTrade*>(&trade)) {
        profile.additive_npv = -equity->reference_price * equity->quantity;
        profile.multiplier = equity->quantity;
    }
    return profile;
}

InstrumentPricingProfile fx_forward_pricing_profile(const domain::Trade& trade) {
    InstrumentPricingProfile profile;
    if (const auto* fx_forward = dynamic_cast<const domain::FxForwardTrade*>(&trade)) {
        profile.additive_npv = -fx_forward->forward_rate * fx_forward->notional;
        profile.multiplier = fx_forward->notional;
    }
    return profile;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_equity_spot(
    const domain::Trade& trade,
    const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::EquitySpotTrade*>(&trade);
    return typed ? instruments::InstrumentFactory::create_equity_spot(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_fixed_rate_bond(
    const domain::Trade& trade,
    const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::FixedRateBondTrade*>(&trade);
    return typed ? instruments::InstrumentFactory::create_bond(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_fx_forward(
    const domain::Trade& trade,
    const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::FxForwardTrade*>(&trade);
    return typed ? instruments::InstrumentFactory::create_fx_forward(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_vanilla_swap(
    const domain::Trade& trade,
    const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::VanillaSwapTrade*>(&trade);
    return typed ? instruments::InstrumentFactory::create_swap(*typed, context) : nullptr;
}

ProductPricingDefinition unsupported_definition(
    domain::ProductType product_type,
    std::string reason) {
    return ProductPricingDefinition{
        no_realized_cashflows,
        {},
        "unsupported",
        default_pricing_profile,
        product_type,
        std::move(reason),
        domain::SupportStatus::Unsupported
    };
}

const ProductPricingDefinition& unknown_definition() {
    static const ProductPricingDefinition definition = unsupported_definition(
        domain::ProductType::Unknown,
        "Unknown product type");
    return definition;
}

const std::map<domain::ProductType, ProductPricingDefinition>& definitions() {
    static const std::map<domain::ProductType, ProductPricingDefinition> registry = {
        {domain::ProductType::CallableBond, unsupported_definition(
            domain::ProductType::CallableBond,
            "Product is declared in the taxonomy but no pricing model is registered yet")},
        {domain::ProductType::CapFloor, unsupported_definition(
            domain::ProductType::CapFloor,
            "Product is declared in the taxonomy but no pricing model is registered yet")},
        {domain::ProductType::Cds, unsupported_definition(
            domain::ProductType::Cds,
            "Product is declared in the taxonomy but no pricing model is registered yet")},
        {domain::ProductType::CdsOption, unsupported_definition(
            domain::ProductType::CdsOption,
            "Product is declared in the taxonomy but no pricing model is registered yet")},
        {domain::ProductType::CommodityFuture, unsupported_definition(
            domain::ProductType::CommodityFuture,
            "Product is declared in the taxonomy but no pricing model is registered yet")},
        {domain::ProductType::CommodityFutureOption, unsupported_definition(
            domain::ProductType::CommodityFutureOption,
            "Product is declared in the taxonomy but no pricing model is registered yet")},
        {domain::ProductType::CommoditySwing, unsupported_definition(
            domain::ProductType::CommoditySwing,
            "Product is declared in the taxonomy but no pricing model is registered yet")},
        {domain::ProductType::CreditBond, unsupported_definition(
            domain::ProductType::CreditBond,
            "Product is declared in the taxonomy but no pricing model is registered yet")},
        {domain::ProductType::CrossCurrencySwap, unsupported_definition(
            domain::ProductType::CrossCurrencySwap,
            "Product is declared in the taxonomy but no pricing model is registered yet")},
        {domain::ProductType::Deposit, unsupported_definition(
            domain::ProductType::Deposit,
            "Product is declared in the taxonomy but no pricing model is registered yet")},
        {domain::ProductType::EquityOption, unsupported_definition(
            domain::ProductType::EquityOption,
            "Equity option product is declared; vanilla and American option models are not registered yet")},
        {domain::ProductType::EquitySpot, ProductPricingDefinition{
            no_realized_cashflows,
            build_equity_spot,
            "QuantLib::Stock with trade-level quantity/reference adjustment",
            equity_spot_pricing_profile,
            domain::ProductType::EquitySpot,
            "Equity spot exposure is supported; dividends, borrow, and equity option models are not yet included",
            domain::SupportStatus::PartiallySupported}},
        {domain::ProductType::FixedRateBond, ProductPricingDefinition{
            no_realized_cashflows,
            build_fixed_rate_bond,
            "QuantLib::FixedRateBond/DiscountingBondEngine",
            default_pricing_profile,
            domain::ProductType::FixedRateBond,
            "Fixed-rate bond pricing is supported for configured discount curves",
            domain::SupportStatus::Supported}},
        {domain::ProductType::Fra, unsupported_definition(
            domain::ProductType::Fra,
            "Product is declared in the taxonomy but no pricing model is registered yet")},
        {domain::ProductType::Future, unsupported_definition(
            domain::ProductType::Future,
            "Product is declared in the taxonomy but no pricing model is registered yet")},
        {domain::ProductType::FxForward, ProductPricingDefinition{
            no_realized_cashflows,
            build_fx_forward,
            "QuantLib::Stock with forward-rate offset",
            fx_forward_pricing_profile,
            domain::ProductType::FxForward,
            "Simple FX forward exposure is supported; full domestic/foreign curve forward valuation is not yet included",
            domain::SupportStatus::PartiallySupported}},
        {domain::ProductType::FxOption, unsupported_definition(
            domain::ProductType::FxOption,
            "Product is declared in the taxonomy but no pricing model is registered yet")},
        {domain::ProductType::OisSwap, unsupported_definition(
            domain::ProductType::OisSwap,
            "Product is declared in the taxonomy but no pricing model is registered yet")},
        {domain::ProductType::Swaption, unsupported_definition(
            domain::ProductType::Swaption,
            "Product is declared in the taxonomy but no pricing model is registered yet")},
        {domain::ProductType::VanillaSwap, ProductPricingDefinition{
            no_realized_cashflows,
            build_vanilla_swap,
            "QuantLib::VanillaSwap/DiscountingSwapEngine",
            default_pricing_profile,
            domain::ProductType::VanillaSwap,
            "Vanilla fixed-float swap pricing is supported for configured rates curves",
            domain::SupportStatus::Supported}},
        {domain::ProductType::Unknown, unknown_definition()}
    };
    return registry;
}

} // namespace

QuantLib::ext::shared_ptr<QuantLib::Instrument> ProductPricingRegistry::create_instrument(
    const domain::Trade& trade,
    const PricingContext& context) {
    const auto& definition = definition_for(trade);
    return definition.instrument_builder ? definition.instrument_builder(trade, context) : nullptr;
}

CashflowExtractionResult ProductPricingRegistry::extract_realized_cashflows(
    const domain::Trade& trade,
    const domain::MarketSnapshot& previous_market,
    const domain::MarketSnapshot& current_market) {
    const auto& definition = definition_for(trade);
    auto result = definition.cashflow_extractor
        ? definition.cashflow_extractor(trade, previous_market, current_market)
        : no_realized_cashflows(trade, previous_market, current_market);
    if (result.model_name.empty()) {
        result.model_name = definition.model_name;
    }
    result.tags["product_support_status"] = domain::to_string(definition.status);
    return result;
}

const ProductPricingDefinition& ProductPricingRegistry::definition_for(const domain::Trade& trade) {
    const auto product_type = resolve_product_type(trade);
    const auto& registry = definitions();
    const auto it = registry.find(product_type);
    return it == registry.end() ? unknown_definition() : it->second;
}

InstrumentPricingProfile ProductPricingRegistry::pricing_profile(const domain::Trade& trade) {
    const auto& definition = definition_for(trade);
    return definition.pricing_profile_builder ? definition.pricing_profile_builder(trade) : InstrumentPricingProfile{};
}

ProductSupportProfile ProductPricingRegistry::support_profile(const domain::Trade& trade) {
    const auto& definition = definition_for(trade);

    ProductSupportProfile profile;
    profile.asset_class = resolve_asset_class(trade, definition.product_type);
    profile.model_name = definition.model_name;
    profile.product_type = definition.product_type;
    profile.reason = definition.reason;
    profile.status = definition.status;
    return profile;
}

} // namespace qrp::analytics
