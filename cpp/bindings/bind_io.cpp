#include "bindings.hpp"
#include <qrp/io/json_loader.hpp>

namespace py = pybind11;
using namespace qrp;

namespace qrp::bindings {

void bind_io(py::module_& m) {
        m.def("load_market", &io::load_market, "Load market from JSON file");

        m.def("load_portfolio", &io::load_portfolio, "Load portfolio from JSON file");
}

} // namespace qrp::bindings

