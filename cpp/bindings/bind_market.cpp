#include "bindings.hpp"
#include <pybind11/stl.h>
#include <qrp/market/factor_shock_resolver.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <qrp/market/scenario_engine.hpp>

namespace py = pybind11;
using namespace qrp;

namespace qrp::bindings {

void bind_market(py::module_& m) {
        py::class_<market::MarketSnapshot>(m, "MarketSnapshotObject")
            .def(py::init<const domain::MarketSnapshot&>())
            .def("built_state", &market::MarketSnapshot::built_state);
        py::class_<market::ScenarioDefinition>(m, "ScenarioDefinition")
            .def(py::init<>())
            .def_readwrite("name", &market::ScenarioDefinition::name)
            .def_readwrite("factor_shocks", &market::ScenarioDefinition::factor_shocks);
        m.def("resolve_quote_values", &market::FactorShockResolver::resolve_quote_values,
              py::arg("scenario"), py::arg("factors"), py::arg("bindings"), py::arg("base_market_dto"),
              "Resolve factor shocks into absolute quote values");
}

} // namespace qrp::bindings

