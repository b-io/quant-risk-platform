#pragma once
#include <ql/instrument.hpp>
#include <ql/shared_ptr.hpp>
#include <qrp/analytics/pricing_context.hpp>
#include <qrp/domain/portfolio.hpp>
#include <memory>

namespace qrp::instruments {

class InstrumentFactory {
public:
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_instrument(
        const domain::Trade& trade,
        const analytics::PricingContext& context);

private:
    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_swap(
        const domain::Trade& trade,
        const analytics::PricingContext& context);

    static QuantLib::ext::shared_ptr<QuantLib::Instrument> create_bond(
        const domain::Trade& trade,
        const analytics::PricingContext& context);
};

} // namespace qrp::instruments
