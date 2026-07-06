// Implements the instrument factory facade shared by analytics services.

#include <qrp/analytics/product_pricing_registry.hpp>
#include <qrp/instruments/instrument_factory.hpp>

namespace qrp::instruments {

QuantLib::ext::shared_ptr<QuantLib::Instrument>
InstrumentFactory::create_instrument(const domain::Trade& trade, const analytics::PricingContext& context) {
    return analytics::ProductPricingRegistry::create_instrument(trade, context);
}

} // namespace qrp::instruments
