// Registers analytics services and result DTOs with the Python extension module.

#include "bindings.hpp"
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
#include <qrp/market/market_snapshot.hpp>
#include <qrp/market/scenario_engine.hpp>

// Exposes valuation, stress, risk, Monte Carlo, and product-support analytics.

namespace py = pybind11;
using namespace qrp;

namespace qrp::bindings {

void bind_analytics(py::module_& m) {
        py::enum_<analytics::MonteCarloMode>(m, "MonteCarloMode")
            .value("AgedHorizonRevaluation", analytics::MonteCarloMode::AgedHorizonRevaluation)
            .value("HorizonShockOnly", analytics::MonteCarloMode::HorizonShockOnly);

        py::enum_<analytics::PnlExplainComponentType>(m, "PnlExplainComponentType")
            .value("Carry", analytics::PnlExplainComponentType::Carry)
            .value("Cash", analytics::PnlExplainComponentType::Cash)
            .value("MarketMove", analytics::PnlExplainComponentType::MarketMove)
            .value("Residual", analytics::PnlExplainComponentType::Residual);

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
            .def_readwrite("valuation_date_after", &analytics::PathTrace::valuation_date_after)
            .def_readwrite("num_priced", &analytics::PathTrace::num_priced)
            .def_readwrite("num_expired", &analytics::PathTrace::num_expired)
            .def_readwrite("num_failed", &analytics::PathTrace::num_failed)
            .def_readwrite("num_unsupported", &analytics::PathTrace::num_unsupported);

        py::class_<analytics::CashflowExtractionResult>(m, "CashflowExtractionResult")
            .def(py::init<>())
            .def_readwrite("realized_cash_pnl", &analytics::CashflowExtractionResult::realized_cash_pnl)
            .def_readwrite("extraction_supported", &analytics::CashflowExtractionResult::extraction_supported)
            .def_readwrite("model_name", &analytics::CashflowExtractionResult::model_name)
            .def_readwrite("support_status", &analytics::CashflowExtractionResult::support_status)
            .def_readwrite("status_message", &analytics::CashflowExtractionResult::status_message)
            .def_readwrite("tags", &analytics::CashflowExtractionResult::tags);

        py::class_<analytics::ValuationResult>(m, "ValuationResult")
            .def(py::init<>())
            .def_readwrite("asset_class", &analytics::ValuationResult::asset_class)
            .def_readwrite("currency", &analytics::ValuationResult::currency)
            .def_readwrite("model_name", &analytics::ValuationResult::model_name)
            .def_readwrite("npv", &analytics::ValuationResult::npv)
            .def_readwrite("product_type", &analytics::ValuationResult::product_type)
            .def_readwrite("support_status", &analytics::ValuationResult::support_status)
            .def_readwrite("status_message", &analytics::ValuationResult::status_message)
            .def_readwrite("tags", &analytics::ValuationResult::tags)
            .def_readwrite("trade_id", &analytics::ValuationResult::trade_id);

        py::class_<analytics::PnlExplainComponent>(m, "PnlExplainComponent")
            .def(py::init<>())
            .def_readwrite("component_id", &analytics::PnlExplainComponent::component_id)
            .def_readwrite("component_type", &analytics::PnlExplainComponent::component_type)
            .def_readwrite("label", &analytics::PnlExplainComponent::label)
            .def_readwrite("amount", &analytics::PnlExplainComponent::amount)
            .def_readwrite("factor_id", &analytics::PnlExplainComponent::factor_id)
            .def_readwrite("model_name", &analytics::PnlExplainComponent::model_name)
            .def_readwrite("support_status", &analytics::PnlExplainComponent::support_status)
            .def_readwrite("status_message", &analytics::PnlExplainComponent::status_message)
            .def_readwrite("tags", &analytics::PnlExplainComponent::tags);

        py::class_<analytics::PnlExplainResult>(m, "PnlExplainResult")
            .def(py::init<>())
            .def_readwrite("trade_id", &analytics::PnlExplainResult::trade_id)
            .def_readwrite("prev_npv", &analytics::PnlExplainResult::prev_npv)
            .def_readwrite("curr_npv", &analytics::PnlExplainResult::curr_npv)
            .def_readwrite("total_pnl", &analytics::PnlExplainResult::total_pnl)
            .def_readwrite("carry_pnl", &analytics::PnlExplainResult::carry_pnl)
            .def_readwrite("market_move_pnl", &analytics::PnlExplainResult::market_move_pnl)
            .def_readwrite("cash_pnl", &analytics::PnlExplainResult::cash_pnl)
            .def_readwrite("residual", &analytics::PnlExplainResult::residual)
            .def_readwrite("prev_valuation_available", &analytics::PnlExplainResult::prev_valuation_available)
            .def_readwrite("curr_valuation_available", &analytics::PnlExplainResult::curr_valuation_available)
            .def_readwrite("rolled_valuation_available", &analytics::PnlExplainResult::rolled_valuation_available)
            .def_readwrite("prev_valuation", &analytics::PnlExplainResult::prev_valuation)
            .def_readwrite("curr_valuation", &analytics::PnlExplainResult::curr_valuation)
            .def_readwrite("rolled_valuation", &analytics::PnlExplainResult::rolled_valuation)
            .def_readwrite("cashflow_extraction", &analytics::PnlExplainResult::cashflow_extraction)
            .def_readwrite("components", &analytics::PnlExplainResult::components)
            .def_readwrite("diagnostics", &analytics::PnlExplainResult::diagnostics);
        py::class_<analytics::RiskResult>(m, "RiskResult")
            .def(py::init<>())
            .def_readwrite("trade_id", &analytics::RiskResult::trade_id)
            .def_readwrite("pv01", &analytics::RiskResult::pv01)
            .def_readwrite("cs01", &analytics::RiskResult::cs01)
            .def_readwrite("bucketed_risk", &analytics::RiskResult::bucketed_risk);
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
            .def_readwrite("traces", &analytics::SimulationResult::traces)
            .def_readwrite("num_trades_total", &analytics::SimulationResult::num_trades_total)
            .def_readwrite("num_trades_priced_t0", &analytics::SimulationResult::num_trades_priced_t0)
            .def_readwrite("num_trades_failed_t0", &analytics::SimulationResult::num_trades_failed_t0)
            .def_readwrite("num_trades_priced_tH", &analytics::SimulationResult::num_trades_priced_tH)
            .def_readwrite("num_trades_expired_tH", &analytics::SimulationResult::num_trades_expired_tH)
            .def_readwrite("num_trades_failed_tH", &analytics::SimulationResult::num_trades_failed_tH)
            .def_readwrite("num_trades_unsupported_tH", &analytics::SimulationResult::num_trades_unsupported_tH)
            .def_readwrite("construction_errors", &analytics::SimulationResult::construction_errors);

        py::class_<analytics::StressResult>(m, "StressResult")
            .def(py::init<>())
            .def_readwrite("scenario_name", &analytics::StressResult::scenario_name)
            .def_readwrite("total_pnl", &analytics::StressResult::total_pnl)
            .def_readwrite("trade_pnls", &analytics::StressResult::trade_pnls);
        m.def("compute_risk", &analytics::RiskService::compute_risk, 
              py::arg("portfolio"), py::arg("base_market_dto"),
              py::arg("factors"),
              py::arg("bindings"),
              "Compute risk for a portfolio");

        m.def("explain_pnl", &analytics::PnlExplainService::explain_pnl, "Explain P&L between two market states");
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

} // namespace qrp::bindings

