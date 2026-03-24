cmake_minimum_required(VERSION 3.20)
project(quant-risk-platform VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define directories
set(QRP_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cpp/include)
set(QRP_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cpp/src)

# Dependencies
include(cmake/Dependencies.cmake)

# Library
file(GLOB_RECURSE QRP_SOURCES "${QRP_SRC_DIR}/*.cpp")
add_library(qrp_core ${QRP_SOURCES})
target_include_directories(qrp_core PUBLIC ${QRP_INCLUDE_DIR})
target_link_libraries(qrp_core PRIVATE QuantLib::QuantLib nlohmann_json::nlohmann_json fmt::fmt)

# CLI
add_executable(qrp_cli cpp/cli/main.cpp)
target_link_libraries(qrp_cli PRIVATE qrp_core)

# Python bindings
find_package(pybind11 REQUIRED)
pybind11_add_module(quant_risk_platform cpp/bindings/module.cpp)
target_link_libraries(quant_risk_platform PRIVATE qrp_core)

# Tests
enable_testing()
add_subdirectory(tests)

# Benchmarks
add_executable(benchmark_portfolio benchmarks/benchmark_portfolio.cpp)
target_link_libraries(benchmark_portfolio PRIVATE qrp_core)
