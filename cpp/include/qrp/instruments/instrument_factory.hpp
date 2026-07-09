#pragma once

// Declares trade-to-QuantLib instrument construction for supported product types.

#include <qrp/analytics/pricing_context.hpp>
#include <qrp/domain/portfolio.hpp>

#include <memory>
#include <ql/instrument.hpp>
#include <ql/shared_ptr.hpp>

namespace qrp::instruments {

/**
 * @brief Builds commodity instruments from canonical commodity trade DTOs.
 */
class CommodityInstrumentFactory {
public:
    /**
     * @brief Creates a commodity spot mark-to-market instrument.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument>
    create_commodity_spot(const domain::CommoditySpotTrade& trade, const analytics::PricingContext& context);

    /**
     * @brief Creates a commodity forward instrument from quoted forward or spot inputs.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument>
    create_commodity_forward(const domain::CommodityForwardTrade& trade, const analytics::PricingContext& context);

    /**
     * @brief Creates a listed commodity future mark-to-market instrument.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument>
    create_commodity_future(const domain::CommodityFutureTrade& trade, const analytics::PricingContext& context);

    /**
     * @brief Creates a weighted commodity futures strip mark-to-market instrument.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument>
    create_commodity_future_strip(const domain::CommodityFutureStripTrade& trade,
                                  const analytics::PricingContext& context);

    /**
     * @brief Creates a Black-76 option on a commodity future.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument>
    create_commodity_future_option(const domain::CommodityFutureOptionTrade& trade,
                                   const analytics::PricingContext& context);

    /**
     * @brief Creates a normal-model option on a commodity calendar spread.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument>
    create_commodity_calendar_spread_option(const domain::CommodityCalendarSpreadOptionTrade& trade,
                                            const analytics::PricingContext& context);

    /**
     * @brief Creates a commodity swing contract using stateful exercise dynamic programming.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument>
    create_commodity_swing(const domain::CommoditySwingTrade& trade, const analytics::PricingContext& context);

    /**
     * @brief Creates a gas storage contract using inventory dynamic programming.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_gas_storage(const domain::GasStorageTrade& trade,
                                                                              const analytics::PricingContext& context);
};

/**
 * @brief Builds credit instruments from canonical credit trade DTOs.
 */
class CreditInstrumentFactory {
public:
    /**
     * @brief Creates a credit bond instrument discounted by risk-free and issuer spread curves.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_credit_bond(const domain::CreditBondTrade& trade,
                                                                              const analytics::PricingContext& context);

    /**
     * @brief Creates a single-name CDS instrument from spread, recovery, and discount inputs.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_cds(const domain::CdsTrade& trade,
                                                                      const analytics::PricingContext& context);

    /**
     * @brief Creates a CDS index instrument from index spread, recovery, and discount inputs.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_cds_index(const domain::CdsIndexTrade& trade,
                                                                            const analytics::PricingContext& context);

    /**
     * @brief Creates a European option on a single-name CDS spread.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_cds_option(const domain::CdsOptionTrade& trade,
                                                                             const analytics::PricingContext& context);

    /**
     * @brief Creates a European option on a CDS index spread.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument>
    create_credit_index_option(const domain::CreditIndexOptionTrade& trade, const analytics::PricingContext& context);
};

/**
 * @brief Builds equity instruments from canonical equity trade DTOs.
 */
class EquityInstrumentFactory {
public:
    /**
     * @brief Creates an equity spot instrument linked to the underlier market quote.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_equity_spot(const domain::EquitySpotTrade& trade,
                                                                              const analytics::PricingContext& context);

    /**
     * @brief Creates an equity forward instrument with dividend and borrow inputs.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument>
    create_equity_forward(const domain::EquityForwardTrade& trade, const analytics::PricingContext& context);

    /**
     * @brief Creates a listed equity or index future instrument.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument>
    create_equity_future(const domain::EquityFutureTrade& trade, const analytics::PricingContext& context);

    /**
     * @brief Creates a European or American equity/index option instrument.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument>
    create_equity_option(const domain::EquityOptionTrade& trade, const analytics::PricingContext& context);
};

/**
 * @brief Builds FX instruments from canonical FX trade DTOs.
 */
class FxInstrumentFactory {
public:
    /**
     * @brief Creates an FX spot exposure instrument linked to the market spot quote.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_fx_spot(const domain::FxSpotTrade& trade,
                                                                          const analytics::PricingContext& context);

    /**
     * @brief Creates an FX forward instrument from spot, forward, and notional inputs.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_fx_forward(const domain::FxForwardTrade& trade,
                                                                             const analytics::PricingContext& context);

    /**
     * @brief Creates an FX swap instrument from near and far forward legs.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_fx_swap(const domain::FxSwapTrade& trade,
                                                                          const analytics::PricingContext& context);

    /**
     * @brief Creates an NDF instrument settled in the quote currency.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_ndf(const domain::NdfTrade& trade,
                                                                      const analytics::PricingContext& context);

    /**
     * @brief Creates a vanilla European FX option instrument.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_fx_option(const domain::FxOptionTrade& trade,
                                                                            const analytics::PricingContext& context);
};

/**
 * @brief Builds rates instruments from canonical rates trade DTOs.
 */
class RatesInstrumentFactory {
public:
    /**
     * @brief Creates a cash deposit instrument discounted from configured rates curves.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_deposit(const domain::DepositTrade& trade,
                                                                          const analytics::PricingContext& context);

    /**
     * @brief Creates a QuantLib FRA instrument from a forward-rate agreement trade.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_fra(const domain::FraTrade& trade,
                                                                      const analytics::PricingContext& context);

    /**
     * @brief Creates an exchange-traded short-rate future exposure instrument.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument>
    create_interest_rate_future(const domain::InterestRateFutureTrade& trade, const analytics::PricingContext& context);

    /**
     * @brief Creates a QuantLib swap instrument from a vanilla swap trade.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_swap(const domain::VanillaSwapTrade& trade,
                                                                       const analytics::PricingContext& context);

    /**
     * @brief Creates a QuantLib overnight-indexed swap instrument.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_ois_swap(const domain::OisSwapTrade& trade,
                                                                           const analytics::PricingContext& context);

    /**
     * @brief Creates a QuantLib bond instrument from a fixed-rate bond trade.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_bond(const domain::FixedRateBondTrade& trade,
                                                                       const analytics::PricingContext& context);

    /**
     * @brief Creates an LSMC-priced callable fixed-rate bond instrument.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument>
    create_callable_bond(const domain::CallableBondTrade& trade, const analytics::PricingContext& context);

    /**
     * @brief Creates a QuantLib floating-rate note instrument.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument>
    create_floating_rate_note(const domain::FloatingRateNoteTrade& trade, const analytics::PricingContext& context);

    /**
     * @brief Creates a QuantLib cap or floor instrument on an IBOR leg.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_cap_floor(const domain::CapFloorTrade& trade,
                                                                            const analytics::PricingContext& context);

    /**
     * @brief Creates a QuantLib European swaption instrument.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument>
    create_european_swaption(const domain::EuropeanSwaptionTrade& trade, const analytics::PricingContext& context);

    /**
     * @brief Creates an LSMC-priced Bermudan swaption instrument.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument>
    create_bermudan_swaption(const domain::BermudanSwaptionTrade& trade, const analytics::PricingContext& context);
};

/**
 * @brief Facade used by analytics services to dispatch through the product-pricing registry.
 */
class InstrumentFactory {
public:
    /**
     * @brief Dispatches to the product-specific creator for the trade type.
     */
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_instrument(const domain::Trade& trade,
                                                                             const analytics::PricingContext& context);
};

} // namespace qrp::instruments
