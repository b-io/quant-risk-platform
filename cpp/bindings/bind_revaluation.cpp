// Registers reusable revaluation-session APIs with the Python extension module.

#include "bindings.hpp"

#include <qrp/analytics/revaluation_session.hpp>

#include <pybind11/stl.h>

#include <memory>
#include <utility>
#include <vector>

// Exposes the C++-owned reactive revaluation session without leaking raw QuantLib handles.

namespace py = pybind11;
using namespace qrp;

namespace qrp::bindings {

/**
 * @brief Registers stateful revaluation-session APIs with the Python module.
 */
void bind_revaluation(py::module_& m) {
    py::class_<analytics::QuoteMove>(m, "QuoteMove")
        .def(py::init<>())
        .def_readwrite("after", &analytics::QuoteMove::after)
        .def_readwrite("before", &analytics::QuoteMove::before)
        .def_readwrite("quote_id", &analytics::QuoteMove::quote_id)
        .def_readwrite("restored", &analytics::QuoteMove::restored);

    py::class_<analytics::RevaluationReport>(m, "RevaluationReport")
        .def(py::init<>())
        .def_readwrite("base_total_npv", &analytics::RevaluationReport::base_total_npv)
        .def_readwrite("quote_moves", &analytics::RevaluationReport::quote_moves)
        .def_readwrite("restored_total_npv", &analytics::RevaluationReport::restored_total_npv)
        .def_readwrite("scenario_name", &analytics::RevaluationReport::scenario_name)
        .def_readwrite("scenario_pnl", &analytics::RevaluationReport::scenario_pnl)
        .def_readwrite("shocked_total_npv", &analytics::RevaluationReport::shocked_total_npv)
        .def_readwrite("trade_pnls", &analytics::RevaluationReport::trade_pnls);

    py::class_<analytics::RevaluationDependency>(m, "RevaluationDependency")
        .def(py::init<>())
        .def_readwrite("asset_class", &analytics::RevaluationDependency::asset_class)
        .def_readwrite("curve_id", &analytics::RevaluationDependency::curve_id)
        .def_readwrite("dependency_type", &analytics::RevaluationDependency::dependency_type)
        .def_readwrite("factor_ids", &analytics::RevaluationDependency::factor_ids)
        .def_readwrite("product_type", &analytics::RevaluationDependency::product_type)
        .def_readwrite("quote_id", &analytics::RevaluationDependency::quote_id)
        .def_readwrite("trade_id", &analytics::RevaluationDependency::trade_id);

    py::class_<analytics::RevaluationDependencyGraph>(m, "RevaluationDependencyGraph")
        .def_property_readonly("dependencies",
                               [](const analytics::RevaluationDependencyGraph& graph) { return graph.dependencies; })
        .def_property_readonly(
            "dependency_count",
            [](const analytics::RevaluationDependencyGraph& graph) { return graph.dependency_count; })
        .def_property_readonly("quote_count",
                               [](const analytics::RevaluationDependencyGraph& graph) { return graph.quote_count; })
        .def_property_readonly("quote_ids",
                               [](const analytics::RevaluationDependencyGraph& graph) { return graph.quote_ids; })
        .def_property_readonly("trade_count",
                               [](const analytics::RevaluationDependencyGraph& graph) { return graph.trade_count; })
        .def_property_readonly("trade_ids",
                               [](const analytics::RevaluationDependencyGraph& graph) { return graph.trade_ids; });

    py::class_<analytics::RevaluationImpactPreview>(m, "RevaluationImpactPreview")
        .def(py::init<>())
        .def_readwrite("dependencies", &analytics::RevaluationImpactPreview::dependencies)
        .def_readwrite("potentially_affected_trade_count",
                       &analytics::RevaluationImpactPreview::potentially_affected_trade_count)
        .def_readwrite("potentially_affected_trade_ids",
                       &analytics::RevaluationImpactPreview::potentially_affected_trade_ids)
        .def_readwrite("updated_quote_ids", &analytics::RevaluationImpactPreview::updated_quote_ids);

    py::class_<analytics::TradeRevaluationDiff>(m, "TradeRevaluationDiff")
        .def(py::init<>())
        .def_readwrite("asset_class", &analytics::TradeRevaluationDiff::asset_class)
        .def_readwrite("base_npv", &analytics::TradeRevaluationDiff::base_npv)
        .def_readwrite("dependency_quote_ids", &analytics::TradeRevaluationDiff::dependency_quote_ids)
        .def_readwrite("moved_above_tolerance", &analytics::TradeRevaluationDiff::moved_above_tolerance)
        .def_readwrite("pnl", &analytics::TradeRevaluationDiff::pnl)
        .def_readwrite("product_type", &analytics::TradeRevaluationDiff::product_type)
        .def_readwrite("shocked_npv", &analytics::TradeRevaluationDiff::shocked_npv)
        .def_readwrite("trade_id", &analytics::TradeRevaluationDiff::trade_id);

    py::class_<analytics::RevaluationImpactReport>(m, "RevaluationImpactReport")
        .def(py::init<>())
        .def_readwrite("candidate_base_total_npv", &analytics::RevaluationImpactReport::candidate_base_total_npv)
        .def_readwrite("candidate_pnl", &analytics::RevaluationImpactReport::candidate_pnl)
        .def_readwrite("candidate_restored_total_npv",
                       &analytics::RevaluationImpactReport::candidate_restored_total_npv)
        .def_readwrite("candidate_shocked_total_npv", &analytics::RevaluationImpactReport::candidate_shocked_total_npv)
        .def_readwrite("dependencies", &analytics::RevaluationImpactReport::dependencies)
        .def_readwrite("pnl_tolerance", &analytics::RevaluationImpactReport::pnl_tolerance)
        .def_readwrite("potentially_affected_trade_count",
                       &analytics::RevaluationImpactReport::potentially_affected_trade_count)
        .def_readwrite("potentially_affected_trade_ids",
                       &analytics::RevaluationImpactReport::potentially_affected_trade_ids)
        .def_readwrite("quote_moves", &analytics::RevaluationImpactReport::quote_moves)
        .def_readwrite("scenario_name", &analytics::RevaluationImpactReport::scenario_name)
        .def_readwrite("trade_diffs", &analytics::RevaluationImpactReport::trade_diffs)
        .def_readwrite("updated_quote_ids", &analytics::RevaluationImpactReport::updated_quote_ids);

    py::class_<analytics::RevaluationSession>(m, "RevaluationSession")
        .def(py::init<domain::Portfolio,
                      domain::MarketSnapshot,
                      std::vector<domain::FactorDefinition>,
                      std::vector<domain::FactorBinding>>(),
             py::arg("portfolio"),
             py::arg("base_market"),
             py::arg("factors") = std::vector<domain::FactorDefinition>(),
             py::arg("bindings") = std::vector<domain::FactorBinding>())
        .def("apply_quote_updates",
             &analytics::RevaluationSession::apply_quote_updates,
             py::arg("quote_updates"),
             "Apply absolute quote updates to the current session state")
        .def("apply_scenario",
             &analytics::RevaluationSession::apply_scenario,
             py::arg("scenario"),
             "Apply a factor scenario to the current session state")
        .def("price", &analytics::RevaluationSession::price, "Price the portfolio from the current session state")
        .def("reset", &analytics::RevaluationSession::reset, "Reset quote handles to the base market snapshot")
        .def("revalue_scenario",
             &analytics::RevaluationSession::revalue_scenario,
             py::arg("scenario"),
             "Revalue one scenario from the base state and restore the session")
        .def("preview_quote_update_impact",
             &analytics::RevaluationSession::preview_quote_update_impact,
             py::arg("quote_updates"),
             "Preview the structurally affected trades for quote updates without repricing")
        .def("preview_scenario_impact",
             &analytics::RevaluationSession::preview_scenario_impact,
             py::arg("scenario"),
             "Resolve a scenario and preview structurally affected trades without repricing")
        .def("revalue_quote_update_impact",
             &analytics::RevaluationSession::revalue_quote_update_impact,
             py::arg("quote_updates"),
             py::arg("pnl_tolerance") = 1.0e-8,
             "Reprice only structurally affected candidate trades for quote updates")
        .def("revalue_scenario_impact",
             &analytics::RevaluationSession::revalue_scenario_impact,
             py::arg("scenario"),
             py::arg("pnl_tolerance") = 1.0e-8,
             "Resolve a scenario and reprice only structurally affected candidate trades")
        .def("dependency_graph",
             &analytics::RevaluationSession::dependency_graph,
             "Return a read-only quote-to-trade dependency graph snapshot")
        .def("dependencies_for_quote",
             &analytics::RevaluationSession::dependencies_for_quote,
             py::arg("quote_id"),
             "Return read-only dependency edges for one quote id")
        .def("dependencies_for_trade",
             &analytics::RevaluationSession::dependencies_for_trade,
             py::arg("trade_id"),
             "Return read-only dependency edges for one trade id")
        .def("total_npv", &analytics::RevaluationSession::total_npv, "Return the current total portfolio NPV")
        .def("trade_values",
             &analytics::RevaluationSession::trade_values,
             "Return current trade NPVs keyed by trade id");

    m.def(
        "create_revaluation_session",
        [](domain::Portfolio portfolio,
           domain::MarketSnapshot base_market,
           std::vector<domain::FactorDefinition> factors,
           std::vector<domain::FactorBinding> bindings) {
            return std::make_unique<analytics::RevaluationSession>(std::move(portfolio),
                                                                   std::move(base_market),
                                                                   std::move(factors),
                                                                   std::move(bindings));
        },
        py::arg("portfolio"),
        py::arg("base_market"),
        py::arg("factors") = std::vector<domain::FactorDefinition>(),
        py::arg("bindings") = std::vector<domain::FactorBinding>(),
        "Create a C++-owned revaluation session with reusable market and instrument caches");
}

} // namespace qrp::bindings
