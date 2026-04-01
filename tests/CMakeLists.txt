add_executable(unit_tests
    unit/test_dummy.cpp
    unit/test_market.cpp
    unit/test_persistence.cpp
)
target_link_libraries(unit_tests PRIVATE qrp_core GTest::gtest_main)
add_test(NAME unit_tests COMMAND unit_tests)

add_executable(integration_tests
    integration/test_risk.cpp
    integration/test_valuation.cpp
)
target_link_libraries(integration_tests PRIVATE qrp_core GTest::gtest_main)
add_test(NAME integration_tests COMMAND integration_tests)
