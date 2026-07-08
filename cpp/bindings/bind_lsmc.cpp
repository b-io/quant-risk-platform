// Registers reusable LSMC exercise-policy helpers with the Python extension module.

#include "bindings.hpp"

#include <qrp/analytics/lsmc/lsmc_engine.hpp>
#include <qrp/analytics/regression/basis_function.hpp>

#include <pybind11/stl.h>

namespace py = pybind11;
using namespace qrp;

namespace qrp::bindings {

void bind_lsmc(py::module_& m) {
    py::class_<analytics::lsmc::LsmcConfig>(m, "LsmcConfig")
        .def(py::init<>())
        .def_readwrite("discount_rate", &analytics::lsmc::LsmcConfig::discount_rate)
        .def_readwrite("num_paths", &analytics::lsmc::LsmcConfig::num_paths)
        .def_readwrite("seed", &analytics::lsmc::LsmcConfig::seed);

    py::class_<analytics::lsmc::LsmcRegressionDiagnostic>(m, "LsmcRegressionDiagnostic")
        .def(py::init<>())
        .def_readwrite("basis_function_count", &analytics::lsmc::LsmcRegressionDiagnostic::basis_function_count)
        .def_readwrite("continuation_count", &analytics::lsmc::LsmcRegressionDiagnostic::continuation_count)
        .def_readwrite("exercise_count", &analytics::lsmc::LsmcRegressionDiagnostic::exercise_count)
        .def_readwrite("r_squared", &analytics::lsmc::LsmcRegressionDiagnostic::r_squared)
        .def_readwrite("residual_sum_of_squares", &analytics::lsmc::LsmcRegressionDiagnostic::residual_sum_of_squares)
        .def_readwrite("sample_count", &analytics::lsmc::LsmcRegressionDiagnostic::sample_count)
        .def_readwrite("terminal_exercise_count", &analytics::lsmc::LsmcRegressionDiagnostic::terminal_exercise_count)
        .def_readwrite("time", &analytics::lsmc::LsmcRegressionDiagnostic::time)
        .def_readwrite("time_index", &analytics::lsmc::LsmcRegressionDiagnostic::time_index);

    py::class_<analytics::lsmc::LsmcResult>(m, "LsmcResult")
        .def(py::init<>())
        .def_readwrite("basis_function_names", &analytics::lsmc::LsmcResult::basis_function_names)
        .def_readwrite("config_tags", &analytics::lsmc::LsmcResult::config_tags)
        .def_readwrite("expected_shortfall_95", &analytics::lsmc::LsmcResult::expected_shortfall_95)
        .def_readwrite("path_values", &analytics::lsmc::LsmcResult::path_values)
        .def_readwrite("regression_diagnostics", &analytics::lsmc::LsmcResult::regression_diagnostics)
        .def_readwrite("sorted_path_values", &analytics::lsmc::LsmcResult::sorted_path_values)
        .def_readwrite("standard_error", &analytics::lsmc::LsmcResult::standard_error)
        .def_readwrite("value", &analytics::lsmc::LsmcResult::value)
        .def_readwrite("var_95", &analytics::lsmc::LsmcResult::var_95);

    py::class_<analytics::lsmc::AmericanOptionLsmcRequest>(m, "AmericanOptionLsmcRequest")
        .def(py::init<>())
        .def_readwrite("basis_degree", &analytics::lsmc::AmericanOptionLsmcRequest::basis_degree)
        .def_readwrite("borrow_rate", &analytics::lsmc::AmericanOptionLsmcRequest::borrow_rate)
        .def_readwrite("config", &analytics::lsmc::AmericanOptionLsmcRequest::config)
        .def_readwrite("dividend_yield", &analytics::lsmc::AmericanOptionLsmcRequest::dividend_yield)
        .def_readwrite("exercise_steps", &analytics::lsmc::AmericanOptionLsmcRequest::exercise_steps)
        .def_readwrite("is_put", &analytics::lsmc::AmericanOptionLsmcRequest::is_put)
        .def_readwrite("maturity", &analytics::lsmc::AmericanOptionLsmcRequest::maturity)
        .def_readwrite("risk_free_rate", &analytics::lsmc::AmericanOptionLsmcRequest::risk_free_rate)
        .def_readwrite("spot", &analytics::lsmc::AmericanOptionLsmcRequest::spot)
        .def_readwrite("strike", &analytics::lsmc::AmericanOptionLsmcRequest::strike)
        .def_readwrite("volatility", &analytics::lsmc::AmericanOptionLsmcRequest::volatility);

    py::class_<analytics::regression::PolynomialBasis>(m, "PolynomialBasis")
        .def(py::init<int>(), py::arg("degree"))
        .def("evaluate", &analytics::regression::PolynomialBasis::evaluate, py::arg("state"));

    m.def("price_american_option_lsmc",
          &analytics::lsmc::price_american_option,
          py::arg("request"),
          "Price a vanilla American option with the C++ LSMC exercise-policy engine");
}

} // namespace qrp::bindings
