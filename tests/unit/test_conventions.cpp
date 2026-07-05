// Verifies market convention lookup, overrides, and pricing-context curve-family resolution.

#include <qrp/analytics/pricing_context.hpp>
#include <qrp/conventions/market_convention_registry.hpp>
#include <qrp/market/market_state.hpp>

#include <gtest/gtest.h>

#include <array>
#include <memory>

using namespace qrp;

TEST(ConventionTest, LoadsDefaultUsdIborConvention) {
    auto& registry = conventions::MarketConventionRegistry::instance();

    auto convention = registry.get_rates_convention(domain::Currency::USD, "IBOR_3M");

    EXPECT_EQ(convention.currency, domain::Currency::USD);
    EXPECT_EQ(convention.index_family, "IBOR_3M");
    EXPECT_EQ(convention.calendar, domain::BusinessCalendar::US);
    EXPECT_EQ(convention.fixed_leg_frequency, domain::Frequency::Semiannual);
    EXPECT_EQ(convention.floating_leg_frequency, domain::Frequency::Quarterly);
}

TEST(ConventionTest, FallsBackToCurrencyOisConvention) {
    auto& registry = conventions::MarketConventionRegistry::instance();

    auto convention = registry.get_rates_convention(domain::Currency::EUR, "MISSING_INDEX");

    EXPECT_EQ(convention.currency, domain::Currency::EUR);
    EXPECT_EQ(convention.index_family, "OIS");
    EXPECT_EQ(convention.calendar, domain::BusinessCalendar::Target);
}

TEST(ConventionTest, LoadsDefaultOisConventionsByCurrency) {
    auto& registry = conventions::MarketConventionRegistry::instance();

    struct ExpectedOisConvention {
        domain::Currency currency;
        domain::BusinessCalendar calendar;
        int settlement_days;
        domain::DayCount day_count;
    };

    const std::array<ExpectedOisConvention, 4> expected_conventions = {{
        {domain::Currency::USD, domain::BusinessCalendar::US, 2, domain::DayCount::ACT360},
        {domain::Currency::EUR, domain::BusinessCalendar::Target, 2, domain::DayCount::ACT360},
        {domain::Currency::GBP, domain::BusinessCalendar::UK, 0, domain::DayCount::ACT365},
        {domain::Currency::CHF, domain::BusinessCalendar::CHF, 2, domain::DayCount::ACT360},
    }};

    for (const auto& expected : expected_conventions) {
        auto convention = registry.get_rates_convention(expected.currency, "OIS");

        EXPECT_EQ(convention.currency, expected.currency);
        EXPECT_EQ(convention.index_family, "OIS");
        EXPECT_EQ(convention.calendar, expected.calendar);
        EXPECT_EQ(convention.settlement_days, expected.settlement_days);
        EXPECT_EQ(convention.day_count, expected.day_count);
        EXPECT_EQ(convention.fixed_leg_frequency, domain::Frequency::Annual);
        EXPECT_EQ(convention.floating_leg_frequency, domain::Frequency::Annual);
        EXPECT_EQ(convention.date_generation, domain::DateGeneration::Forward);
    }
}

TEST(ConventionTest, ReturnsMinimalConventionWhenNoCurrencyDefaultExists) {
    auto& registry = conventions::MarketConventionRegistry::instance();

    auto convention = registry.get_rates_convention(domain::Currency::UNKNOWN, "MISSING_INDEX");

    EXPECT_EQ(convention.currency, domain::Currency::UNKNOWN);
    EXPECT_EQ(convention.index_family, "MISSING_INDEX");
    EXPECT_EQ(convention.calendar, domain::BusinessCalendar::Target);
    EXPECT_EQ(convention.settlement_days, 2);
    EXPECT_EQ(convention.day_count, domain::DayCount::ACT360);
}

TEST(ConventionTest, RegistersCustomConvention) {
    auto& registry = conventions::MarketConventionRegistry::instance();

    conventions::RatesConvention custom;
    custom.currency = domain::Currency::JPY;
    custom.index_family = "CUSTOM_6M";
    custom.settlement_days = 3;
    custom.day_count = domain::DayCount::ACT365;
    registry.register_rates_convention(custom);

    auto convention = registry.get_rates_convention(domain::Currency::JPY, "CUSTOM_6M");

    EXPECT_EQ(convention.index_family, "CUSTOM_6M");
    EXPECT_EQ(convention.settlement_days, 3);
    EXPECT_EQ(convention.day_count, domain::DayCount::ACT365);
}

TEST(PricingContextTest, ResolvesStandardCurveFamilies) {
    auto state = std::make_shared<market::MarketState>(QuantLib::Date(24, QuantLib::March, 2024));
    analytics::PricingContext context(state);

    auto discount_id = context.get_discount_curve_id(domain::Currency::CHF);
    auto forecast_id = context.get_forecast_curve_id(domain::Currency::CHF, "IBOR_3M");

    EXPECT_EQ(discount_id.currency, domain::Currency::CHF);
    EXPECT_EQ(discount_id.family, "OIS");
    EXPECT_EQ(forecast_id.currency, domain::Currency::CHF);
    EXPECT_EQ(forecast_id.family, "IBOR_3M");
    EXPECT_EQ(context.market_state_ptr(), state);
}
