add_executable(unit_tests
    unit/analytics/test_lsmc_american_put.cpp
    unit/analytics/test_pnl_explain.cpp
    unit/analytics/test_regression.cpp
    unit/analytics/test_simulation.cpp
    unit/analytics/test_valuation_and_stress.cpp
    unit/analytics/test_var_contribution.cpp
    unit/optimization/test_optimization_engine.cpp
    unit/test_app_workflows.cpp
    unit/test_commodity_products.cpp
    unit/test_conventions.cpp
    unit/test_covariance.cpp
    unit/test_credit_products.cpp
    unit/test_equity_products.cpp
    unit/test_fx_products.cpp
    unit/test_instrument_factory_common.cpp
    unit/test_market.cpp
    unit/test_monte_carlo.cpp
    unit/test_monte_carlo_aging.cpp
    unit/test_persistence.cpp
    unit/test_rates_products.cpp
    unit/test_scenario_engine.cpp
    unit/test_trade_model.cpp
)
target_link_libraries(unit_tests PRIVATE qrp_core GTest::gtest_main)
target_compile_definitions(unit_tests PRIVATE QRP_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/data")
if(DEFINED Python3_EXECUTABLE)
    target_compile_definitions(unit_tests PRIVATE QRP_TEST_PYTHON_EXECUTABLE="${Python3_EXECUTABLE}")
endif()
target_include_directories(unit_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/support")
qrp_apply_coverage(unit_tests)
qrp_apply_project_warnings(unit_tests)
qrp_apply_msvc_debug_symbols(unit_tests)
qrp_copy_core_runtime(unit_tests)
add_test(NAME unit_tests COMMAND unit_tests)

add_executable(integration_tests
    integration/test_portfolio_golden_coverage.cpp
    integration/test_risk.cpp
    integration/test_valuation.cpp
)
target_link_libraries(integration_tests PRIVATE qrp_core GTest::gtest_main)
target_compile_definitions(integration_tests PRIVATE QRP_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/data")
target_include_directories(integration_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/support")
qrp_apply_coverage(integration_tests)
qrp_apply_project_warnings(integration_tests)
qrp_apply_msvc_debug_symbols(integration_tests)
qrp_copy_core_runtime(integration_tests)
add_test(NAME integration_tests COMMAND integration_tests)
set_tests_properties(integration_tests PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

if(MSVC)
    # Ensure unique intermediate directories to avoid MSB8028 warning
    # This warning often occurs when targets in the same directory share common names or PDB names.
    set_target_properties(unit_tests PROPERTIES COMPILE_PDB_NAME "unit_tests_comp")
    set_target_properties(integration_tests PROPERTIES COMPILE_PDB_NAME "integration_tests_comp")
endif()
