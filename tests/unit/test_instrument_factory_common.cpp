// Verifies shared internal helpers used by asset-class instrument factories.

#include <qrp/instruments/instrument_factory_common.hpp>

#include <gtest/gtest.h>

#include <ql/quotes/simplequote.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/time/calendars/nullcalendar.hpp>
#include <ql/time/daycounters/actual365fixed.hpp>
#include <string>
#include <vector>

namespace qrp::testing {
namespace {

TEST(InstrumentFactoryCommonTest, NormalizesDirectionsIndexesAndOptionLabels) {
    using namespace qrp::instruments;

    EXPECT_EQ(lower_copy("Pay_FIXED"), "pay_fixed");
    EXPECT_EQ(normalize_index_family("USD-SOFR"), "OIS");
    EXPECT_EQ(normalize_index_family("USD-LIBOR-6M"), "IBOR_6M");
    EXPECT_EQ(normalize_index_family("EURIBOR 1M"), "IBOR_1M");
    EXPECT_EQ(normalize_index_family(""), "IBOR_3M");
    EXPECT_EQ(parse_frequency_string("semi-annual", domain::Frequency::Monthly), domain::Frequency::Semiannual);
    EXPECT_EQ(parse_frequency_string("yearly", domain::Frequency::Monthly), domain::Frequency::Annual);
    EXPECT_EQ(parse_frequency_string("unknown", domain::Frequency::Quarterly), domain::Frequency::Quarterly);

    EXPECT_DOUBLE_EQ(deposit_direction_sign("borrower"), -1.0);
    EXPECT_DOUBLE_EQ(deposit_direction_sign("lend"), 1.0);
    EXPECT_EQ(fra_position_type("lender"), QuantLib::Position::Short);
    EXPECT_EQ(fra_position_type("borrower"), QuantLib::Position::Long);
    EXPECT_EQ(swap_type_from_direction("pay"), QuantLib::VanillaSwap::Payer);
    EXPECT_EQ(swap_type_from_direction("receive"), QuantLib::VanillaSwap::Receiver);
    EXPECT_DOUBLE_EQ(fx_direction_sign("sell_base"), -1.0);
    EXPECT_DOUBLE_EQ(fx_direction_sign("buy_base"), 1.0);

    EXPECT_TRUE(is_put_option("put_base"));
    EXPECT_FALSE(is_put_option("call"));
    EXPECT_DOUBLE_EQ(credit_bond_direction_sign("sell"), -1.0);
    EXPECT_DOUBLE_EQ(credit_bond_direction_sign("buy"), 1.0);
    EXPECT_DOUBLE_EQ(credit_option_direction_sign("written"), -1.0);
    EXPECT_DOUBLE_EQ(credit_option_direction_sign("long"), 1.0);
    EXPECT_DOUBLE_EQ(credit_protection_direction_sign("seller"), -1.0);
    EXPECT_DOUBLE_EQ(credit_protection_direction_sign("buyer"), 1.0);
    EXPECT_TRUE(is_credit_put_option("receiver"));
    EXPECT_FALSE(is_credit_put_option("payer"));
    EXPECT_DOUBLE_EQ(long_short_direction_sign("sold"), -1.0);
    EXPECT_DOUBLE_EQ(long_short_direction_sign("long"), 1.0);
    EXPECT_TRUE(is_american_exercise("American"));
    EXPECT_FALSE(is_american_exercise("European"));
}

TEST(InstrumentFactoryCommonTest, ResolvesTenorsMathAndCreditCurveHelpers) {
    using namespace qrp::instruments;

    EXPECT_EQ(fx_pair("EUR", "USD"), "EURUSD");
    EXPECT_EQ(index_tenor_from_family("IBOR_6M").units(), QuantLib::Months);
    EXPECT_EQ(index_tenor_from_family("IBOR_6M").length(), 6);
    EXPECT_EQ(index_tenor_from_family("BAD_TENOR").length(), 3);
    EXPECT_DOUBLE_EQ(futures_price_from_quote(97.25), 97.25);
    EXPECT_NEAR(futures_price_from_quote(0.0525), 94.75, 1.0e-12);
    EXPECT_NEAR(normal_cdf(0.0), 0.5, 1.0e-12);
    EXPECT_NEAR(normal_pdf(0.0), 0.3989422804014327, 1.0e-12);
    EXPECT_DOUBLE_EQ(tenor_years_or_fallback("", 2.0), 2.0);
    EXPECT_DOUBLE_EQ(tenor_years_or_fallback("bad", 3.0), 3.0);
    EXPECT_NEAR(tenor_years_or_fallback("6M", 1.0), 0.5, 1.0e-12);
    EXPECT_TRUE(contains_quote_type(domain::QuoteInstrumentType::CDS, {domain::QuoteInstrumentType::CDS}));
    EXPECT_FALSE(contains_quote_type(domain::QuoteInstrumentType::CDS, {domain::QuoteInstrumentType::Deposit}));
    EXPECT_DOUBLE_EQ(bounded_recovery(-0.1), 0.0);
    EXPECT_DOUBLE_EQ(bounded_recovery(0.4), 0.4);
    EXPECT_DOUBLE_EQ(bounded_recovery(0.99), 0.95);

    std::vector<CreditSpreadNode> nodes = {
        {1.0, QuantLib::ext::make_shared<QuantLib::SimpleQuote>(0.01)},
        {3.0, QuantLib::ext::make_shared<QuantLib::SimpleQuote>(0.03)},
    };
    EXPECT_DOUBLE_EQ(credit_spread_at({}, 2.0), 0.0);
    EXPECT_NEAR(credit_spread_at(nodes, 0.5), 0.01, 1.0e-12);
    EXPECT_NEAR(credit_spread_at(nodes, 2.0), 0.02, 1.0e-12);
    EXPECT_NEAR(credit_spread_at(nodes, 5.0), 0.03, 1.0e-12);
    EXPECT_GT(integrated_credit_spread(nodes, 2.0), 0.0);
    EXPECT_DOUBLE_EQ(integrated_credit_spread(nodes, 0.0), 0.0);
    EXPECT_LT(spread_discount_factor(nodes, 2.0), 1.0);
    EXPECT_LT(survival_probability(nodes, 0.4, 2.0), 1.0);
}

TEST(InstrumentFactoryCommonTest, MatchesCreditQuoteMetadata) {
    using namespace qrp::instruments;

    domain::MarketQuote spread_quote;
    spread_quote.id = "ACME_5Y";
    spread_quote.instrument_type = domain::QuoteInstrumentType::CDS;
    spread_quote.quote_type = domain::QuoteType::CreditSpread;
    spread_quote.tenor = "5Y";
    spread_quote.underlier = "ACME";

    EXPECT_TRUE(quote_matches_credit_underlier(spread_quote, "ACME", {domain::QuoteInstrumentType::CDS}));
    EXPECT_FALSE(quote_matches_credit_underlier(spread_quote, "MSFT", {domain::QuoteInstrumentType::CDS}));
    EXPECT_TRUE(quote_matches_underlier_and_tenor(spread_quote, "ACME", "5Y", {domain::QuoteInstrumentType::CDS}));
    EXPECT_TRUE(quote_matches_underlier_and_tenor(spread_quote, "ACME_5Y", "", {domain::QuoteInstrumentType::CDS}));
    EXPECT_FALSE(quote_matches_underlier_and_tenor(spread_quote, "ACME", "10Y", {domain::QuoteInstrumentType::CDS}));

    spread_quote.quote_type = domain::QuoteType::Volatility;
    EXPECT_FALSE(quote_matches_credit_underlier(spread_quote, "ACME", {domain::QuoteInstrumentType::CDS}));

    spread_quote.quote_type = domain::QuoteType::CreditSpread;
    spread_quote.instrument_type = domain::QuoteInstrumentType::RecoveryRate;
    EXPECT_FALSE(quote_matches_credit_underlier(spread_quote, "ACME", {domain::QuoteInstrumentType::RecoveryRate}));
}

TEST(InstrumentFactoryCommonTest, ResolvesMarketBackedCreditQuotesAndFallbacks) {
    using namespace qrp::instruments;

    auto make_quote = [](std::string id,
                         domain::QuoteInstrumentType instrument_type,
                         domain::QuoteType quote_type,
                         std::string tenor,
                         double value,
                         std::string underlier) {
        domain::MarketQuote quote;
        quote.id = std::move(id);
        quote.instrument_type = instrument_type;
        quote.quote_type = quote_type;
        quote.currency = domain::Currency::USD;
        quote.tenor = std::move(tenor);
        quote.value = value;
        quote.underlier = std::move(underlier);
        return quote;
    };

    domain::MarketSnapshot market_dto;
    market_dto.valuation_date = "2026-03-24";
    market_dto.base_currency = domain::Currency::USD;
    market_dto.quotes = {
        make_quote("ACME_CDS_1Y",
                   domain::QuoteInstrumentType::CDS,
                   domain::QuoteType::CreditSpread,
                   "1Y",
                   0.0100,
                   "ACME"),
        make_quote("ACME_CDS_5Y",
                   domain::QuoteInstrumentType::CDS,
                   domain::QuoteType::CreditSpread,
                   "5Y",
                   0.0125,
                   "ACME"),
        make_quote("ACME_VOL",
                   domain::QuoteInstrumentType::CreditSpread,
                   domain::QuoteType::Volatility,
                   "5Y",
                   0.35,
                   "ACME"),
        make_quote("ACME_RECOVERY",
                   domain::QuoteInstrumentType::RecoveryRate,
                   domain::QuoteType::RecoveryRate,
                   "SPOT",
                   0.42,
                   "ACME"),
    };

    market::MarketSnapshot market(market_dto);
    const analytics::PricingContext context(market.built_state());

    const auto explicit_quote = find_quote_handle(context, "ACME_CDS_5Y", "", "", {domain::QuoteInstrumentType::CDS});
    ASSERT_NE(explicit_quote, nullptr);
    EXPECT_DOUBLE_EQ(explicit_quote->value(), 0.0125);

    const auto metadata_quote = find_quote_handle(context, "", "ACME", "1Y", {domain::QuoteInstrumentType::CDS});
    ASSERT_NE(metadata_quote, nullptr);
    EXPECT_DOUBLE_EQ(metadata_quote->value(), 0.0100);
    EXPECT_EQ(find_quote_handle(context, "MISSING", "MSFT", "5Y", {domain::QuoteInstrumentType::CDS}), nullptr);

    const auto fallback_quote =
        quote_or_constant(context, "MISSING", "MSFT", {domain::QuoteInstrumentType::CDS}, 0.0175);
    ASSERT_NE(fallback_quote, nullptr);
    EXPECT_DOUBLE_EQ(fallback_quote->value(), 0.0175);

    const auto spread_nodes =
        make_credit_spread_curve(context, "ACME", "ACME_CDS_5Y", 0.0300, {domain::QuoteInstrumentType::CDS}, 5.0);
    ASSERT_EQ(spread_nodes.size(), 2U);
    EXPECT_NEAR(spread_nodes.front().years, 1.0, 1.0e-12);
    EXPECT_NEAR(spread_nodes.back().years, 5.0, 1.0e-12);
    EXPECT_DOUBLE_EQ(spread_nodes.back().quote->value(), 0.0125);

    const auto fallback_nodes =
        make_credit_spread_curve(context, "MSFT", "", -0.0100, {domain::QuoteInstrumentType::CDS}, 7.0);
    ASSERT_EQ(fallback_nodes.size(), 1U);
    EXPECT_DOUBLE_EQ(fallback_nodes.front().years, 7.0);
    EXPECT_DOUBLE_EQ(fallback_nodes.front().quote->value(), 0.0);

    const auto explicit_recovery = make_recovery_quote(context, "ACME", "ACME_RECOVERY", 0.40);
    ASSERT_NE(explicit_recovery, nullptr);
    EXPECT_DOUBLE_EQ(explicit_recovery->value(), 0.42);

    const auto matched_recovery = make_recovery_quote(context, "ACME", "", 0.40);
    ASSERT_NE(matched_recovery, nullptr);
    EXPECT_DOUBLE_EQ(matched_recovery->value(), 0.42);

    const auto fallback_recovery = make_recovery_quote(context, "MSFT", "", 0.35);
    ASSERT_NE(fallback_recovery, nullptr);
    EXPECT_DOUBLE_EQ(fallback_recovery->value(), 0.35);
}

TEST(InstrumentFactoryCommonTest, ResolvesSchedulesDiscountsVolatilityAndFxForwards) {
    using namespace qrp::instruments;

    domain::MarketQuote volatility_quote;
    volatility_quote.id = "AAPL_VOL";
    volatility_quote.instrument_type = domain::QuoteInstrumentType::EquityVol;
    volatility_quote.quote_type = domain::QuoteType::Volatility;
    volatility_quote.currency = domain::Currency::USD;
    volatility_quote.tenor = "1Y";
    volatility_quote.value = 0.24;

    domain::MarketSnapshot market_dto;
    market_dto.valuation_date = "2026-03-24";
    market_dto.base_currency = domain::Currency::USD;
    market_dto.quotes = {volatility_quote};

    market::MarketSnapshot market(market_dto);
    const analytics::PricingContext context(market.built_state());

    EXPECT_DOUBLE_EQ(quote_or_default_volatility(context, "AAPL_VOL", 0.20, 0.30), 0.24);
    EXPECT_DOUBLE_EQ(quote_or_default_volatility(context, "MISSING_VOL", 0.20, 0.30), 0.20);
    EXPECT_DOUBLE_EQ(quote_or_default_volatility(context, "MISSING_VOL", 0.0, 0.30), 0.30);

    const QuantLib::Date start(24, QuantLib::March, 2026);
    const QuantLib::Date maturity(24, QuantLib::March, 2028);
    QuantLib::Settings::instance().evaluationDate() = start;
    const auto curve = QuantLib::ext::make_shared<QuantLib::FlatForward>(start, 0.05, QuantLib::Actual365Fixed());
    curve->enableExtrapolation();

    EXPECT_DOUBLE_EQ(discount_factor_or_one(nullptr, maturity), 1.0);
    EXPECT_LT(discount_factor_or_one(curve, maturity), 1.0);

    const auto schedule =
        make_schedule(start, maturity, QuantLib::Annual, QuantLib::NullCalendar(), QuantLib::Unadjusted);
    EXPECT_GE(schedule.size(), 3U);
    EXPECT_GT(par_swap_rate(start, maturity, schedule, QuantLib::Actual365Fixed(), curve), 0.0);
    EXPECT_DOUBLE_EQ(par_swap_rate(start, maturity, schedule, QuantLib::Actual365Fixed(), nullptr), 0.0);
    EXPECT_GT(swap_annuity_at_exercise(start, schedule, QuantLib::Actual365Fixed(), curve, 1'000'000.0), 0.0);
    EXPECT_DOUBLE_EQ(swap_annuity_at_exercise(start, schedule, QuantLib::Actual365Fixed(), nullptr, 1'000'000.0), 0.0);

    const auto spot = QuantLib::ext::make_shared<QuantLib::SimpleQuote>(1.10);
    const auto outright_forward = QuantLib::ext::make_shared<QuantLib::SimpleQuote>(1.12);
    const auto forward_points = QuantLib::ext::make_shared<QuantLib::SimpleQuote>(0.015);

    EXPECT_DOUBLE_EQ(forward_rate_from_quotes(spot, outright_forward, nullptr, nullptr, nullptr, maturity), 1.12);
    EXPECT_DOUBLE_EQ(forward_rate_from_quotes(spot, nullptr, forward_points, nullptr, nullptr, maturity), 1.115);
    EXPECT_GT(forward_rate_from_quotes(spot, nullptr, nullptr, curve, nullptr, maturity), 1.10);
    EXPECT_DOUBLE_EQ(forward_rate_from_quotes(spot, nullptr, nullptr, nullptr, nullptr, maturity), 1.10);
}

} // namespace
} // namespace qrp::testing
