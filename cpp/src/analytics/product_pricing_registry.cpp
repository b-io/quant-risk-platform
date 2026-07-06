// Implements the product-pricing registry that declares supported models and market requirements.

#include <qrp/analytics/product_pricing_registry.hpp>
#include <qrp/instruments/instrument_factory.hpp>

#include <algorithm>
#include <cctype>
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

CashflowExtractionResult no_realized_cashflows(const domain::Trade& trade,
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

std::string lower_copy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool is_short_direction(const std::string& direction) {
    const auto lowered = lower_copy(direction);
    return lowered == "short" || lowered == "sell" || lowered == "sold" || lowered == "written";
}

double option_position_sign(const std::string& direction) {
    return is_short_direction(direction) ? -1.0 : 1.0;
}

InstrumentPricingProfile equity_spot_pricing_profile(const domain::Trade& trade) {
    InstrumentPricingProfile profile;
    if (const auto* equity = dynamic_cast<const domain::EquitySpotTrade*>(&trade)) {
        profile.additive_npv = -equity->reference_price * equity->quantity;
        profile.multiplier = equity->quantity;
    }
    return profile;
}

InstrumentPricingProfile interest_rate_future_pricing_profile(const domain::Trade& trade) {
    InstrumentPricingProfile profile;
    if (const auto* future = dynamic_cast<const domain::InterestRateFutureTrade*>(&trade)) {
        const double direction_sign = is_short_direction(future->direction) ? -1.0 : 1.0;
        profile.multiplier = direction_sign * future->quantity * future->contract_size;
        profile.additive_npv = -future->reference_price * profile.multiplier;
    }
    return profile;
}

InstrumentPricingProfile option_pricing_profile(const domain::Trade& trade) {
    InstrumentPricingProfile profile;
    profile.multiplier = option_position_sign(trade.direction);
    return profile;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_bermudan_swaption(const domain::Trade& trade,
                                                                        const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::BermudanSwaptionTrade*>(&trade);
    return typed ? instruments::RatesInstrumentFactory::create_bermudan_swaption(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_cap_floor(const domain::Trade& trade,
                                                                const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::CapFloorTrade*>(&trade);
    return typed ? instruments::RatesInstrumentFactory::create_cap_floor(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_cds(const domain::Trade& trade, const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::CdsTrade*>(&trade);
    return typed ? instruments::CreditInstrumentFactory::create_cds(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_cds_index(const domain::Trade& trade,
                                                                const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::CdsIndexTrade*>(&trade);
    return typed ? instruments::CreditInstrumentFactory::create_cds_index(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_cds_option(const domain::Trade& trade,
                                                                 const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::CdsOptionTrade*>(&trade);
    return typed ? instruments::CreditInstrumentFactory::create_cds_option(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_credit_bond(const domain::Trade& trade,
                                                                  const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::CreditBondTrade*>(&trade);
    return typed ? instruments::CreditInstrumentFactory::create_credit_bond(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_credit_index_option(const domain::Trade& trade,
                                                                          const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::CreditIndexOptionTrade*>(&trade);
    return typed ? instruments::CreditInstrumentFactory::create_credit_index_option(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_commodity_calendar_spread_option(const domain::Trade& trade,
                                                                                       const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::CommodityCalendarSpreadOptionTrade*>(&trade);
    return typed ? instruments::CommodityInstrumentFactory::create_commodity_calendar_spread_option(*typed, context)
                 : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_commodity_forward(const domain::Trade& trade,
                                                                        const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::CommodityForwardTrade*>(&trade);
    return typed ? instruments::CommodityInstrumentFactory::create_commodity_forward(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_commodity_future(const domain::Trade& trade,
                                                                       const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::CommodityFutureTrade*>(&trade);
    return typed ? instruments::CommodityInstrumentFactory::create_commodity_future(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_commodity_future_option(const domain::Trade& trade,
                                                                              const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::CommodityFutureOptionTrade*>(&trade);
    return typed ? instruments::CommodityInstrumentFactory::create_commodity_future_option(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_commodity_future_strip(const domain::Trade& trade,
                                                                             const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::CommodityFutureStripTrade*>(&trade);
    return typed ? instruments::CommodityInstrumentFactory::create_commodity_future_strip(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_commodity_spot(const domain::Trade& trade,
                                                                     const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::CommoditySpotTrade*>(&trade);
    return typed ? instruments::CommodityInstrumentFactory::create_commodity_spot(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_commodity_swing(const domain::Trade& trade,
                                                                      const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::CommoditySwingTrade*>(&trade);
    return typed ? instruments::CommodityInstrumentFactory::create_commodity_swing(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_deposit(const domain::Trade& trade,
                                                              const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::DepositTrade*>(&trade);
    return typed ? instruments::RatesInstrumentFactory::create_deposit(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_equity_spot(const domain::Trade& trade,
                                                                  const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::EquitySpotTrade*>(&trade);
    return typed ? instruments::EquityInstrumentFactory::create_equity_spot(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_equity_forward(const domain::Trade& trade,
                                                                     const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::EquityForwardTrade*>(&trade);
    return typed ? instruments::EquityInstrumentFactory::create_equity_forward(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_equity_future(const domain::Trade& trade,
                                                                    const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::EquityFutureTrade*>(&trade);
    return typed ? instruments::EquityInstrumentFactory::create_equity_future(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_equity_option(const domain::Trade& trade,
                                                                    const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::EquityOptionTrade*>(&trade);
    return typed ? instruments::EquityInstrumentFactory::create_equity_option(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_european_swaption(const domain::Trade& trade,
                                                                        const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::EuropeanSwaptionTrade*>(&trade);
    return typed ? instruments::RatesInstrumentFactory::create_european_swaption(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_fixed_rate_bond(const domain::Trade& trade,
                                                                      const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::FixedRateBondTrade*>(&trade);
    return typed ? instruments::RatesInstrumentFactory::create_bond(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_floating_rate_note(const domain::Trade& trade,
                                                                         const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::FloatingRateNoteTrade*>(&trade);
    return typed ? instruments::RatesInstrumentFactory::create_floating_rate_note(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_fra(const domain::Trade& trade, const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::FraTrade*>(&trade);
    return typed ? instruments::RatesInstrumentFactory::create_fra(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_fx_forward(const domain::Trade& trade,
                                                                 const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::FxForwardTrade*>(&trade);
    return typed ? instruments::FxInstrumentFactory::create_fx_forward(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_fx_option(const domain::Trade& trade,
                                                                const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::FxOptionTrade*>(&trade);
    return typed ? instruments::FxInstrumentFactory::create_fx_option(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_fx_spot(const domain::Trade& trade,
                                                              const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::FxSpotTrade*>(&trade);
    return typed ? instruments::FxInstrumentFactory::create_fx_spot(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_fx_swap(const domain::Trade& trade,
                                                              const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::FxSwapTrade*>(&trade);
    return typed ? instruments::FxInstrumentFactory::create_fx_swap(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_interest_rate_future(const domain::Trade& trade,
                                                                           const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::InterestRateFutureTrade*>(&trade);
    return typed ? instruments::RatesInstrumentFactory::create_interest_rate_future(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_ndf(const domain::Trade& trade, const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::NdfTrade*>(&trade);
    return typed ? instruments::FxInstrumentFactory::create_ndf(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_ois_swap(const domain::Trade& trade,
                                                               const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::OisSwapTrade*>(&trade);
    return typed ? instruments::RatesInstrumentFactory::create_ois_swap(*typed, context) : nullptr;
}

QuantLib::ext::shared_ptr<QuantLib::Instrument> build_vanilla_swap(const domain::Trade& trade,
                                                                   const PricingContext& context) {
    const auto* typed = dynamic_cast<const domain::VanillaSwapTrade*>(&trade);
    return typed ? instruments::RatesInstrumentFactory::create_swap(*typed, context) : nullptr;
}

ProductPricingDefinition unsupported_definition(domain::ProductType product_type, std::string reason) {
    return ProductPricingDefinition{no_realized_cashflows,
                                    {},
                                    "unsupported",
                                    default_pricing_profile,
                                    product_type,
                                    std::move(reason),
                                    domain::SupportStatus::Unsupported};
}

const ProductPricingDefinition& unknown_definition() {
    static const ProductPricingDefinition definition =
        unsupported_definition(domain::ProductType::Unknown, "Unknown product type");
    return definition;
}

const std::map<domain::ProductType, ProductPricingDefinition>& definitions() {
    static const std::map<domain::ProductType, ProductPricingDefinition> registry = {
        {domain::ProductType::BermudanSwaption,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_bermudan_swaption,
                                  "QRP::BermudanSwaptionLsmcInstrument/one-factor LSMC",
                                  option_pricing_profile,
                                  domain::ProductType::BermudanSwaption,
                                  "Bermudan swaption pricing is supported with a deterministic "
                                  "one-factor LSMC approximation",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::CallableBond,
         unsupported_definition(domain::ProductType::CallableBond,
                                "Product is declared in the taxonomy but no pricing model is registered yet")},
        {domain::ProductType::CapFloor,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_cap_floor,
                                  "QuantLib::CapFloor/BlackCapFloorEngine",
                                  option_pricing_profile,
                                  domain::ProductType::CapFloor,
                                  "Caps and floors are supported for configured rates curves and "
                                  "flat cap/floor volatility quotes",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::Cds,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_cds,
                                  "QRP::CdsInstrument/spread-implied hazard curve",
                                  default_pricing_profile,
                                  domain::ProductType::Cds,
                                  "Single-name CDS pricing is supported from CDS spread curves and recovery quotes",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::CdsIndex,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_cds_index,
                                  "QRP::CdsInstrument/index spread-implied hazard curve",
                                  default_pricing_profile,
                                  domain::ProductType::CdsIndex,
                                  "CDS index pricing is supported from index spread curves, "
                                  "recovery quotes, and index factors",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::CdsOption,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_cds_option,
                                  "QRP::CreditSpreadOptionInstrument/Black spread option",
                                  default_pricing_profile,
                                  domain::ProductType::CdsOption,
                                  "European CDS options are supported with quoted or trade-level "
                                  "credit spread volatility",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::CommodityCalendarSpreadOption,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_commodity_calendar_spread_option,
                                  "QRP::CalendarSpreadOptionInstrument/normal spread option",
                                  default_pricing_profile,
                                  domain::ProductType::CommodityCalendarSpreadOption,
                                  "Commodity calendar spread options are supported from near/far "
                                  "futures and spread volatility",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::CommodityForward,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_commodity_forward,
                                  "QRP::PriceMtmInstrument/discounted commodity forward",
                                  default_pricing_profile,
                                  domain::ProductType::CommodityForward,
                                  "Commodity forwards are supported from forward/futures quotes or spot fallback",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::CommodityFuture,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_commodity_future,
                                  "QRP::PriceMtmInstrument/listed commodity future",
                                  default_pricing_profile,
                                  domain::ProductType::CommodityFuture,
                                  "Listed commodity futures are supported from futures price quotes",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::CommodityFutureOption,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_commodity_future_option,
                                  "QRP::BlackFutureOptionInstrument/Black-76",
                                  default_pricing_profile,
                                  domain::ProductType::CommodityFutureOption,
                                  "Options on commodity futures are supported with quoted or trade-level volatility",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::CommodityFutureStrip,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_commodity_future_strip,
                                  "QRP::StripMtmInstrument/weighted futures strip",
                                  default_pricing_profile,
                                  domain::ProductType::CommodityFutureStrip,
                                  "Commodity futures strips are supported as weighted baskets of listed futures",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::CommoditySpot,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_commodity_spot,
                                  "QRP::PriceMtmInstrument/commodity spot exposure",
                                  default_pricing_profile,
                                  domain::ProductType::CommoditySpot,
                                  "Commodity spot exposure is supported from configured commodity spot quotes",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::CommoditySwing,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_commodity_swing,
                                  "QRP::CommoditySwingInstrument/exercise-envelope approximation",
                                  default_pricing_profile,
                                  domain::ProductType::CommoditySwing,
                                  "Commodity swing contracts are supported with an intrinsic "
                                  "exercise-envelope approximation over configured exercise dates",
                                  domain::SupportStatus::PartiallySupported}},
        {domain::ProductType::CreditBond,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_credit_bond,
                                  "QRP::CreditBondInstrument/spread-discounted cashflows",
                                  default_pricing_profile,
                                  domain::ProductType::CreditBond,
                                  "Credit bond pricing is supported from risk-free discount curves "
                                  "and issuer spread curves",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::CreditIndexOption,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_credit_index_option,
                                  "QRP::CreditSpreadOptionInstrument/index Black spread option",
                                  default_pricing_profile,
                                  domain::ProductType::CreditIndexOption,
                                  "European credit index options are supported with quoted or "
                                  "trade-level spread volatility",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::CrossCurrencySwap,
         unsupported_definition(domain::ProductType::CrossCurrencySwap,
                                "Product is declared in the taxonomy but no pricing model is registered yet")},
        {domain::ProductType::Deposit,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_deposit,
                                  "QRP::DepositInstrument/discounted cashflow",
                                  default_pricing_profile,
                                  domain::ProductType::Deposit,
                                  "Cash deposit valuation is supported from configured discount curves",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::EquityForward,
         ProductPricingDefinition{
             no_realized_cashflows,
             build_equity_forward,
             "QRP::EquityForwardInstrument/cost-of-carry",
             default_pricing_profile,
             domain::ProductType::EquityForward,
             "Equity forwards are supported from spot, discount, dividend-yield, and borrow inputs",
             domain::SupportStatus::Supported}},
        {domain::ProductType::EquityFuture,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_equity_future,
                                  "QRP::EquityFutureInstrument/listed future or theoretical carry",
                                  default_pricing_profile,
                                  domain::ProductType::EquityFuture,
                                  "Equity and index futures are supported from listed futures "
                                  "quotes or spot carry inputs",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::EquityOption,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_equity_option,
                                  "QRP::EquityOptionInstrument/Black-Scholes plus binomial American exercise",
                                  default_pricing_profile,
                                  domain::ProductType::EquityOption,
                                  "European and American equity/index options are supported with dividend, borrow, and "
                                  "volatility inputs",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::EquitySpot,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_equity_spot,
                                  "QuantLib::Stock with trade-level quantity/reference adjustment",
                                  equity_spot_pricing_profile,
                                  domain::ProductType::EquitySpot,
                                  "Equity spot exposure is supported from configured equity spot quotes",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::EuropeanSwaption,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_european_swaption,
                                  "QuantLib::Swaption/BlackSwaptionEngine",
                                  option_pricing_profile,
                                  domain::ProductType::EuropeanSwaption,
                                  "European swaption pricing is supported for configured rates "
                                  "curves and flat swaption volatility quotes",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::FixedRateBond,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_fixed_rate_bond,
                                  "QuantLib::FixedRateBond/DiscountingBondEngine",
                                  default_pricing_profile,
                                  domain::ProductType::FixedRateBond,
                                  "Fixed-rate bond pricing is supported for configured discount curves",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::FloatingRateNote,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_floating_rate_note,
                                  "QuantLib::FloatingRateBond/DiscountingBondEngine",
                                  default_pricing_profile,
                                  domain::ProductType::FloatingRateNote,
                                  "Floating-rate note pricing is supported for configured discount "
                                  "and IBOR forecast curves",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::Fra,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_fra,
                                  "QuantLib::ForwardRateAgreement",
                                  default_pricing_profile,
                                  domain::ProductType::Fra,
                                  "FRA pricing is supported for configured discount and IBOR forecast curves",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::FxForward,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_fx_forward,
                                  "QRP::FxForwardInstrument/discounted forward exposure",
                                  default_pricing_profile,
                                  domain::ProductType::FxForward,
                                  "FX forwards are supported from spot, forward points/outrights, "
                                  "and configured domestic/foreign curves",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::FxOption,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_fx_option,
                                  "QRP::FxOptionInstrument/Garman-Kohlhagen",
                                  option_pricing_profile,
                                  domain::ProductType::FxOption,
                                  "Vanilla European FX options are supported with flat or quoted FX volatility",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::FxSpot,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_fx_spot,
                                  "QRP::FxSpotInstrument/spot exposure",
                                  default_pricing_profile,
                                  domain::ProductType::FxSpot,
                                  "FX spot exposure is supported from configured spot quotes",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::FxSwap,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_fx_swap,
                                  "QRP::FxSwapInstrument/two-leg forward exposure",
                                  default_pricing_profile,
                                  domain::ProductType::FxSwap,
                                  "FX swaps are supported as near and far forward legs",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::InterestRateFuture,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_interest_rate_future,
                                  "QRP::InterestRateFutureInstrument/price exposure",
                                  interest_rate_future_pricing_profile,
                                  domain::ProductType::InterestRateFuture,
                                  "Interest-rate futures are supported from futures quotes or forward curves",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::Ndf,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_ndf,
                                  "QRP::FxForwardInstrument/NDF cash settlement",
                                  default_pricing_profile,
                                  domain::ProductType::Ndf,
                                  "NDFs are supported as quote-currency settled forward exposures",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::OisSwap,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_ois_swap,
                                  "QuantLib::OvernightIndexedSwap/DiscountingSwapEngine",
                                  default_pricing_profile,
                                  domain::ProductType::OisSwap,
                                  "OIS swap pricing is supported for configured OIS discount curves",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::VanillaSwap,
         ProductPricingDefinition{no_realized_cashflows,
                                  build_vanilla_swap,
                                  "QuantLib::VanillaSwap/DiscountingSwapEngine",
                                  default_pricing_profile,
                                  domain::ProductType::VanillaSwap,
                                  "Vanilla fixed-float swap pricing is supported for configured rates curves",
                                  domain::SupportStatus::Supported}},
        {domain::ProductType::Unknown, unknown_definition()}};
    return registry;
}

} // namespace

QuantLib::ext::shared_ptr<QuantLib::Instrument>
ProductPricingRegistry::create_instrument(const domain::Trade& trade, const PricingContext& context) {
    const auto& definition = definition_for(trade);
    return definition.instrument_builder ? definition.instrument_builder(trade, context) : nullptr;
}

CashflowExtractionResult
ProductPricingRegistry::extract_realized_cashflows(const domain::Trade& trade,
                                                   const domain::MarketSnapshot& previous_market,
                                                   const domain::MarketSnapshot& current_market) {
    const auto& definition = definition_for(trade);
    auto result = definition.cashflow_extractor(trade, previous_market, current_market);
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
