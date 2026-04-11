#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <qrp/analytics/covariance_estimator.hpp>
#include <qrp/analytics/monte_carlo_engine.hpp>
#include <qrp/analytics/pnl_explain_service.hpp>
#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/risk_service.hpp>
#include <qrp/analytics/stress_engine.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/domain/factors.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/portfolio.hpp>
#include <qrp/io/json_loader.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <qrp/market/scenario_engine.hpp>

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
        .value("UNKNOWN", domain::Currency::UNKNOWN)
        .export_values();

    py::enum_<domain::CurvePurpose>(m, "CurvePurpose")
        .value("Discount", domain::CurvePurpose::Discount)
        .value("Forward", domain::CurvePurpose::Forward)
        .value("Forward3M", domain::CurvePurpose::Forward3M)
        .value("Forward6M", domain::CurvePurpose::Forward6M)
        .value("Credit", domain::CurvePurpose::Credit)
        .value("Volatility", domain::CurvePurpose::Volatility)
        .value("UNKNOWN", domain::CurvePurpose::UNKNOWN)
        .export_values();

    py::enum_<domain::FactorType>(m, "FactorType")
        .value("FXSpot", domain::FactorType::FXSpot)
        .value("RateZero", domain::FactorType::RateZero)
        .value("RateForward", domain::FactorType::RateForward)
        .value("CreditSpread", domain::FactorType::CreditSpread)
        .value("HazardRate", domain::FactorType::HazardRate)
        .value("Volatility", domain::FactorType::Volatility)
        .value("EquitySpot", domain::FactorType::EquitySpot)
        .value("CommodityForward", domain::FactorType::CommodityForward)
        .value("BasisSpread", domain::FactorType::BasisSpread)
        .value("Custom", domain::FactorType::Custom)
        .export_values();

    py::enum_<analytics::MonteCarloMode>(m, "MonteCarloMode")
        .value("HorizonShockOnly", analytics::MonteCarloMode::HorizonShockOnly)
        .value("AgedHorizonRevaluation", analytics::MonteCarloMode::AgedHorizonRevaluation)
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
        .value("UNKNOWN", domain::QuoteInstrumentType::UNKNOWN)
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
        .value("UNKNOWN", domain::QuoteType::UNKNOWN)
        .export_values();

    py::enum_<domain::ShockMeasure>(m, "ShockMeasure")
        .value("Absolute", domain::ShockMeasure::Absolute)
        .value("Relative", domain::ShockMeasure::Relative)
        .value("LogReturn", domain::ShockMeasure::LogReturn)
        .value("BasisPoints", domain::ShockMeasure::BasisPoints)
        .value("VolPoints", domain::ShockMeasure::VolPoints)
        .export_values();

    // Classes
    py::class_<analytics::CovarianceEstimationConfig>(m, "CovarianceEstimationConfig")
        .def(py::init<>())
        .def_readwrite("start_date", &analytics::CovarianceEstimationConfig::start_date)
        .def_readwrite("end_date", &analytics::CovarianceEstimationConfig::end_date)
        .def_readwrite("use_ewma", &analytics::CovarianceEstimationConfig::use_ewma)
        .def_readwrite("ewma_lambda", &analytics::CovarianceEstimationConfig::ewma_lambda)
        .def_readwrite("demean", &analytics::CovarianceEstimationConfig::demean)
        .def_readwrite("observation_interval_years", &analytics::CovarianceEstimationConfig::observation_interval_years)
        .def_readwrite("target_horizon_years", &analytics::CovarianceEstimationConfig::target_horizon_years)
        .def_readwrite("return_horizon_scaled_covariance", &analytics::CovarianceEstimationConfig::return_horizon_scaled_covariance)
        .def_readwrite("eigenvalue_floor", &analytics::CovarianceEstimationConfig::eigenvalue_floor);

    py::class_<domain::CurveId>(m, "CurveId")
        .def(py::init<>())
        .def_readwrite("currency", &domain::CurveId::currency)
        .def_readwrite("family", &domain::CurveId::family);

    py::class_<domain::CurveSpec>(m, "CurveSpec")
        .def(py::init<>())
        .def_readwrite("id", &domain::CurveSpec::id)
        .def_readwrite("purpose", &domain::CurveSpec::purpose)
        .def_readwrite("quote_ids", &domain::CurveSpec::quote_ids)
        .def_readwrite("day_count", &domain::CurveSpec::day_count)
        .def_readwrite("calendar", &domain::CurveSpec::calendar)
        .def_readwrite("interpolation", &domain::CurveSpec::interpolation);

    py::class_<domain::FactorBinding>(m, "FactorBinding")
        .def(py::init<>())
        .def_readwrite("factor_id", &domain::FactorBinding::factor_id)
        .def_readwrite("quote_id", &domain::FactorBinding::quote_id)
        .def_readwrite("shock_measure", &domain::FactorBinding::shock_measure)
        .def_readwrite("weight", &domain::FactorBinding::weight)
        .def_readwrite("transform", &domain::FactorBinding::transform)
        .def_readwrite("selector_json", &domain::FactorBinding::selector_json);

    py::class_<domain::FactorDefinition>(m, "FactorDefinition")
        .def(py::init<>())
        .def_readwrite("factor_id", &domain::FactorDefinition::factor_id)
        .def_readwrite("factor_type", &domain::FactorDefinition::factor_type)
        .def_readwrite("shock_measure", &domain::FactorDefinition::shock_measure)
        .def_readwrite("currency", &domain::FactorDefinition::currency)
        .def_readwrite("curve_id", &domain::FactorDefinition::curve_id)
        .def_readwrite("tenor", &domain::FactorDefinition::tenor)
        .def_readwrite("quote_ids", &domain::FactorDefinition::quote_ids)
        .def_readwrite("description", &domain::FactorDefinition::description);

    py::class_<domain::FactorObservation>(m, "FactorObservation")
        .def(py::init<>())
        .def_readwrite("factor_id", &domain::FactorObservation::factor_id)
        .def_readwrite("market_date", &domain::FactorObservation::market_date)
        .def_readwrite("level", &domain::FactorObservation::level)
        .def_readwrite("move", &domain::FactorObservation::move)
        .def_readwrite("move_unit", &domain::FactorObservation::move_unit);

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

    py::class_<domain::MarketSnapshot>(m, "MarketSnapshot")
        .def(py::init<>())
        .def_readwrite("valuation_date", &domain::MarketSnapshot::valuation_date)
        .def_readwrite("quotes", &domain::MarketSnapshot::quotes)
        .def_readwrite("curves", &domain::MarketSnapshot::curves);

    py::class_<market::MarketSnapshot>(m, "MarketSnapshotObject")
        .def(py::init<const domain::MarketSnapshot&>())
        .def("built_state", &market::MarketSnapshot::built_state);

    py::class_<QuantLib::Matrix>(m, "Matrix")
        .def(py::init<>())
        .def(py::init<size_t, size_t, double>(), py::arg("rows"), py::arg("cols"), py::arg("value") = 0.0)
        .def("rows", &QuantLib::Matrix::rows)
        .def("columns", &QuantLib::Matrix::columns)
        .def("__getitem__", [](const QuantLib::Matrix& m, std::pair<size_t, size_t> i) {
            if (i.first >= m.rows() || i.second >= m.columns()) throw py::index_error();
            return m[i.first][i.second];
        })
        .def("__setitem__", [](QuantLib::Matrix& m, std::pair<size_t, size_t> i, double v) {
            if (i.first >= m.rows() || i.second >= m.columns()) throw py::index_error();
            m[i.first][i.second] = v;
        });

    py::class_<analytics::MonteCarloConfig>(m, "MonteCarloConfig")
        .def(py::init<>())
        .def_readwrite("num_paths", &analytics::MonteCarloConfig::num_paths)
        .def_readwrite("seed", &analytics::MonteCarloConfig::seed)
        .def_readwrite("horizon_days", &analytics::MonteCarloConfig::horizon_days)
        .def_readwrite("mode", &analytics::MonteCarloConfig::mode)
        .def_readwrite("covariance_is_already_horizon_scaled", &analytics::MonteCarloConfig::covariance_is_already_horizon_scaled)
        .def_readwrite("require_bindings", &analytics::MonteCarloConfig::require_bindings);

    py::class_<analytics::PathTrace>(m, "PathTrace")
        .def(py::init<>())
        .def_readwrite("path_index", &analytics::PathTrace::path_index)
        .def_readwrite("factor_shocks", &analytics::PathTrace::factor_shocks)
        .def_readwrite("quote_before_after", &analytics::PathTrace::quote_before_after)
        .def_readwrite("portfolio_value_before", &analytics::PathTrace::portfolio_value_before)
        .def_readwrite("portfolio_value_after", &analytics::PathTrace::portfolio_value_after)
        .def_readwrite("path_pnl", &analytics::PathTrace::path_pnl)
        .def_readwrite("portfolio_value_frozen_aged", &analytics::PathTrace::portfolio_value_frozen_aged)
        .def_readwrite("aging_pnl", &analytics::PathTrace::aging_pnl)
        .def_readwrite("market_pnl", &analytics::PathTrace::market_pnl)
        .def_readwrite("total_pnl", &analytics::PathTrace::total_pnl)
        .def_readwrite("valuation_date_before", &analytics::PathTrace::valuation_date_before)
        .def_readwrite("valuation_date_after", &analytics::PathTrace::valuation_date_after);

    py::class_<analytics::PnlExplainResult>(m, "PnlExplainResult")
        .def(py::init<>())
        .def_readwrite("trade_id", &analytics::PnlExplainResult::trade_id)
        .def_readwrite("prev_npv", &analytics::PnlExplainResult::prev_npv)
        .def_readwrite("curr_npv", &analytics::PnlExplainResult::curr_npv)
        .def_readwrite("total_pnl", &analytics::PnlExplainResult::total_pnl)
        .def_readwrite("carry_pnl", &analytics::PnlExplainResult::carry_pnl)
        .def_readwrite("cash_pnl", &analytics::PnlExplainResult::cash_pnl)
        .def_readwrite("market_move_pnl", &analytics::PnlExplainResult::market_move_pnl)
        .def_readwrite("residual", &analytics::PnlExplainResult::residual);

    py::class_<domain::Portfolio>(m, "Portfolio")
        .def(py::init<>())
        .def_readwrite("portfolio_id", &domain::Portfolio::portfolio_id)
        .def_readwrite("trades", &domain::Portfolio::trades);

    py::class_<analytics::RiskResult>(m, "RiskResult")
        .def(py::init<>())
        .def_readwrite("trade_id", &analytics::RiskResult::trade_id)
        .def_readwrite("pv01", &analytics::RiskResult::pv01)
        .def_readwrite("cs01", &analytics::RiskResult::cs01)
        .def_readwrite("bucketed_risk", &analytics::RiskResult::bucketed_risk);

    py::class_<market::ScenarioDefinition>(m, "ScenarioDefinition")
        .def(py::init<>())
        .def_readwrite("name", &market::ScenarioDefinition::name)
        .def_readwrite("factor_shocks", &market::ScenarioDefinition::factor_shocks);

    py::class_<analytics::SimulationResult>(m, "SimulationResult")
        .def(py::init<>())
        .def_readwrite("portfolio_values", &analytics::SimulationResult::portfolio_values)
        .def_readwrite("portfolio_pnls", &analytics::SimulationResult::portfolio_pnls)
        .def_readwrite("base_portfolio_value", &analytics::SimulationResult::base_portfolio_value)
        .def_readwrite("var_95", &analytics::SimulationResult::var_95)
        .def_readwrite("expected_shortfall_95", &analytics::SimulationResult::expected_shortfall_95)
        .def_readwrite("seed", &analytics::SimulationResult::seed)
        .def_readwrite("horizon_days", &analytics::SimulationResult::horizon_days)
        .def_readwrite("mode", &analytics::SimulationResult::mode)
        .def_readwrite("factor_ids", &analytics::SimulationResult::factor_ids)
        .def_readwrite("traces", &analytics::SimulationResult::traces);

    py::class_<analytics::StressResult>(m, "StressResult")
        .def(py::init<>())
        .def_readwrite("scenario_name", &analytics::StressResult::scenario_name)
        .def_readwrite("total_pnl", &analytics::StressResult::total_pnl)
        .def_readwrite("trade_pnls", &analytics::StressResult::trade_pnls);

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

    py::class_<analytics::ValuationResult>(m, "ValuationResult")
        .def(py::init<>())
        .def_readwrite("trade_id", &analytics::ValuationResult::trade_id)
        .def_readwrite("npv", &analytics::ValuationResult::npv)
        .def_readwrite("currency", &analytics::ValuationResult::currency)
        .def_readwrite("tags", &analytics::ValuationResult::tags);

    m.def("compute_risk", &analytics::RiskService::compute_risk, 
          py::arg("portfolio"), py::arg("base_market_dto"),
          py::arg("factors") = std::vector<domain::FactorDefinition>(),
          py::arg("bindings") = std::vector<domain::FactorBinding>(),
          "Compute risk for a portfolio");

    m.def("explain_pnl", &analytics::PnlExplainService::explain_pnl, "Explain P&L between two market states");

    m.def("load_market", &io::load_market, "Load market from JSON file");

    m.def("load_portfolio", &io::load_portfolio, "Load portfolio from JSON file");

    m.def("price_portfolio", [](const domain::Portfolio& p, const domain::MarketSnapshot& m_dto) {
        market::MarketSnapshot m(m_dto);
        analytics::PricingContext ctx(m.built_state());
        return analytics::ValuationService::price_portfolio(p, ctx);
    }, "Price a portfolio");

    m.def("run_historical_stress", &analytics::StressEngine::run_historical_stress,
          py::arg("portfolio"), py::arg("base_market_dto"), py::arg("historical_scenarios"),
          py::arg("factors") = std::vector<domain::FactorDefinition>(),
          py::arg("bindings") = std::vector<domain::FactorBinding>(),
          "Run historical stress scenarios");

    m.def("run_simulation", &analytics::MonteCarloEngine::run_simulation,
          py::arg("portfolio"),
          py::arg("base_market_dto"),
          py::arg("factors"),
          py::arg("bindings"),
          py::arg("covariance"),
          py::arg("config"),
          "Run Monte Carlo simulation");
}
