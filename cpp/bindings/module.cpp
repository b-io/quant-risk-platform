#include <qrp/analytics/monte_carlo_engine.hpp>
#include <qrp/analytics/pnl_explain_service.hpp>
#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/risk_service.hpp>
#include <qrp/analytics/stress_engine.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/portfolio.hpp>
#include <qrp/io/json_loader.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <qrp/market/scenario_engine.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;
using namespace qrp;

PYBIND11_MODULE(quant_risk_platform, m) {
    m.doc() = "Quant Risk Platform Python API";

    // Enums
    py::enum_<domain::Currency>(m, "Currency")
        .value("USD", domain::Currency::USD)
        .value("EUR", domain::Currency::EUR)
        .value("GBP", domain::Currency::GBP)
        .value("CHF", domain::Currency::CHF)
        .value("JPY", domain::Currency::JPY)
        .export_values();

    py::enum_<domain::CurvePurpose>(m, "CurvePurpose")
        .value("Discount", domain::CurvePurpose::Discount)
        .value("Forward", domain::CurvePurpose::Forward)
        .value("Forward3M", domain::CurvePurpose::Forward3M)
        .value("Forward6M", domain::CurvePurpose::Forward6M)
        .value("Credit", domain::CurvePurpose::Credit)
        .value("Volatility", domain::CurvePurpose::Volatility)
        .export_values();

    py::enum_<domain::QuoteInstrumentType>(m, "QuoteInstrumentType")
        .value("Deposit", domain::QuoteInstrumentType::Deposit)
        .value("OIS", domain::QuoteInstrumentType::OIS)
        .value("IRS", domain::QuoteInstrumentType::IRS)
        .value("FRA", domain::QuoteInstrumentType::FRA)
        .value("Future", domain::QuoteInstrumentType::Future)
        .value("Bond", domain::QuoteInstrumentType::Bond)
        .value("CDS", domain::QuoteInstrumentType::CDS)
        .value("CapFloorVol", domain::QuoteInstrumentType::CapFloorVol)
        .value("SwaptionVol", domain::QuoteInstrumentType::SwaptionVol)
        .export_values();

    py::enum_<domain::QuoteType>(m, "QuoteType")
        .value("Deposit", domain::QuoteType::Deposit)
        .value("OIS", domain::QuoteType::OIS)
        .value("IRS", domain::QuoteType::IRS)
        .value("FRA", domain::QuoteType::FRA)
        .value("Future", domain::QuoteType::Future)
        .value("Swap", domain::QuoteType::Swap)
        .value("BasisSwap", domain::QuoteType::BasisSwap)
        .value("BondYield", domain::QuoteType::BondYield)
        .value("CreditSpread", domain::QuoteType::CreditSpread)
        .export_values();

    // Domain DTOs
    py::class_<domain::MarketQuote>(m, "MarketQuote")
        .def(py::init<>())
        .def_readwrite("id", &domain::MarketQuote::id)
        .def_readwrite("instrument_type", &domain::MarketQuote::instrument_type)
        .def_readwrite("currency", &domain::MarketQuote::currency)
        .def_readwrite("tenor", &domain::MarketQuote::tenor)
        .def_readwrite("value", &domain::MarketQuote::value)
        .def_readwrite("instrument_family", &domain::MarketQuote::instrument_family)
        .def_readwrite("index_family", &domain::MarketQuote::index_family)
        .def_readwrite("day_count", &domain::MarketQuote::day_count)
        .def_readwrite("calendar", &domain::MarketQuote::calendar)
        .def_readwrite("bdc", &domain::MarketQuote::bdc)
        .def_readwrite("settlement_days", &domain::MarketQuote::settlement_days);

    py::class_<domain::CurveId>(m, "CurveId")
        .def(py::init<>())
        .def_readwrite("currency", &domain::CurveId::currency)
        .def_readwrite("family", &domain::CurveId::family);

    py::class_<domain::CurveSpec>(m, "CurveSpec")
        .def(py::init<>())
        .def_readwrite("id", &domain::CurveSpec::id)
        .def_readwrite("quote_ids", &domain::CurveSpec::quote_ids)
        .def_readwrite("day_count", &domain::CurveSpec::day_count)
        .def_readwrite("calendar", &domain::CurveSpec::calendar)
        .def_readwrite("interpolation", &domain::CurveSpec::interpolation);

    py::class_<domain::MarketSnapshot>(m, "MarketSnapshot")
        .def(py::init<>())
        .def_readwrite("valuation_date", &domain::MarketSnapshot::valuation_date)
        .def_readwrite("quotes", &domain::MarketSnapshot::quotes)
        .def_readwrite("curves", &domain::MarketSnapshot::curves);

    py::class_<domain::Trade>(m, "Trade")
        .def(py::init<>())
        .def_readwrite("id", &domain::Trade::id)
        .def_readwrite("asset_class", &domain::Trade::asset_class)
        .def_readwrite("type", &domain::Trade::type)
        .def_readwrite("currency", &domain::Trade::currency)
        .def_readwrite("notional", &domain::Trade::notional)
        .def_readwrite("start_date", &domain::Trade::start_date)
        .def_readwrite("maturity_date", &domain::Trade::maturity_date)
        .def_readwrite("direction", &domain::Trade::direction)
        .def_readwrite("book", &domain::Trade::book)
        .def_readwrite("strategy", &domain::Trade::strategy)
        .def_readwrite("details", &domain::Trade::details);

    py::class_<domain::Portfolio>(m, "Portfolio")
        .def(py::init<>())
        .def_readwrite("portfolio_id", &domain::Portfolio::portfolio_id)
        .def_readwrite("trades", &domain::Portfolio::trades);

    // Results
    py::class_<analytics::ValuationResult>(m, "ValuationResult")
        .def_readwrite("trade_id", &analytics::ValuationResult::trade_id)
        .def_readwrite("npv", &analytics::ValuationResult::npv)
        .def_readwrite("currency", &analytics::ValuationResult::currency)
        .def_readwrite("tags", &analytics::ValuationResult::tags);

    py::class_<analytics::RiskResult>(m, "RiskResult")
        .def_readwrite("trade_id", &analytics::RiskResult::trade_id)
        .def_readwrite("pv01", &analytics::RiskResult::pv01)
        .def_readwrite("cs01", &analytics::RiskResult::cs01)
        .def_readwrite("bucketed_risk", &analytics::RiskResult::bucketed_risk);

    py::class_<analytics::StressResult>(m, "StressResult")
        .def_readwrite("scenario_name", &analytics::StressResult::scenario_name)
        .def_readwrite("total_pnl", &analytics::StressResult::total_pnl)
        .def_readwrite("trade_pnls", &analytics::StressResult::trade_pnls);

    py::class_<analytics::PnlExplainResult>(m, "PnlExplainResult")
        .def_readwrite("trade_id", &analytics::PnlExplainResult::trade_id)
        .def_readwrite("prev_npv", &analytics::PnlExplainResult::prev_npv)
        .def_readwrite("curr_npv", &analytics::PnlExplainResult::curr_npv)
        .def_readwrite("total_pnl", &analytics::PnlExplainResult::total_pnl)
        .def_readwrite("carry_pnl", &analytics::PnlExplainResult::carry_pnl)
        .def_readwrite("cash_pnl", &analytics::PnlExplainResult::cash_pnl)
        .def_readwrite("market_move_pnl", &analytics::PnlExplainResult::market_move_pnl)
        .def_readwrite("residual", &analytics::PnlExplainResult::residual);

    py::class_<analytics::SimulationResult>(m, "SimulationResult")
        .def_readwrite("portfolio_values", &analytics::SimulationResult::portfolio_values)
        .def_readwrite("var_95", &analytics::SimulationResult::var_95)
        .def_readwrite("expected_shortfall_95", &analytics::SimulationResult::expected_shortfall_95);

    py::class_<market::ScenarioDefinition>(m, "ScenarioDefinition")
        .def(py::init<>())
        .def_readwrite("name", &market::ScenarioDefinition::name)
        .def_readwrite("parallel_shocks", &market::ScenarioDefinition::parallel_shocks)
        .def_readwrite("node_shocks", &market::ScenarioDefinition::node_shocks)
        .def_readwrite("credit_shocks", &market::ScenarioDefinition::credit_shocks);

    // Core Platform Wrapper
    py::class_<market::MarketSnapshot>(m, "MarketSnapshotObject")
        .def(py::init<const domain::MarketSnapshot&>())
        .def("built_state", &market::MarketSnapshot::built_state);

    m.def("load_market", &io::load_market, "Load market from JSON file");
    m.def("load_portfolio", &io::load_portfolio, "Load portfolio from JSON file");

    m.def("price_portfolio", [](const domain::Portfolio& p, const domain::MarketSnapshot& m_dto) {
        market::MarketSnapshot m(m_dto);
        analytics::PricingContext ctx(m.built_state());
        return analytics::ValuationService::price_portfolio(p, ctx);
    }, "Price a portfolio");

    m.def("compute_risk", &analytics::RiskService::compute_risk, "Compute risk for a portfolio");

    m.def("explain_pnl", &analytics::PnlExplainService::explain_pnl, "Explain P&L between two market states");

    m.def("run_historical_stress", &analytics::StressEngine::run_historical_stress, "Run historical stress scenarios");

    m.def("run_simulation", &analytics::MonteCarloEngine::run_simulation, "Run Monte Carlo simulation");
}
