#pragma once
#include <ql/instruments/vanillaswap.hpp>
#include <ql/instruments/bonds/fixedratebond.hpp>
#include <ql/pricingengines/swap/discountingswapengine.hpp>
#include <ql/pricingengines/bond/discountingbondengine.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/time/daycounters/actual360.hpp>
#include <ql/time/daycounters/thirty360.hpp>
#include <ql/indexes/ibor/usdlibor.hpp>
#include <qrp/domain/portfolio.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <memory>

namespace qrp::instruments {

class InstrumentFactory {
public:
    static std::shared_ptr<QuantLib::Instrument> create_instrument(
        const domain::Trade& trade,
        const market::MarketSnapshot& market) {

        if (trade.type == "vanilla_swap") {
            return create_swap(trade, market);
        } else if (trade.type == "fixed_rate_bond") {
            return create_bond(trade, market);
        }
        return nullptr;
    }

private:
    static std::shared_ptr<QuantLib::Instrument> create_swap(
        const domain::Trade& trade,
        const market::MarketSnapshot& market) {

        QuantLib::Date start = market::CurveBuilder::parse_date(trade.start_date);
        QuantLib::Date maturity = market::CurveBuilder::parse_date(trade.maturity_date);

        QuantLib::VanillaSwap::Type type = (trade.direction == "pay_fixed") ?
            QuantLib::VanillaSwap::Payer : QuantLib::VanillaSwap::Receiver;

        double fixed_rate = trade.details.at("fixed_rate").get<double>();
        std::string floating_index_name = trade.details.at("floating_index").get<std::string>();

        // For simplicity, we assume USD_LIBOR_3M and OIS discounting
        auto curve = market.get_curve("USD_OIS");
        QuantLib::Handle<QuantLib::YieldTermStructure> discounting_term_structure(curve);

        auto index = std::make_shared<QuantLib::USDLibor>(QuantLib::Period(3, QuantLib::Months), discounting_term_structure);

        auto swap = std::make_shared<QuantLib::VanillaSwap>(
            type, trade.notional,
            QuantLib::Schedule(start, maturity, QuantLib::Period(QuantLib::Semiannual), QuantLib::TARGET(),
                               QuantLib::ModifiedFollowing, QuantLib::ModifiedFollowing,
                               QuantLib::DateGeneration::Forward, false),
            fixed_rate, QuantLib::Thirty360(QuantLib::Thirty360::BondBasis),
            QuantLib::Schedule(start, maturity, QuantLib::Period(QuantLib::Quarterly), QuantLib::TARGET(),
                               QuantLib::ModifiedFollowing, QuantLib::ModifiedFollowing,
                               QuantLib::DateGeneration::Forward, false),
            index, QuantLib::Actual360());

        swap->setPricingEngine(std::make_shared<QuantLib::DiscountingSwapEngine>(discounting_term_structure));
        return swap;
    }

    static std::shared_ptr<QuantLib::Instrument> create_bond(
        const domain::Trade& trade,
        const market::MarketSnapshot& market) {

        QuantLib::Date start = market::CurveBuilder::parse_date(trade.start_date);
        QuantLib::Date maturity = market::CurveBuilder::parse_date(trade.maturity_date);

        double coupon_rate = trade.details.at("coupon_rate").get<double>();

        auto curve = market.get_curve("USD_OIS");
        QuantLib::Handle<QuantLib::YieldTermStructure> discounting_term_structure(curve);

        auto schedule = QuantLib::Schedule(start, maturity, QuantLib::Period(QuantLib::Annual), QuantLib::TARGET(),
                                            QuantLib::Unadjusted, QuantLib::Unadjusted,
                                            QuantLib::DateGeneration::Backward, false);

        auto bond = std::make_shared<QuantLib::FixedRateBond>(
            0, trade.notional, schedule, std::vector<QuantLib::Rate>{coupon_rate},
            QuantLib::Thirty360(QuantLib::Thirty360::BondBasis));

        bond->setPricingEngine(std::make_shared<QuantLib::DiscountingBondEngine>(discounting_term_structure));
        return bond;
    }
};

} // namespace qrp::instruments
