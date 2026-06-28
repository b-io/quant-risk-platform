#pragma once

// Declares trade-to-QuantLib instrument construction for supported product types.

#include <ql/instrument.hpp>
#include <ql/shared_ptr.hpp>
#include <qrp/analytics/pricing_context.hpp>
#include <qrp/domain/portfolio.hpp>
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
     * @brief Creates a QuantLib swap instrument from a vanilla swap trade.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_swap(
        const domain::VanillaSwapTrade& trade,
        const analytics::PricingContext& context);

    /**
     * @brief Creates a QuantLib bond instrument from a fixed-rate bond trade.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_bond(
        const domain::FixedRateBondTrade& trade,
        const analytics::PricingContext& context);

    /**
     * @brief Creates an FX forward instrument from spot, forward, and notional inputs.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_fx_forward(
        const domain::FxForwardTrade& trade,
        const analytics::PricingContext& context);

    /**
     * @brief Creates an equity spot instrument linked to the underlier market quote.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_equity_spot(
        const domain::EquitySpotTrade& trade,
        const analytics::PricingContext& context);
};

} // namespace qrp::instruments
