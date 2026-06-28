#pragma once

// Declares Python binding registration functions grouped by platform domain.

#include <pybind11/pybind11.h>

namespace qrp::bindings {

void bind_analytics(pybind11::module_& m);
void bind_domain(pybind11::module_& m);
void bind_io(pybind11::module_& m);
void bind_market(pybind11::module_& m);

} // namespace qrp::bindings
