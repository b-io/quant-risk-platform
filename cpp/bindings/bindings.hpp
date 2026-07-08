#pragma once

// Declares Python binding registration functions grouped by platform domain.

#include <pybind11/pybind11.h>

namespace qrp::bindings {

/**
 * @brief Registers analytics services, DTOs, and result types with the Python module.
 */
void bind_analytics(pybind11::module_& m);

/**
 * @brief Registers domain trade, market, product, and factor DTOs with the Python module.
 */
void bind_domain(pybind11::module_& m);

/**
 * @brief Registers JSON loading helpers with the Python module.
 */
void bind_io(pybind11::module_& m);

/**
 * @brief Registers market builders, snapshots, and market-state helpers with the Python module.
 */
void bind_market(pybind11::module_& m);

/**
 * @brief Registers reusable revaluation-session APIs with the Python module.
 */
void bind_revaluation(pybind11::module_& m);

} // namespace qrp::bindings
