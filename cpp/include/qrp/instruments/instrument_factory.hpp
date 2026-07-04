#pragma once

// Declares trade-to-QuantLib instrument construction for supported product types.

#include <qrp/analytics/pricing_context.hpp>
#include <qrp/domain/portfolio.hpp>

#include <ql/instrument.hpp>
#include <ql/shared_ptr.hpp>

#include <memory>

namespace qrp::instruments {

/**
 * @brief Builds QuantLib instruments from canonical domain trade DTOs.
 */
class InstrumentFactory {
public:
    /**
     * @brief Dispatches to the product-specific creator for the trade type.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_instrument(
        const domain::Trade& trade,
        const analytics::PricingContext& context);

    /**
     * @brief Creates a cash deposit instrument discounted from configured rates curves.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_deposit(
        const domain::DepositTrade& trade,
        const analytics::PricingContext& context);

    /**
     * @brief Creates a QuantLib FRA instrument from a forward-rate agreement trade.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_fra(
        const domain::FraTrade& trade,
        const analytics::PricingContext& context);

    /**
     * @brief Creates an exchange-traded short-rate future exposure instrument.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_interest_rate_future(
        const domain::InterestRateFutureTrade& trade,
        const analytics::PricingContext& context);

    /**
     * @brief Creates a QuantLib swap instrument from a vanilla swap trade.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_swap(
        const domain::VanillaSwapTrade& trade,
        const analytics::PricingContext& context);

    /**
     * @brief Creates a QuantLib overnight-indexed swap instrument.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_ois_swap(
        const domain::OisSwapTrade& trade,
        const analytics::PricingContext& context);

    /**
     * @brief Creates a QuantLib bond instrument from a fixed-rate bond trade.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_bond(
        const domain::FixedRateBondTrade& trade,
        const analytics::PricingContext& context);

    /**
     * @brief Creates a QuantLib floating-rate note instrument.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_floating_rate_note(
        const domain::FloatingRateNoteTrade& trade,
        const analytics::PricingContext& context);

    /**
     * @brief Creates a QuantLib cap or floor instrument on an IBOR leg.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_cap_floor(
        const domain::CapFloorTrade& trade,
        const analytics::PricingContext& context);

    /**
     * @brief Creates a QuantLib European swaption instrument.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_european_swaption(
        const domain::EuropeanSwaptionTrade& trade,
        const analytics::PricingContext& context);

    /**
     * @brief Creates an LSMC-priced Bermudan swaption instrument.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_bermudan_swaption(
        const domain::BermudanSwaptionTrade& trade,
        const analytics::PricingContext& context);

    /**
     * @brief Creates an FX spot exposure instrument linked to the market spot quote.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_fx_spot(
        const domain::FxSpotTrade& trade,
        const analytics::PricingContext& context);

    /**
     * @brief Creates an FX forward instrument from spot, forward, and notional inputs.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_fx_forward(
        const domain::FxForwardTrade& trade,
        const analytics::PricingContext& context);

    /**
     * @brief Creates an FX swap instrument from near and far forward legs.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_fx_swap(
        const domain::FxSwapTrade& trade,
        const analytics::PricingContext& context);

    /**
     * @brief Creates an NDF instrument settled in the quote currency.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_ndf(
        const domain::NdfTrade& trade,
        const analytics::PricingContext& context);

    /**
     * @brief Creates a vanilla European FX option instrument.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_fx_option(
        const domain::FxOptionTrade& trade,
        const analytics::PricingContext& context);

    /**
     * @brief Creates an equity spot instrument linked to the underlier market quote.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_equity_spot(
        const domain::EquitySpotTrade& trade,
        const analytics::PricingContext& context);
};

} // namespace qrp::instruments
