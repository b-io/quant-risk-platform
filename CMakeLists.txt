cmake_minimum_required(VERSION 3.20)
project(quant-risk-platform VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Define directories
set(QRP_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cpp/include)
set(QRP_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cpp/src)

# Dependencies
include(cmake/Dependencies.cmake)

# Library
add_library(qrp_core
    cpp/src/analytics/pnl_explain_service.cpp
    cpp/src/analytics/risk_service.cpp
    cpp/src/analytics/stress_engine.cpp
    cpp/src/analytics/monte_carlo_engine.cpp
    cpp/src/analytics/valuation_service.cpp
    cpp/src/instruments/instrument_factory.cpp
    cpp/src/io/json_loader.cpp
    cpp/src/market/market_snapshot.cpp
    cpp/src/market/scenario_engine.cpp
    cpp/src/conventions/market_convention_registry.cpp
)
target_compile_options(qrp_core PRIVATE /FS)
target_include_directories(qrp_core PUBLIC ${QRP_INCLUDE_DIR})
target_link_libraries(qrp_core PRIVATE QuantLib::QuantLib nlohmann_json::nlohmann_json fmt::fmt)

# CLI
add_executable(qrp_cli cpp/cli/main.cpp)
# Link fmt to expose headers/include dirs to CLI compilation and link
target_link_libraries(qrp_cli PRIVATE qrp_core fmt::fmt)

# Python bindings
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 REQUIRED)
pybind11_add_module(quant_risk_platform cpp/bindings/module.cpp)
target_link_libraries(quant_risk_platform PRIVATE qrp_core)

# Tests
enable_testing()
add_subdirectory(tests)

# Benchmarks (guarded)
# TODO: Restore full benchmark targets once benchmark sources are committed.
option(QRP_BUILD_BENCHMARKS "Build benchmark targets" OFF)
if(QRP_BUILD_BENCHMARKS)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/benchmark_portfolio.cpp")
        add_executable(benchmark_portfolio benchmarks/benchmark_portfolio.cpp)
        target_link_libraries(benchmark_portfolio PRIVATE qrp_core)
    else()
        message(STATUS "QRP_BUILD_BENCHMARKS=ON but benchmark sources not found; skipping benchmarks")
    endif()
endif()
