// Registers JSON loading helpers with the Python extension module.

#include "bindings.hpp"
#include <qrp/io/json_loader.hpp>

// Exposes JSON loading helpers used by Python workflows and notebooks.

namespace py = pybind11;
using namespace qrp;

namespace qrp::bindings {

void bind_io(py::module_& m) {
        m.def("load_market", &io::load_market, "Load market from JSON file");

        m.def("load_portfolio", &io::load_portfolio, "Load portfolio from JSON file");
}

} // namespace qrp::bindings

