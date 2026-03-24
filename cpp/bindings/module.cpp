#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/portfolio.hpp>
#include <qrp/io/json_loader.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/analytics/risk_service.hpp>
#include <qrp/analytics/pnl_explain_service.hpp>
#include <qrp/market/scenario_engine.hpp>

namespace py = pybind11;
using namespace qrp;

PYBIND11_MODULE(quant_risk_platform, m) {
    m.doc() = "Quant Risk Platform Python API";

    // Domain DTOs (simplified representation for Python if needed, or bind directly)
    py::class_<domain::CurveNode>(m, "CurveNode")
        .def(py::init<>())
        .def_readwrite("tenor", &domain::CurveNode::tenor)
        .def_readwrite("value", &domain::CurveNode::value);

    py::class_<domain::CurveDefinition>(m, "CurveDefinition")
        .def(py::init<>())
        .def_readwrite("type", &domain::CurveDefinition::type)
        .def_readwrite("currency", &domain::CurveDefinition::currency)
        .def_readwrite("nodes", &domain::CurveDefinition::nodes);

    py::class_<domain::MarketSnapshot>(m, "MarketSnapshot")
        .def(py::init<>())
        .def_readwrite("valuation_date", &domain::MarketSnapshot::valuation_date)
        .def_readwrite("markets", &domain::MarketSnapshot::markets);

    py::class_<domain::Trade>(m, "Trade")
        .def(py::init<>())
        .def_readwrite("id", &domain::Trade::id)
        .def_readwrite("asset_class", &domain::Trade::asset_class)
        .def_readwrite("type", &domain::Trade::type)
        .def_readwrite("currency", &domain::Trade::currency)
        .def_readwrite("notional", &domain::Trade::notional)
        .def_readwrite("start_date", &domain::Trade::start_date)
        .def_readwrite("maturity_date", &domain::Trade::maturity_date);

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
        .def_readwrite("bucketed_risk", &analytics::RiskResult::bucketed_risk);

    py::class_<analytics::PnlExplainResult>(m, "PnlExplainResult")
        .def_readwrite("trade_id", &analytics::PnlExplainResult::trade_id)
        .def_readwrite("prev_npv", &analytics::PnlExplainResult::prev_npv)
        .def_readwrite("curr_npv", &analytics::PnlExplainResult::curr_npv)
        .def_readwrite("total_pnl", &analytics::PnlExplainResult::total_pnl)
        .def_readwrite("market_move_pnl", &analytics::PnlExplainResult::market_move_pnl)
        .def_readwrite("residual", &analytics::PnlExplainResult::residual);

    // Core Platform Wrapper
    py::class_<market::MarketSnapshot>(m, "MarketObject")
        .def(py::init<const domain::MarketSnapshot&>());

    m.def("load_market", &io::load_market, "Load market from JSON file");
    m.def("load_portfolio", &io::load_portfolio, "Load portfolio from JSON file");

    m.def("price_portfolio", [](const domain::Portfolio& p, const market::MarketSnapshot& m) {
        return analytics::ValuationService::price_portfolio(p, m);
    }, "Price a portfolio");

    m.def("compute_risk", &analytics::RiskService::compute_risk, "Compute risk for a portfolio");

    m.def("explain_pnl", &analytics::PnlExplainService::explain_pnl, "Explain P&L between two market states");
}
