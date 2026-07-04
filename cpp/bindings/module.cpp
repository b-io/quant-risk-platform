// Defines the Python extension module and installs all domain binding groups.

#include "bindings.hpp"

#include <pybind11/pybind11.h>

// Defines the Python extension module and wires each domain-specific binding group.

PYBIND11_MODULE(quant_risk_platform, m) {
    m.doc() = "Quant Risk Platform Python API";

    // Domain types register first because later groups expose them; remaining groups are alphabetical.
    qrp::bindings::bind_domain(m);
    qrp::bindings::bind_analytics(m);
    qrp::bindings::bind_io(m);
    qrp::bindings::bind_market(m);
}
