// Registers domain enums, trade DTOs, market DTOs, and factor DTOs with Python.

#include "bindings.hpp"

#include <qrp/domain/factors.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/portfolio.hpp>

#include <pybind11/stl.h>

// Exposes canonical domain DTOs, enums, and conversion helpers.

namespace py = pybind11;
using namespace qrp;

namespace qrp::bindings {

/**
 * @brief Registers domain enums and DTOs with the Python extension module.
 */
void bind_domain(py::module_& m) {
    py::enum_<domain::AssetClass>(m, "AssetClass")
        .value("Commodity", domain::AssetClass::Commodity)
        .value("Credit", domain::AssetClass::Credit)
        .value("Equity", domain::AssetClass::Equity)
        .value("FX", domain::AssetClass::FX)
        .value("Inflation", domain::AssetClass::Inflation)
        .value("Rates", domain::AssetClass::Rates)
        .value("Unknown", domain::AssetClass::Unknown);

    py::enum_<domain::BusinessCalendar>(m, "BusinessCalendar")
        .value("CHF", domain::BusinessCalendar::CHF)
        .value("JP", domain::BusinessCalendar::JP)
        .value("Target", domain::BusinessCalendar::Target)
        .value("UK", domain::BusinessCalendar::UK)
        .value("UNKNOWN", domain::BusinessCalendar::UNKNOWN)
        .value("US", domain::BusinessCalendar::US)
        .value("WeekendsOnly", domain::BusinessCalendar::WeekendsOnly);

    py::enum_<domain::BusinessDayConvention>(m, "BusinessDayConvention")
        .value("Following", domain::BusinessDayConvention::Following)
        .value("HalfMonthModifiedFollowing", domain::BusinessDayConvention::HalfMonthModifiedFollowing)
        .value("ModifiedFollowing", domain::BusinessDayConvention::ModifiedFollowing)
        .value("ModifiedPreceding", domain::BusinessDayConvention::ModifiedPreceding)
        .value("Nearest", domain::BusinessDayConvention::Nearest)
        .value("Preceding", domain::BusinessDayConvention::Preceding)
        .value("Unadjusted", domain::BusinessDayConvention::Unadjusted)
        .value("UNKNOWN", domain::BusinessDayConvention::UNKNOWN);

    py::enum_<domain::Currency>(m, "Currency")
        .value("CHF", domain::Currency::CHF)
        .value("EUR", domain::Currency::EUR)
        .value("GBP", domain::Currency::GBP)
        .value("JPY", domain::Currency::JPY)
        .value("UNKNOWN", domain::Currency::UNKNOWN)
        .value("USD", domain::Currency::USD);

    py::enum_<domain::CurvePurpose>(m, "CurvePurpose")
        .value("CommodityForward", domain::CurvePurpose::CommodityForward)
        .value("CommodityVolatility", domain::CurvePurpose::CommodityVolatility)
        .value("Credit", domain::CurvePurpose::Credit)
        .value("CreditSpread", domain::CurvePurpose::CreditSpread)
        .value("Discount", domain::CurvePurpose::Discount)
        .value("EquityBorrow", domain::CurvePurpose::EquityBorrow)
        .value("EquityDividend", domain::CurvePurpose::EquityDividend)
        .value("EquityVolatility", domain::CurvePurpose::EquityVolatility)
        .value("Forward", domain::CurvePurpose::Forward)
        .value("Forward3M", domain::CurvePurpose::Forward3M)
        .value("Forward6M", domain::CurvePurpose::Forward6M)
        .value("FXForward", domain::CurvePurpose::FXForward)
        .value("FXVolatility", domain::CurvePurpose::FXVolatility)
        .value("Hazard", domain::CurvePurpose::Hazard)
        .value("Inflation", domain::CurvePurpose::Inflation)
        .value("OISDiscount", domain::CurvePurpose::OISDiscount)
        .value("Recovery", domain::CurvePurpose::Recovery)
        .value("UNKNOWN", domain::CurvePurpose::UNKNOWN)
        .value("Volatility", domain::CurvePurpose::Volatility);

    py::enum_<domain::DateGeneration>(m, "DateGeneration")
        .value("Backward", domain::DateGeneration::Backward)
        .value("CDS", domain::DateGeneration::CDS)
        .value("CDS2015", domain::DateGeneration::CDS2015)
        .value("Forward", domain::DateGeneration::Forward)
        .value("OldCDS", domain::DateGeneration::OldCDS)
        .value("ThirdWednesday", domain::DateGeneration::ThirdWednesday)
        .value("Twentieth", domain::DateGeneration::Twentieth)
        .value("TwentiethIMM", domain::DateGeneration::TwentiethIMM)
        .value("UNKNOWN", domain::DateGeneration::UNKNOWN)
        .value("Zero", domain::DateGeneration::Zero);

    py::enum_<domain::DayCount>(m, "DayCount")
        .value("ACT360", domain::DayCount::ACT360)
        .value("ACT365", domain::DayCount::ACT365)
        .value("ACT365F", domain::DayCount::ACT365F)
        .value("ACTACT", domain::DayCount::ACTACT)
        .value("ACTACT_AFB", domain::DayCount::ACTACT_AFB)
        .value("ACTACT_EURO", domain::DayCount::ACTACT_EURO)
        .value("ACTACT_ISDA", domain::DayCount::ACTACT_ISDA)
        .value("ACTACT_ISMA", domain::DayCount::ACTACT_ISMA)
        .value("Thirty360", domain::DayCount::Thirty360)
        .value("UNKNOWN", domain::DayCount::UNKNOWN);

    py::enum_<domain::FactorType>(m, "FactorType")
        .value("CommodityForward", domain::FactorType::CommodityForward)
        .value("CommoditySpot", domain::FactorType::CommoditySpot)
        .value("CreditRecovery", domain::FactorType::CreditRecovery)
        .value("CreditSpread", domain::FactorType::CreditSpread)
        .value("HazardRate", domain::FactorType::HazardRate)
        .value("EquityBorrowRate", domain::FactorType::EquityBorrowRate)
        .value("EquityDividendYield", domain::FactorType::EquityDividendYield)
        .value("EquityForward", domain::FactorType::EquityForward)
        .value("EquitySpot", domain::FactorType::EquitySpot)
        .value("FXForwardPoint", domain::FactorType::FXForwardPoint)
        .value("FXSpot", domain::FactorType::FXSpot)
        .value("Custom", domain::FactorType::Custom)
        .value("Volatility", domain::FactorType::Volatility)
        .value("BasisSpread", domain::FactorType::BasisSpread)
        .value("RateForward", domain::FactorType::RateForward)
        .value("RateZero", domain::FactorType::RateZero);

    py::enum_<domain::Frequency>(m, "Frequency")
        .value("Annual", domain::Frequency::Annual)
        .value("Bimonthly", domain::Frequency::Bimonthly)
        .value("Biweekly", domain::Frequency::Biweekly)
        .value("Daily", domain::Frequency::Daily)
        .value("EveryFourthMonth", domain::Frequency::EveryFourthMonth)
        .value("EveryFourthWeek", domain::Frequency::EveryFourthWeek)
        .value("Monthly", domain::Frequency::Monthly)
        .value("Once", domain::Frequency::Once)
        .value("OtherFrequency", domain::Frequency::OtherFrequency)
        .value("Quarterly", domain::Frequency::Quarterly)
        .value("Semiannual", domain::Frequency::Semiannual)
        .value("UNKNOWN", domain::Frequency::UNKNOWN)
        .value("Weekly", domain::Frequency::Weekly);

    py::enum_<domain::InterpolationType>(m, "InterpolationType")
        .value("CubicSpline", domain::InterpolationType::CubicSpline)
        .value("Linear", domain::InterpolationType::Linear)
        .value("LogLinear", domain::InterpolationType::LogLinear)
        .value("UNKNOWN", domain::InterpolationType::UNKNOWN);

    py::enum_<domain::ProductType>(m, "ProductType")
        .value("CommoditySpot", domain::ProductType::CommoditySpot)
        .value("CommodityForward", domain::ProductType::CommodityForward)
        .value("CommodityFuture", domain::ProductType::CommodityFuture)
        .value("CommodityFutureStrip", domain::ProductType::CommodityFutureStrip)
        .value("CommodityFutureOption", domain::ProductType::CommodityFutureOption)
        .value("CommodityCalendarSpreadOption", domain::ProductType::CommodityCalendarSpreadOption)
        .value("CommoditySwing", domain::ProductType::CommoditySwing)
        .value("CreditBond", domain::ProductType::CreditBond)
        .value("Cds", domain::ProductType::Cds)
        .value("CdsIndex", domain::ProductType::CdsIndex)
        .value("CdsOption", domain::ProductType::CdsOption)
        .value("CreditIndexOption", domain::ProductType::CreditIndexOption)
        .value("EquitySpot", domain::ProductType::EquitySpot)
        .value("EquityForward", domain::ProductType::EquityForward)
        .value("EquityFuture", domain::ProductType::EquityFuture)
        .value("EquityOption", domain::ProductType::EquityOption)
        .value("FxSpot", domain::ProductType::FxSpot)
        .value("FxForward", domain::ProductType::FxForward)
        .value("FxSwap", domain::ProductType::FxSwap)
        .value("Ndf", domain::ProductType::Ndf)
        .value("FxOption", domain::ProductType::FxOption)
        .value("CrossCurrencySwap", domain::ProductType::CrossCurrencySwap)
        .value("Deposit", domain::ProductType::Deposit)
        .value("Fra", domain::ProductType::Fra)
        .value("InterestRateFuture", domain::ProductType::InterestRateFuture)
        .value("VanillaSwap", domain::ProductType::VanillaSwap)
        .value("OisSwap", domain::ProductType::OisSwap)
        .value("FixedRateBond", domain::ProductType::FixedRateBond)
        .value("FloatingRateNote", domain::ProductType::FloatingRateNote)
        .value("CapFloor", domain::ProductType::CapFloor)
        .value("EuropeanSwaption", domain::ProductType::EuropeanSwaption)
        .value("BermudanSwaption", domain::ProductType::BermudanSwaption)
        .value("CallableBond", domain::ProductType::CallableBond)
        .value("Unknown", domain::ProductType::Unknown);

    py::enum_<domain::QuoteInstrumentType>(m, "QuoteInstrumentType")
        .value("Bond", domain::QuoteInstrumentType::Bond)
        .value("BondPrice", domain::QuoteInstrumentType::BondPrice)
        .value("BondSpread", domain::QuoteInstrumentType::BondSpread)
        .value("BorrowRate", domain::QuoteInstrumentType::BorrowRate)
        .value("CapFloorVol", domain::QuoteInstrumentType::CapFloorVol)
        .value("CDS", domain::QuoteInstrumentType::CDS)
        .value("CommodityForward", domain::QuoteInstrumentType::CommodityForward)
        .value("CommodityFuture", domain::QuoteInstrumentType::CommodityFuture)
        .value("CommoditySpot", domain::QuoteInstrumentType::CommoditySpot)
        .value("CommodityVol", domain::QuoteInstrumentType::CommodityVol)
        .value("ConvenienceYield", domain::QuoteInstrumentType::ConvenienceYield)
        .value("CreditIndex", domain::QuoteInstrumentType::CreditIndex)
        .value("CreditSpread", domain::QuoteInstrumentType::CreditSpread)
        .value("Deposit", domain::QuoteInstrumentType::Deposit)
        .value("DividendYield", domain::QuoteInstrumentType::DividendYield)
        .value("EquitySpot", domain::QuoteInstrumentType::EquitySpot)
        .value("EquityVol", domain::QuoteInstrumentType::EquityVol)
        .value("FRA", domain::QuoteInstrumentType::FRA)
        .value("Future", domain::QuoteInstrumentType::Future)
        .value("FXForward", domain::QuoteInstrumentType::FXForward)
        .value("FXForwardPoint", domain::QuoteInstrumentType::FXForwardPoint)
        .value("FXSpot", domain::QuoteInstrumentType::FXSpot)
        .value("FXVol", domain::QuoteInstrumentType::FXVol)
        .value("HazardRate", domain::QuoteInstrumentType::HazardRate)
        .value("InterestRateFuture", domain::QuoteInstrumentType::InterestRateFuture)
        .value("IRS", domain::QuoteInstrumentType::IRS)
        .value("OIS", domain::QuoteInstrumentType::OIS)
        .value("RecoveryRate", domain::QuoteInstrumentType::RecoveryRate)
        .value("SwaptionVol", domain::QuoteInstrumentType::SwaptionVol)
        .value("UNKNOWN", domain::QuoteInstrumentType::UNKNOWN);

    py::enum_<domain::QuoteType>(m, "QuoteType")
        .value("BasisSwap", domain::QuoteType::BasisSwap)
        .value("BondYield", domain::QuoteType::BondYield)
        .value("BorrowRate", domain::QuoteType::BorrowRate)
        .value("CommodityForward", domain::QuoteType::CommodityForward)
        .value("CreditSpread", domain::QuoteType::CreditSpread)
        .value("Deposit", domain::QuoteType::Deposit)
        .value("DividendYield", domain::QuoteType::DividendYield)
        .value("EquitySpot", domain::QuoteType::EquitySpot)
        .value("FRA", domain::QuoteType::FRA)
        .value("Future", domain::QuoteType::Future)
        .value("FXForward", domain::QuoteType::FXForward)
        .value("FXForwardPoint", domain::QuoteType::FXForwardPoint)
        .value("FXSpot", domain::QuoteType::FXSpot)
        .value("HazardRate", domain::QuoteType::HazardRate)
        .value("InterestRateFuture", domain::QuoteType::InterestRateFuture)
        .value("IRS", domain::QuoteType::IRS)
        .value("OIS", domain::QuoteType::OIS)
        .value("Price", domain::QuoteType::Price)
        .value("RecoveryRate", domain::QuoteType::RecoveryRate)
        .value("Swap", domain::QuoteType::Swap)
        .value("UNKNOWN", domain::QuoteType::UNKNOWN)
        .value("Volatility", domain::QuoteType::Volatility);

    py::enum_<domain::ShockMeasure>(m, "ShockMeasure")
        .value("Absolute", domain::ShockMeasure::Absolute)
        .value("BasisPoints", domain::ShockMeasure::BasisPoints)
        .value("LogReturn", domain::ShockMeasure::LogReturn)
        .value("Relative", domain::ShockMeasure::Relative)
        .value("VolPoints", domain::ShockMeasure::VolPoints);

    py::enum_<domain::SupportStatus>(m, "SupportStatus")
        .value("Failed", domain::SupportStatus::Failed)
        .value("PartiallySupported", domain::SupportStatus::PartiallySupported)
        .value("Supported", domain::SupportStatus::Supported)
        .value("Unsupported", domain::SupportStatus::Unsupported);

    py::enum_<domain::TradeType>(m, "TradeType")
        .value("CommoditySpot", domain::TradeType::CommoditySpot)
        .value("CommodityForward", domain::TradeType::CommodityForward)
        .value("CommodityFuture", domain::TradeType::CommodityFuture)
        .value("CommodityFutureStrip", domain::TradeType::CommodityFutureStrip)
        .value("CommodityFutureOption", domain::TradeType::CommodityFutureOption)
        .value("CommodityCalendarSpreadOption", domain::TradeType::CommodityCalendarSpreadOption)
        .value("CommoditySwing", domain::TradeType::CommoditySwing)
        .value("CreditBond", domain::TradeType::CreditBond)
        .value("Cds", domain::TradeType::Cds)
        .value("CdsIndex", domain::TradeType::CdsIndex)
        .value("CdsOption", domain::TradeType::CdsOption)
        .value("CreditIndexOption", domain::TradeType::CreditIndexOption)
        .value("EquitySpot", domain::TradeType::EquitySpot)
        .value("EquityForward", domain::TradeType::EquityForward)
        .value("EquityFuture", domain::TradeType::EquityFuture)
        .value("EquityOption", domain::TradeType::EquityOption)
        .value("FxSpot", domain::TradeType::FxSpot)
        .value("FxForward", domain::TradeType::FxForward)
        .value("FxSwap", domain::TradeType::FxSwap)
        .value("Ndf", domain::TradeType::Ndf)
        .value("FxOption", domain::TradeType::FxOption)
        .value("Deposit", domain::TradeType::Deposit)
        .value("Fra", domain::TradeType::Fra)
        .value("InterestRateFuture", domain::TradeType::InterestRateFuture)
        .value("VanillaSwap", domain::TradeType::VanillaSwap)
        .value("OisSwap", domain::TradeType::OisSwap)
        .value("FixedRateBond", domain::TradeType::FixedRateBond)
        .value("CallableBond", domain::TradeType::CallableBond)
        .value("FloatingRateNote", domain::TradeType::FloatingRateNote)
        .value("CapFloor", domain::TradeType::CapFloor)
        .value("EuropeanSwaption", domain::TradeType::EuropeanSwaption)
        .value("BermudanSwaption", domain::TradeType::BermudanSwaption)
        .value("Unknown", domain::TradeType::Unknown);

    m.def("is_canonical_factor_id", &domain::is_canonical_factor_id);
    m.def("make_commodity_forward_factor_id", &domain::make_commodity_forward_factor_id);
    m.def("make_commodity_vol_factor_id", &domain::make_commodity_vol_factor_id);
    m.def("make_credit_spread_factor_id", &domain::make_credit_spread_factor_id);
    m.def("make_credit_vol_factor_id", &domain::make_credit_vol_factor_id);
    m.def("make_equity_spot_factor_id", &domain::make_equity_spot_factor_id);
    m.def("make_equity_vol_factor_id", &domain::make_equity_vol_factor_id);
    m.def("make_fx_spot_factor_id", &domain::make_fx_spot_factor_id);
    m.def("make_fx_vol_factor_id", &domain::make_fx_vol_factor_id);
    m.def("make_rates_factor_id", &domain::make_rates_factor_id);
    m.def("make_rates_vol_factor_id", &domain::make_rates_vol_factor_id);
    py::class_<domain::CurveId>(m, "CurveId")
        .def(py::init<>())
        .def_readwrite("currency", &domain::CurveId::currency)
        .def_readwrite("family", &domain::CurveId::family);

    py::class_<domain::CurveSpec>(m, "CurveSpec")
        .def(py::init<>())
        .def_readwrite("calendar", &domain::CurveSpec::calendar)
        .def_readwrite("collateral_curve_id", &domain::CurveSpec::collateral_curve_id)
        .def_readwrite("construction_family", &domain::CurveSpec::construction_family)
        .def_readwrite("day_count", &domain::CurveSpec::day_count)
        .def_readwrite("discount_curve_id", &domain::CurveSpec::discount_curve_id)
        .def_readwrite("id", &domain::CurveSpec::id)
        .def_readwrite("interpolation", &domain::CurveSpec::interpolation)
        .def_readwrite("metadata_json", &domain::CurveSpec::metadata_json)
        .def_readwrite("purpose", &domain::CurveSpec::purpose)
        .def_readwrite("quote_ids", &domain::CurveSpec::quote_ids);

    py::class_<domain::FactorBinding>(m, "FactorBinding")
        .def(py::init<>())
        .def_readwrite("factor_id", &domain::FactorBinding::factor_id)
        .def_readwrite("quote_id", &domain::FactorBinding::quote_id)
        .def_readwrite("selector_json", &domain::FactorBinding::selector_json)
        .def_readwrite("shock_measure", &domain::FactorBinding::shock_measure)
        .def_readwrite("transform", &domain::FactorBinding::transform)
        .def_readwrite("weight", &domain::FactorBinding::weight);

    py::class_<domain::FactorDefinition>(m, "FactorDefinition")
        .def(py::init<>())
        .def_readwrite("currency", &domain::FactorDefinition::currency)
        .def_readwrite("curve_id", &domain::FactorDefinition::curve_id)
        .def_readwrite("description", &domain::FactorDefinition::description)
        .def_readwrite("factor_id", &domain::FactorDefinition::factor_id)
        .def_readwrite("factor_type", &domain::FactorDefinition::factor_type)
        .def_readwrite("quote_ids", &domain::FactorDefinition::quote_ids)
        .def_readwrite("shock_measure", &domain::FactorDefinition::shock_measure)
        .def_readwrite("tenor", &domain::FactorDefinition::tenor);

    py::class_<domain::FactorObservation>(m, "FactorObservation")
        .def(py::init<>())
        .def_readwrite("factor_id", &domain::FactorObservation::factor_id)
        .def_readwrite("level", &domain::FactorObservation::level)
        .def_readwrite("market_date", &domain::FactorObservation::market_date)
        .def_readwrite("move", &domain::FactorObservation::move)
        .def_readwrite("move_unit", &domain::FactorObservation::move_unit);

    py::class_<domain::MarketDataDiagnostic>(m, "MarketDataDiagnostic")
        .def(py::init<>())
        .def_readwrite("code", &domain::MarketDataDiagnostic::code)
        .def_readwrite("curve_id", &domain::MarketDataDiagnostic::curve_id)
        .def_readwrite("message", &domain::MarketDataDiagnostic::message)
        .def_readwrite("quote_id", &domain::MarketDataDiagnostic::quote_id)
        .def_readwrite("severity", &domain::MarketDataDiagnostic::severity);

    py::class_<domain::MarketQuote>(m, "MarketQuote")
        .def(py::init<>())
        .def_readwrite("bdc", &domain::MarketQuote::bdc)
        .def_readwrite("calendar", &domain::MarketQuote::calendar)
        .def_readwrite("currency", &domain::MarketQuote::currency)
        .def_readwrite("day_count", &domain::MarketQuote::day_count)
        .def_readwrite("expiry", &domain::MarketQuote::expiry)
        .def_readwrite("id", &domain::MarketQuote::id)
        .def_readwrite("index_family", &domain::MarketQuote::index_family)
        .def_readwrite("instrument_family", &domain::MarketQuote::instrument_family)
        .def_readwrite("instrument_type", &domain::MarketQuote::instrument_type)
        .def_readwrite("market_ts", &domain::MarketQuote::market_ts)
        .def_readwrite("quote_type", &domain::MarketQuote::quote_type)
        .def_readwrite("recorded_ts", &domain::MarketQuote::recorded_ts)
        .def_readwrite("risk_factor_id", &domain::MarketQuote::risk_factor_id)
        .def_readwrite("settlement_days", &domain::MarketQuote::settlement_days)
        .def_readwrite("source_name", &domain::MarketQuote::source_name)
        .def_readwrite("source_ts", &domain::MarketQuote::source_ts)
        .def_readwrite("stale_after_days", &domain::MarketQuote::stale_after_days)
        .def_readwrite("strike", &domain::MarketQuote::strike)
        .def_readwrite("tenor", &domain::MarketQuote::tenor)
        .def_readwrite("underlier", &domain::MarketQuote::underlier)
        .def_readwrite("value", &domain::MarketQuote::value);

    py::class_<domain::MarketSnapshot>(m, "MarketSnapshot")
        .def(py::init<>())
        .def_readwrite("base_currency", &domain::MarketSnapshot::base_currency)
        .def_readwrite("curves", &domain::MarketSnapshot::curves)
        .def_readwrite("default_stale_after_days", &domain::MarketSnapshot::default_stale_after_days)
        .def_readwrite("diagnostics", &domain::MarketSnapshot::diagnostics)
        .def_readwrite("fixings", &domain::MarketSnapshot::fixings)
        .def_readwrite("quotes", &domain::MarketSnapshot::quotes)
        .def_readwrite("recorded_ts", &domain::MarketSnapshot::recorded_ts)
        .def_readwrite("schema_version", &domain::MarketSnapshot::schema_version)
        .def_readwrite("snapshot_id", &domain::MarketSnapshot::snapshot_id)
        .def_readwrite("source_name", &domain::MarketSnapshot::source_name)
        .def_readwrite("valuation_date", &domain::MarketSnapshot::valuation_date);

    m.def("blocking_market_snapshot_diagnostics", &domain::blocking_market_snapshot_diagnostics);
    m.def("collect_market_snapshot_diagnostics", &domain::collect_market_snapshot_diagnostics);
    m.def("format_market_data_diagnostic", &domain::format_market_data_diagnostic);
    m.def("is_blocking_market_data_diagnostic", &domain::is_blocking_market_data_diagnostic);
    m.def("throw_if_market_snapshot_not_ready", &domain::throw_if_market_snapshot_not_ready);
    m.def("validate_market_snapshot", &domain::validate_market_snapshot);

    // Register the base trade before the asset-class derived DTOs.
    py::class_<domain::Trade, std::shared_ptr<domain::Trade>>(m, "Trade")
        .def_readwrite("asset_class", &domain::Trade::asset_class)
        .def_readwrite("asset_class_type", &domain::Trade::asset_class_type)
        .def_readwrite("book", &domain::Trade::book)
        .def_readwrite("currency", &domain::Trade::currency)
        .def_readwrite("direction", &domain::Trade::direction)
        .def_readwrite("id", &domain::Trade::id)
        .def_readwrite("product_type", &domain::Trade::product_type)
        .def_readwrite("strategy", &domain::Trade::strategy)
        .def_readwrite("trade_type", &domain::Trade::trade_type)
        .def_property(
            "type",
            [](const domain::Trade& trade) { return trade.type; },
            [](domain::Trade& trade, const std::string& type) {
                trade.trade_type = domain::parse_trade_type(type);
                trade.product_type = domain::product_type_from_trade_type(trade.trade_type);
                trade.type = domain::to_string(trade.trade_type);
            });

    py::class_<domain::BermudanSwaptionTrade, domain::Trade, std::shared_ptr<domain::BermudanSwaptionTrade>>(
        m,
        "BermudanSwaptionTrade")
        .def(py::init<>())
        .def_readwrite("exercise_dates", &domain::BermudanSwaptionTrade::exercise_dates)
        .def_readwrite("fixed_rate", &domain::BermudanSwaptionTrade::fixed_rate)
        .def_readwrite("floating_index", &domain::BermudanSwaptionTrade::floating_index)
        .def_readwrite("maturity_date", &domain::BermudanSwaptionTrade::maturity_date)
        .def_readwrite("mean_reversion", &domain::BermudanSwaptionTrade::mean_reversion)
        .def_readwrite("notional", &domain::BermudanSwaptionTrade::notional)
        .def_readwrite("start_date", &domain::BermudanSwaptionTrade::start_date)
        .def_readwrite("volatility", &domain::BermudanSwaptionTrade::volatility)
        .def_readwrite("volatility_quote_id", &domain::BermudanSwaptionTrade::volatility_quote_id);

    py::class_<domain::CapFloorTrade, domain::Trade, std::shared_ptr<domain::CapFloorTrade>>(m, "CapFloorTrade")
        .def(py::init<>())
        .def_readwrite("cap_floor_type", &domain::CapFloorTrade::cap_floor_type)
        .def_readwrite("floating_index", &domain::CapFloorTrade::floating_index)
        .def_readwrite("maturity_date", &domain::CapFloorTrade::maturity_date)
        .def_readwrite("notional", &domain::CapFloorTrade::notional)
        .def_readwrite("start_date", &domain::CapFloorTrade::start_date)
        .def_readwrite("strike_rate", &domain::CapFloorTrade::strike_rate)
        .def_readwrite("volatility", &domain::CapFloorTrade::volatility)
        .def_readwrite("volatility_quote_id", &domain::CapFloorTrade::volatility_quote_id);

    py::class_<domain::CdsTrade, domain::Trade, std::shared_ptr<domain::CdsTrade>>(m, "CdsTrade")
        .def(py::init<>())
        .def_readwrite("coupon_rate", &domain::CdsTrade::coupon_rate)
        .def_readwrite("frequency", &domain::CdsTrade::frequency)
        .def_readwrite("issuer", &domain::CdsTrade::issuer)
        .def_readwrite("maturity_date", &domain::CdsTrade::maturity_date)
        .def_readwrite("notional", &domain::CdsTrade::notional)
        .def_readwrite("recovery_quote_id", &domain::CdsTrade::recovery_quote_id)
        .def_readwrite("recovery_rate", &domain::CdsTrade::recovery_rate)
        .def_readwrite("spread_quote_id", &domain::CdsTrade::spread_quote_id)
        .def_readwrite("start_date", &domain::CdsTrade::start_date);

    py::class_<domain::CdsIndexTrade, domain::Trade, std::shared_ptr<domain::CdsIndexTrade>>(m, "CdsIndexTrade")
        .def(py::init<>())
        .def_readwrite("coupon_rate", &domain::CdsIndexTrade::coupon_rate)
        .def_readwrite("frequency", &domain::CdsIndexTrade::frequency)
        .def_readwrite("index_factor", &domain::CdsIndexTrade::index_factor)
        .def_readwrite("index_name", &domain::CdsIndexTrade::index_name)
        .def_readwrite("maturity_date", &domain::CdsIndexTrade::maturity_date)
        .def_readwrite("notional", &domain::CdsIndexTrade::notional)
        .def_readwrite("recovery_quote_id", &domain::CdsIndexTrade::recovery_quote_id)
        .def_readwrite("recovery_rate", &domain::CdsIndexTrade::recovery_rate)
        .def_readwrite("spread_quote_id", &domain::CdsIndexTrade::spread_quote_id)
        .def_readwrite("start_date", &domain::CdsIndexTrade::start_date);

    py::class_<domain::CdsOptionTrade, domain::Trade, std::shared_ptr<domain::CdsOptionTrade>>(m, "CdsOptionTrade")
        .def(py::init<>())
        .def_readwrite("expiry_date", &domain::CdsOptionTrade::expiry_date)
        .def_readwrite("frequency", &domain::CdsOptionTrade::frequency)
        .def_readwrite("issuer", &domain::CdsOptionTrade::issuer)
        .def_readwrite("maturity_date", &domain::CdsOptionTrade::maturity_date)
        .def_readwrite("notional", &domain::CdsOptionTrade::notional)
        .def_readwrite("option_type", &domain::CdsOptionTrade::option_type)
        .def_readwrite("recovery_quote_id", &domain::CdsOptionTrade::recovery_quote_id)
        .def_readwrite("recovery_rate", &domain::CdsOptionTrade::recovery_rate)
        .def_readwrite("spread_quote_id", &domain::CdsOptionTrade::spread_quote_id)
        .def_readwrite("strike_spread", &domain::CdsOptionTrade::strike_spread)
        .def_readwrite("volatility", &domain::CdsOptionTrade::volatility)
        .def_readwrite("volatility_quote_id", &domain::CdsOptionTrade::volatility_quote_id);

    py::class_<domain::CommodityCalendarSpreadOptionTrade,
               domain::Trade,
               std::shared_ptr<domain::CommodityCalendarSpreadOptionTrade>>(m, "CommodityCalendarSpreadOptionTrade")
        .def(py::init<>())
        .def_readwrite("contract_size", &domain::CommodityCalendarSpreadOptionTrade::contract_size)
        .def_readwrite("expiry_date", &domain::CommodityCalendarSpreadOptionTrade::expiry_date)
        .def_readwrite("far_future_quote_id", &domain::CommodityCalendarSpreadOptionTrade::far_future_quote_id)
        .def_readwrite("near_future_quote_id", &domain::CommodityCalendarSpreadOptionTrade::near_future_quote_id)
        .def_readwrite("option_type", &domain::CommodityCalendarSpreadOptionTrade::option_type)
        .def_readwrite("quantity", &domain::CommodityCalendarSpreadOptionTrade::quantity)
        .def_readwrite("strike_spread", &domain::CommodityCalendarSpreadOptionTrade::strike_spread)
        .def_readwrite("underlier", &domain::CommodityCalendarSpreadOptionTrade::underlier)
        .def_readwrite("unit", &domain::CommodityCalendarSpreadOptionTrade::unit)
        .def_readwrite("volatility", &domain::CommodityCalendarSpreadOptionTrade::volatility)
        .def_readwrite("volatility_quote_id", &domain::CommodityCalendarSpreadOptionTrade::volatility_quote_id);

    py::class_<domain::CommodityForwardTrade, domain::Trade, std::shared_ptr<domain::CommodityForwardTrade>>(
        m,
        "CommodityForwardTrade")
        .def(py::init<>())
        .def_readwrite("contract_price", &domain::CommodityForwardTrade::contract_price)
        .def_readwrite("forward_quote_id", &domain::CommodityForwardTrade::forward_quote_id)
        .def_readwrite("maturity_date", &domain::CommodityForwardTrade::maturity_date)
        .def_readwrite("quantity", &domain::CommodityForwardTrade::quantity)
        .def_readwrite("spot_quote_id", &domain::CommodityForwardTrade::spot_quote_id)
        .def_readwrite("tenor", &domain::CommodityForwardTrade::tenor)
        .def_readwrite("underlier", &domain::CommodityForwardTrade::underlier)
        .def_readwrite("unit", &domain::CommodityForwardTrade::unit);

    py::class_<domain::CommodityFutureTrade, domain::Trade, std::shared_ptr<domain::CommodityFutureTrade>>(
        m,
        "CommodityFutureTrade")
        .def(py::init<>())
        .def_readwrite("contract_size", &domain::CommodityFutureTrade::contract_size)
        .def_readwrite("future_quote_id", &domain::CommodityFutureTrade::future_quote_id)
        .def_readwrite("maturity_date", &domain::CommodityFutureTrade::maturity_date)
        .def_readwrite("quantity", &domain::CommodityFutureTrade::quantity)
        .def_readwrite("reference_price", &domain::CommodityFutureTrade::reference_price)
        .def_readwrite("tenor", &domain::CommodityFutureTrade::tenor)
        .def_readwrite("underlier", &domain::CommodityFutureTrade::underlier)
        .def_readwrite("unit", &domain::CommodityFutureTrade::unit);

    py::class_<domain::CommodityFutureOptionTrade, domain::Trade, std::shared_ptr<domain::CommodityFutureOptionTrade>>(
        m,
        "CommodityFutureOptionTrade")
        .def(py::init<>())
        .def_readwrite("contract_size", &domain::CommodityFutureOptionTrade::contract_size)
        .def_readwrite("expiry_date", &domain::CommodityFutureOptionTrade::expiry_date)
        .def_readwrite("future_quote_id", &domain::CommodityFutureOptionTrade::future_quote_id)
        .def_readwrite("maturity_date", &domain::CommodityFutureOptionTrade::maturity_date)
        .def_readwrite("option_type", &domain::CommodityFutureOptionTrade::option_type)
        .def_readwrite("quantity", &domain::CommodityFutureOptionTrade::quantity)
        .def_readwrite("strike_price", &domain::CommodityFutureOptionTrade::strike_price)
        .def_readwrite("tenor", &domain::CommodityFutureOptionTrade::tenor)
        .def_readwrite("underlier", &domain::CommodityFutureOptionTrade::underlier)
        .def_readwrite("unit", &domain::CommodityFutureOptionTrade::unit)
        .def_readwrite("volatility", &domain::CommodityFutureOptionTrade::volatility)
        .def_readwrite("volatility_quote_id", &domain::CommodityFutureOptionTrade::volatility_quote_id);

    py::class_<domain::CommodityFutureStripTrade, domain::Trade, std::shared_ptr<domain::CommodityFutureStripTrade>>(
        m,
        "CommodityFutureStripTrade")
        .def(py::init<>())
        .def_readwrite("contract_size", &domain::CommodityFutureStripTrade::contract_size)
        .def_readwrite("future_quote_ids", &domain::CommodityFutureStripTrade::future_quote_ids)
        .def_readwrite("maturity_date", &domain::CommodityFutureStripTrade::maturity_date)
        .def_readwrite("quantity", &domain::CommodityFutureStripTrade::quantity)
        .def_readwrite("reference_price", &domain::CommodityFutureStripTrade::reference_price)
        .def_readwrite("start_date", &domain::CommodityFutureStripTrade::start_date)
        .def_readwrite("underlier", &domain::CommodityFutureStripTrade::underlier)
        .def_readwrite("unit", &domain::CommodityFutureStripTrade::unit)
        .def_readwrite("weights", &domain::CommodityFutureStripTrade::weights);

    py::class_<domain::CommoditySpotTrade, domain::Trade, std::shared_ptr<domain::CommoditySpotTrade>>(
        m,
        "CommoditySpotTrade")
        .def(py::init<>())
        .def_readwrite("quantity", &domain::CommoditySpotTrade::quantity)
        .def_readwrite("reference_price", &domain::CommoditySpotTrade::reference_price)
        .def_readwrite("spot_quote_id", &domain::CommoditySpotTrade::spot_quote_id)
        .def_readwrite("underlier", &domain::CommoditySpotTrade::underlier)
        .def_readwrite("unit", &domain::CommoditySpotTrade::unit);

    py::class_<domain::CommoditySwingTrade, domain::Trade, std::shared_ptr<domain::CommoditySwingTrade>>(
        m,
        "CommoditySwingTrade")
        .def(py::init<>())
        .def_readwrite("exercise_dates", &domain::CommoditySwingTrade::exercise_dates)
        .def_readwrite("forward_quote_ids", &domain::CommoditySwingTrade::forward_quote_ids)
        .def_readwrite("maturity_date", &domain::CommoditySwingTrade::maturity_date)
        .def_readwrite("max_quantity", &domain::CommoditySwingTrade::max_quantity)
        .def_readwrite("min_quantity", &domain::CommoditySwingTrade::min_quantity)
        .def_readwrite("start_date", &domain::CommoditySwingTrade::start_date)
        .def_readwrite("strike_price", &domain::CommoditySwingTrade::strike_price)
        .def_readwrite("underlier", &domain::CommoditySwingTrade::underlier)
        .def_readwrite("unit", &domain::CommoditySwingTrade::unit)
        .def_readwrite("volatility", &domain::CommoditySwingTrade::volatility);

    py::class_<domain::CreditBondTrade, domain::Trade, std::shared_ptr<domain::CreditBondTrade>>(m, "CreditBondTrade")
        .def(py::init<>())
        .def_readwrite("coupon_rate", &domain::CreditBondTrade::coupon_rate)
        .def_readwrite("credit_spread", &domain::CreditBondTrade::credit_spread)
        .def_readwrite("frequency", &domain::CreditBondTrade::frequency)
        .def_readwrite("issuer", &domain::CreditBondTrade::issuer)
        .def_readwrite("maturity_date", &domain::CreditBondTrade::maturity_date)
        .def_readwrite("notional", &domain::CreditBondTrade::notional)
        .def_readwrite("spread_quote_id", &domain::CreditBondTrade::spread_quote_id)
        .def_readwrite("start_date", &domain::CreditBondTrade::start_date);

    py::class_<domain::CreditIndexOptionTrade, domain::Trade, std::shared_ptr<domain::CreditIndexOptionTrade>>(
        m,
        "CreditIndexOptionTrade")
        .def(py::init<>())
        .def_readwrite("expiry_date", &domain::CreditIndexOptionTrade::expiry_date)
        .def_readwrite("frequency", &domain::CreditIndexOptionTrade::frequency)
        .def_readwrite("index_factor", &domain::CreditIndexOptionTrade::index_factor)
        .def_readwrite("index_name", &domain::CreditIndexOptionTrade::index_name)
        .def_readwrite("maturity_date", &domain::CreditIndexOptionTrade::maturity_date)
        .def_readwrite("notional", &domain::CreditIndexOptionTrade::notional)
        .def_readwrite("option_type", &domain::CreditIndexOptionTrade::option_type)
        .def_readwrite("recovery_quote_id", &domain::CreditIndexOptionTrade::recovery_quote_id)
        .def_readwrite("recovery_rate", &domain::CreditIndexOptionTrade::recovery_rate)
        .def_readwrite("spread_quote_id", &domain::CreditIndexOptionTrade::spread_quote_id)
        .def_readwrite("strike_spread", &domain::CreditIndexOptionTrade::strike_spread)
        .def_readwrite("volatility", &domain::CreditIndexOptionTrade::volatility)
        .def_readwrite("volatility_quote_id", &domain::CreditIndexOptionTrade::volatility_quote_id);

    py::class_<domain::DepositTrade, domain::Trade, std::shared_ptr<domain::DepositTrade>>(m, "DepositTrade")
        .def(py::init<>())
        .def_readwrite("deposit_rate", &domain::DepositTrade::deposit_rate)
        .def_readwrite("maturity_date", &domain::DepositTrade::maturity_date)
        .def_readwrite("notional", &domain::DepositTrade::notional)
        .def_readwrite("start_date", &domain::DepositTrade::start_date);

    py::class_<domain::EquitySpotTrade, domain::Trade, std::shared_ptr<domain::EquitySpotTrade>>(m, "EquitySpotTrade")
        .def(py::init<>())
        .def_readwrite("quantity", &domain::EquitySpotTrade::quantity)
        .def_readwrite("reference_price", &domain::EquitySpotTrade::reference_price)
        .def_readwrite("underlier", &domain::EquitySpotTrade::underlier);

    py::class_<domain::EquityForwardTrade, domain::Trade, std::shared_ptr<domain::EquityForwardTrade>>(
        m,
        "EquityForwardTrade")
        .def(py::init<>())
        .def_readwrite("borrow_rate", &domain::EquityForwardTrade::borrow_rate)
        .def_readwrite("borrow_rate_quote_id", &domain::EquityForwardTrade::borrow_rate_quote_id)
        .def_readwrite("dividend_yield", &domain::EquityForwardTrade::dividend_yield)
        .def_readwrite("dividend_yield_quote_id", &domain::EquityForwardTrade::dividend_yield_quote_id)
        .def_readwrite("forward_price", &domain::EquityForwardTrade::forward_price)
        .def_readwrite("maturity_date", &domain::EquityForwardTrade::maturity_date)
        .def_readwrite("quantity", &domain::EquityForwardTrade::quantity)
        .def_readwrite("spot_quote_id", &domain::EquityForwardTrade::spot_quote_id)
        .def_readwrite("underlier", &domain::EquityForwardTrade::underlier);

    py::class_<domain::EquityFutureTrade, domain::Trade, std::shared_ptr<domain::EquityFutureTrade>>(
        m,
        "EquityFutureTrade")
        .def(py::init<>())
        .def_readwrite("borrow_rate", &domain::EquityFutureTrade::borrow_rate)
        .def_readwrite("borrow_rate_quote_id", &domain::EquityFutureTrade::borrow_rate_quote_id)
        .def_readwrite("contract_size", &domain::EquityFutureTrade::contract_size)
        .def_readwrite("dividend_yield", &domain::EquityFutureTrade::dividend_yield)
        .def_readwrite("dividend_yield_quote_id", &domain::EquityFutureTrade::dividend_yield_quote_id)
        .def_readwrite("future_quote_id", &domain::EquityFutureTrade::future_quote_id)
        .def_readwrite("maturity_date", &domain::EquityFutureTrade::maturity_date)
        .def_readwrite("quantity", &domain::EquityFutureTrade::quantity)
        .def_readwrite("reference_price", &domain::EquityFutureTrade::reference_price)
        .def_readwrite("spot_quote_id", &domain::EquityFutureTrade::spot_quote_id)
        .def_readwrite("underlier", &domain::EquityFutureTrade::underlier);

    py::class_<domain::EquityOptionTrade, domain::Trade, std::shared_ptr<domain::EquityOptionTrade>>(
        m,
        "EquityOptionTrade")
        .def(py::init<>())
        .def_readwrite("borrow_rate", &domain::EquityOptionTrade::borrow_rate)
        .def_readwrite("borrow_rate_quote_id", &domain::EquityOptionTrade::borrow_rate_quote_id)
        .def_readwrite("dividend_yield", &domain::EquityOptionTrade::dividend_yield)
        .def_readwrite("dividend_yield_quote_id", &domain::EquityOptionTrade::dividend_yield_quote_id)
        .def_readwrite("exercise_style", &domain::EquityOptionTrade::exercise_style)
        .def_readwrite("expiry_date", &domain::EquityOptionTrade::expiry_date)
        .def_readwrite("option_type", &domain::EquityOptionTrade::option_type)
        .def_readwrite("quantity", &domain::EquityOptionTrade::quantity)
        .def_readwrite("settlement_date", &domain::EquityOptionTrade::settlement_date)
        .def_readwrite("spot_quote_id", &domain::EquityOptionTrade::spot_quote_id)
        .def_readwrite("strike_price", &domain::EquityOptionTrade::strike_price)
        .def_readwrite("underlier", &domain::EquityOptionTrade::underlier)
        .def_readwrite("volatility", &domain::EquityOptionTrade::volatility)
        .def_readwrite("volatility_quote_id", &domain::EquityOptionTrade::volatility_quote_id);

    py::class_<domain::EuropeanSwaptionTrade, domain::Trade, std::shared_ptr<domain::EuropeanSwaptionTrade>>(
        m,
        "EuropeanSwaptionTrade")
        .def(py::init<>())
        .def_readwrite("fixed_rate", &domain::EuropeanSwaptionTrade::fixed_rate)
        .def_readwrite("floating_index", &domain::EuropeanSwaptionTrade::floating_index)
        .def_readwrite("maturity_date", &domain::EuropeanSwaptionTrade::maturity_date)
        .def_readwrite("notional", &domain::EuropeanSwaptionTrade::notional)
        .def_readwrite("option_expiry_date", &domain::EuropeanSwaptionTrade::option_expiry_date)
        .def_readwrite("start_date", &domain::EuropeanSwaptionTrade::start_date)
        .def_readwrite("volatility", &domain::EuropeanSwaptionTrade::volatility)
        .def_readwrite("volatility_quote_id", &domain::EuropeanSwaptionTrade::volatility_quote_id);

    py::class_<domain::FixedRateBondTrade, domain::Trade, std::shared_ptr<domain::FixedRateBondTrade>>(
        m,
        "FixedRateBondTrade")
        .def(py::init<>())
        .def_readwrite("coupon_rate", &domain::FixedRateBondTrade::coupon_rate)
        .def_readwrite("frequency", &domain::FixedRateBondTrade::frequency)
        .def_readwrite("maturity_date", &domain::FixedRateBondTrade::maturity_date)
        .def_readwrite("notional", &domain::FixedRateBondTrade::notional)
        .def_readwrite("start_date", &domain::FixedRateBondTrade::start_date);

    py::class_<domain::CallableBondTrade, domain::FixedRateBondTrade, std::shared_ptr<domain::CallableBondTrade>>(
        m,
        "CallableBondTrade")
        .def(py::init<>())
        .def_readwrite("call_dates", &domain::CallableBondTrade::call_dates)
        .def_readwrite("call_prices", &domain::CallableBondTrade::call_prices)
        .def_readwrite("mean_reversion", &domain::CallableBondTrade::mean_reversion)
        .def_readwrite("volatility", &domain::CallableBondTrade::volatility)
        .def_readwrite("volatility_quote_id", &domain::CallableBondTrade::volatility_quote_id);

    py::class_<domain::FloatingRateNoteTrade, domain::Trade, std::shared_ptr<domain::FloatingRateNoteTrade>>(
        m,
        "FloatingRateNoteTrade")
        .def(py::init<>())
        .def_readwrite("floating_index", &domain::FloatingRateNoteTrade::floating_index)
        .def_readwrite("frequency", &domain::FloatingRateNoteTrade::frequency)
        .def_readwrite("maturity_date", &domain::FloatingRateNoteTrade::maturity_date)
        .def_readwrite("notional", &domain::FloatingRateNoteTrade::notional)
        .def_readwrite("spread", &domain::FloatingRateNoteTrade::spread)
        .def_readwrite("start_date", &domain::FloatingRateNoteTrade::start_date);

    py::class_<domain::FraTrade, domain::Trade, std::shared_ptr<domain::FraTrade>>(m, "FraTrade")
        .def(py::init<>())
        .def_readwrite("floating_index", &domain::FraTrade::floating_index)
        .def_readwrite("maturity_date", &domain::FraTrade::maturity_date)
        .def_readwrite("notional", &domain::FraTrade::notional)
        .def_readwrite("start_date", &domain::FraTrade::start_date)
        .def_readwrite("strike_rate", &domain::FraTrade::strike_rate);

    py::class_<domain::FxForwardTrade, domain::Trade, std::shared_ptr<domain::FxForwardTrade>>(m, "FxForwardTrade")
        .def(py::init<>())
        .def_readwrite("base_currency", &domain::FxForwardTrade::base_currency)
        .def_readwrite("forward_points_quote_id", &domain::FxForwardTrade::forward_points_quote_id)
        .def_readwrite("forward_quote_id", &domain::FxForwardTrade::forward_quote_id)
        .def_readwrite("forward_rate", &domain::FxForwardTrade::forward_rate)
        .def_readwrite("maturity_date", &domain::FxForwardTrade::maturity_date)
        .def_readwrite("notional", &domain::FxForwardTrade::notional)
        .def_readwrite("quote_currency", &domain::FxForwardTrade::quote_currency)
        .def_readwrite("spot_quote_id", &domain::FxForwardTrade::spot_quote_id)
        .def_readwrite("start_date", &domain::FxForwardTrade::start_date);

    py::class_<domain::FxOptionTrade, domain::Trade, std::shared_ptr<domain::FxOptionTrade>>(m, "FxOptionTrade")
        .def(py::init<>())
        .def_readwrite("base_currency", &domain::FxOptionTrade::base_currency)
        .def_readwrite("expiry_date", &domain::FxOptionTrade::expiry_date)
        .def_readwrite("notional", &domain::FxOptionTrade::notional)
        .def_readwrite("option_type", &domain::FxOptionTrade::option_type)
        .def_readwrite("quote_currency", &domain::FxOptionTrade::quote_currency)
        .def_readwrite("settlement_date", &domain::FxOptionTrade::settlement_date)
        .def_readwrite("spot_quote_id", &domain::FxOptionTrade::spot_quote_id)
        .def_readwrite("strike_rate", &domain::FxOptionTrade::strike_rate)
        .def_readwrite("volatility", &domain::FxOptionTrade::volatility)
        .def_readwrite("volatility_quote_id", &domain::FxOptionTrade::volatility_quote_id);

    py::class_<domain::FxSpotTrade, domain::Trade, std::shared_ptr<domain::FxSpotTrade>>(m, "FxSpotTrade")
        .def(py::init<>())
        .def_readwrite("base_currency", &domain::FxSpotTrade::base_currency)
        .def_readwrite("notional", &domain::FxSpotTrade::notional)
        .def_readwrite("quote_currency", &domain::FxSpotTrade::quote_currency)
        .def_readwrite("reference_rate", &domain::FxSpotTrade::reference_rate)
        .def_readwrite("spot_quote_id", &domain::FxSpotTrade::spot_quote_id);

    py::class_<domain::FxSwapTrade, domain::Trade, std::shared_ptr<domain::FxSwapTrade>>(m, "FxSwapTrade")
        .def(py::init<>())
        .def_readwrite("base_currency", &domain::FxSwapTrade::base_currency)
        .def_readwrite("far_forward_points_quote_id", &domain::FxSwapTrade::far_forward_points_quote_id)
        .def_readwrite("far_forward_quote_id", &domain::FxSwapTrade::far_forward_quote_id)
        .def_readwrite("far_rate", &domain::FxSwapTrade::far_rate)
        .def_readwrite("maturity_date", &domain::FxSwapTrade::maturity_date)
        .def_readwrite("near_forward_points_quote_id", &domain::FxSwapTrade::near_forward_points_quote_id)
        .def_readwrite("near_forward_quote_id", &domain::FxSwapTrade::near_forward_quote_id)
        .def_readwrite("near_rate", &domain::FxSwapTrade::near_rate)
        .def_readwrite("notional", &domain::FxSwapTrade::notional)
        .def_readwrite("quote_currency", &domain::FxSwapTrade::quote_currency)
        .def_readwrite("spot_quote_id", &domain::FxSwapTrade::spot_quote_id)
        .def_readwrite("start_date", &domain::FxSwapTrade::start_date);

    py::class_<domain::InterestRateFutureTrade, domain::Trade, std::shared_ptr<domain::InterestRateFutureTrade>>(
        m,
        "InterestRateFutureTrade")
        .def(py::init<>())
        .def_readwrite("contract_size", &domain::InterestRateFutureTrade::contract_size)
        .def_readwrite("floating_index", &domain::InterestRateFutureTrade::floating_index)
        .def_readwrite("future_quote_id", &domain::InterestRateFutureTrade::future_quote_id)
        .def_readwrite("maturity_date", &domain::InterestRateFutureTrade::maturity_date)
        .def_readwrite("quantity", &domain::InterestRateFutureTrade::quantity)
        .def_readwrite("reference_price", &domain::InterestRateFutureTrade::reference_price)
        .def_readwrite("start_date", &domain::InterestRateFutureTrade::start_date);

    py::class_<domain::NdfTrade, domain::Trade, std::shared_ptr<domain::NdfTrade>>(m, "NdfTrade")
        .def(py::init<>())
        .def_readwrite("base_currency", &domain::NdfTrade::base_currency)
        .def_readwrite("fixing_date", &domain::NdfTrade::fixing_date)
        .def_readwrite("fixing_quote_id", &domain::NdfTrade::fixing_quote_id)
        .def_readwrite("forward_points_quote_id", &domain::NdfTrade::forward_points_quote_id)
        .def_readwrite("forward_quote_id", &domain::NdfTrade::forward_quote_id)
        .def_readwrite("forward_rate", &domain::NdfTrade::forward_rate)
        .def_readwrite("maturity_date", &domain::NdfTrade::maturity_date)
        .def_readwrite("notional", &domain::NdfTrade::notional)
        .def_readwrite("quote_currency", &domain::NdfTrade::quote_currency)
        .def_readwrite("spot_quote_id", &domain::NdfTrade::spot_quote_id);

    py::class_<domain::OisSwapTrade, domain::Trade, std::shared_ptr<domain::OisSwapTrade>>(m, "OisSwapTrade")
        .def(py::init<>())
        .def_readwrite("fixed_rate", &domain::OisSwapTrade::fixed_rate)
        .def_readwrite("maturity_date", &domain::OisSwapTrade::maturity_date)
        .def_readwrite("notional", &domain::OisSwapTrade::notional)
        .def_readwrite("overnight_index", &domain::OisSwapTrade::overnight_index)
        .def_readwrite("spread", &domain::OisSwapTrade::spread)
        .def_readwrite("start_date", &domain::OisSwapTrade::start_date);

    py::class_<domain::Portfolio>(m, "Portfolio")
        .def(py::init<>())
        .def_readwrite("portfolio_id", &domain::Portfolio::portfolio_id)
        .def_readwrite("trades", &domain::Portfolio::trades);

    py::class_<domain::VanillaSwapTrade, domain::Trade, std::shared_ptr<domain::VanillaSwapTrade>>(m,
                                                                                                   "VanillaSwapTrade")
        .def(py::init<>())
        .def_readwrite("fixed_rate", &domain::VanillaSwapTrade::fixed_rate)
        .def_readwrite("floating_index", &domain::VanillaSwapTrade::floating_index)
        .def_readwrite("maturity_date", &domain::VanillaSwapTrade::maturity_date)
        .def_readwrite("notional", &domain::VanillaSwapTrade::notional)
        .def_readwrite("start_date", &domain::VanillaSwapTrade::start_date);
}

} // namespace qrp::bindings
