#include "bindings.hpp"
#include <pybind11/stl.h>
#include <qrp/market/factor_shock_resolver.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <qrp/market/scenario_engine.hpp>

// Exposes market state, market snapshot build reports, and scenario shock helpers.

namespace py = pybind11;
using namespace qrp;

namespace qrp::bindings {

/**
 * @brief Binds market-state construction, market build reports, and scenario shock helpers.
 */
void bind_market(py::module_& m) {
        py::class_<market::CurveBuildResult>(m, "CurveBuildResult")
            .def(py::init<>())
            .def_readwrite("built", &market::CurveBuildResult::built)
            .def_readwrite("id", &market::CurveBuildResult::id)
            .def_readwrite("purpose", &market::CurveBuildResult::purpose)
            .def_readwrite("quote_ids", &market::CurveBuildResult::quote_ids)
            .def_readwrite("status_message", &market::CurveBuildResult::status_message);

        py::class_<market::MarketSnapshot>(m, "MarketSnapshotObject")
            .def(py::init<const domain::MarketSnapshot&>())
            .def("built_state", &market::MarketSnapshot::built_state);

        py::class_<market::ScenarioDefinition>(m, "ScenarioDefinition")
            .def(py::init<>())
            .def_readwrite("name", &market::ScenarioDefinition::name)
            .def_readwrite("factor_shocks", &market::ScenarioDefinition::factor_shocks);

        m.def("build_rates_market_report", [](const domain::MarketSnapshot& dto) {
            return market::RatesMarketBuilder::build(dto).curve_results;
        }, "Build supported rates market objects and return curve build diagnostics");

        m.def("resolve_quote_values", &market::FactorShockResolver::resolve_quote_values,
              py::arg("scenario"), py::arg("factors"), py::arg("bindings"), py::arg("base_market_dto"),
              "Resolve factor shocks into absolute quote values");
}

} // namespace qrp::bindings

