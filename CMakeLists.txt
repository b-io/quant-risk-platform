cmake_minimum_required(VERSION 3.20)

message(STATUS "")
message(STATUS "========== Environment ==========")
if(DEFINED ENV{QRP_ENV_PROFILE})
  message(STATUS "QRP env profile      : $ENV{QRP_ENV_PROFILE}")
else()
  message(STATUS "QRP env profile      : (not detected)")
endif()
if(DEFINED ENV{QRP_ENV_FILE})
  message(STATUS "QRP env file         : $ENV{QRP_ENV_FILE}")
endif()
if(DEFINED ENV{QRP_MSVC_ENV_READY})
  message(STATUS "MSVC env ready       : $ENV{QRP_MSVC_ENV_READY}")
endif()
if(DEFINED ENV{DEV_TOOLS_ROOT})
  message(STATUS "DEV_TOOLS_ROOT       : $ENV{DEV_TOOLS_ROOT}")
endif()
if(DEFINED ENV{VCPKG_ROOT})
  message(STATUS "VCPKG_ROOT           : $ENV{VCPKG_ROOT}")
endif()
if(DEFINED ENV{VCPKG_TARGET_TRIPLET})
  message(STATUS "VCPKG_TARGET_TRIPLET : $ENV{VCPKG_TARGET_TRIPLET}")
endif()
message(STATUS "=================================")
message(STATUS "")

project(quant-risk-platform VERSION 0.1.0 LANGUAGES CXX)

# -----------------------
# Build options
# -----------------------
option(QRP_BUILD_CLI "Build the C++ CLI" ON)
option(QRP_BUILD_BENCHMARKS "Build benchmark targets" OFF)
option(QRP_BUILD_PYTHON "Build Python bindings" OFF)
option(QRP_BUILD_SHARED "Build qrp_core as a shared library" OFF)
option(QRP_ENABLE_COVERAGE "Enable GCC/Clang coverage instrumentation" OFF)
option(QRP_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
set(QRP_COVERAGE_MIN_LINE 95 CACHE STRING "Minimum line coverage percentage enforced by the coverage target")
include(CTest) # defines BUILD_TESTING

# -----------------------
# Summary banner (early)
# -----------------------
message(STATUS "")
message(STATUS "========== Build Configuration ==========")
message(STATUS "CMake version        : ${CMAKE_VERSION}")
message(STATUS "Generator            : ${CMAKE_GENERATOR}")
if(CMAKE_GENERATOR_PLATFORM)
  message(STATUS "Generator platform   : ${CMAKE_GENERATOR_PLATFORM}")
endif()
if(CMAKE_GENERATOR_TOOLSET)
  message(STATUS "Generator toolset    : ${CMAKE_GENERATOR_TOOLSET}")
endif()
if(CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Configuration types  : ${CMAKE_CONFIGURATION_TYPES}")
else()
  message(STATUS "Build type           : ${CMAKE_BUILD_TYPE}")
endif()
message(STATUS "Source dir           : ${CMAKE_SOURCE_DIR}")
message(STATUS "Binary dir           : ${CMAKE_BINARY_DIR}")
message(STATUS "C++ compiler         : ${CMAKE_CXX_COMPILER}")
message(STATUS "C++ compiler version : ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build CLI            : ${QRP_BUILD_CLI}")
message(STATUS "Build benchmarks     : ${QRP_BUILD_BENCHMARKS}")
message(STATUS "Build coverage       : ${QRP_ENABLE_COVERAGE}")
message(STATUS "Build Python         : ${QRP_BUILD_PYTHON}")
message(STATUS "Build shared core    : ${QRP_BUILD_SHARED}")
message(STATUS "Build tests          : ${BUILD_TESTING}")
if(DEFINED CMAKE_TOOLCHAIN_FILE)
  message(STATUS "Toolchain file       : ${CMAKE_TOOLCHAIN_FILE}")
endif()
if(DEFINED VCPKG_TARGET_TRIPLET)
  message(STATUS "vcpkg triplet        : ${VCPKG_TARGET_TRIPLET}")
endif()
# Python interpreter (may be empty if bindings disabled)
if(QRP_BUILD_PYTHON)
  find_package(Python3 3.8 COMPONENTS Interpreter Development.Module REQUIRED)
endif()

if(DEFINED Python3_EXECUTABLE)
  message(STATUS "Python executable    : ${Python3_EXECUTABLE}")
else()
  message(STATUS "Python executable    : (not used)")
endif()
message(STATUS "=========================================")
message(STATUS "")

# -----------------------
# Languages and standards
# -----------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Prefer per-target features too

# MSVC runtime: MD/MDd is generally preferred; required for Python extension
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# -----------------------
# Directories
# -----------------------
set(QRP_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cpp/include)
set(QRP_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cpp/src)

# -----------------------
# Dependencies (C++ only; tests guarded within)
# -----------------------
include(cmake/Dependencies.cmake)

# -----------------------
# Core library
# -----------------------
if(QRP_BUILD_SHARED)
  set(_QRP_CORE_LIBRARY_TYPE SHARED)
else()
  set(_QRP_CORE_LIBRARY_TYPE STATIC)
endif()

add_library(qrp_core ${_QRP_CORE_LIBRARY_TYPE}
    cpp/src/analytics/cashflow_extractor.cpp
    cpp/src/analytics/covariance_estimator.cpp
    cpp/src/analytics/lsmc/lsmc_engine.cpp
    cpp/src/analytics/monte_carlo_engine.cpp
    cpp/src/analytics/pnl_explain_service.cpp
    cpp/src/analytics/pricing_context.cpp
    cpp/src/analytics/product_pricing_registry.cpp
    cpp/src/analytics/revaluation_session.cpp
    cpp/src/analytics/risk_service.cpp
    cpp/src/analytics/simulation_primitives.cpp
    cpp/src/analytics/stress_engine.cpp
    cpp/src/analytics/valuation_service.cpp
    cpp/src/analytics/var_contribution_service.cpp
    cpp/src/app/quant_risk_platform.cpp
    cpp/src/conventions/market_convention_registry.cpp
    cpp/src/conventions/rates_market_conventions.cpp
    cpp/src/instruments/commodity_instrument_factory.cpp
    cpp/src/instruments/credit_instrument_factory.cpp
    cpp/src/instruments/equity_instrument_factory.cpp
    cpp/src/instruments/fx_instrument_factory.cpp
    cpp/src/instruments/instrument_factory.cpp
    cpp/src/instruments/rates_instrument_factory.cpp
    cpp/src/io/json_loader.cpp
    cpp/src/market/market_snapshot.cpp
    cpp/src/market/scenario_engine.cpp
    cpp/src/optimization/adapters/cvxpy_adapter.cpp
    cpp/src/optimization/portfolio_optimization_engine.cpp
    cpp/src/persistence/repositories.cpp
    cpp/src/persistence/sqlite_storage_backend.cpp
    cpp/src/persistence/sqlite_storage_factors.cpp
    cpp/src/persistence/sqlite_storage_market.cpp
    cpp/src/persistence/sqlite_storage_portfolio.cpp
    cpp/src/persistence/sqlite_storage_results.cpp
    cpp/src/persistence/sqlite_storage_schema.cpp
    cpp/src/util/logger.cpp
)
if(QRP_BUILD_PYTHON)
  # The Python extension is a shared module and may link the static core library.
  # On ELF platforms that requires qrp_core objects to be compiled with -fPIC.
  set_target_properties(qrp_core PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
if(MSVC)
  target_compile_options(qrp_core PRIVATE /FS)
endif()
target_include_directories(qrp_core PUBLIC ${QRP_INCLUDE_DIR})
# PUBLIC if headers expose these types; keep PRIVATE for link-only deps
target_link_libraries(qrp_core
  PUBLIC
    Eigen3::Eigen
    nlohmann_json::nlohmann_json
    QuantLib::QuantLib
    spdlog::spdlog
    SQLite::SQLite3
  PRIVATE
    fmt::fmt
)
if(WIN32 AND QRP_BUILD_SHARED)
  # Keep the current public headers working when qrp_core is built as a DLL.
  set_target_properties(qrp_core PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

target_compile_features(qrp_core PUBLIC cxx_std_20)

# Warnings
function(qrp_apply_project_warnings target_name)
  if(MSVC)
    target_compile_options(${target_name} PRIVATE
      /W4
      /utf-8
      $<$<BOOL:${QRP_WARNINGS_AS_ERRORS}>:/WX>
    )
  else()
    target_compile_options(${target_name} PRIVATE
      -Wall
      -Wextra
      -Wconversion
      -Wpedantic
      $<$<BOOL:${QRP_WARNINGS_AS_ERRORS}>:-Werror>
    )
  endif()
endfunction()

function(qrp_apply_msvc_debug_symbols target_name)
  if(NOT MSVC)
    return()
  endif()

  get_target_property(_QRP_TARGET_TYPE ${target_name} TYPE)
  target_compile_options(${target_name} PRIVATE /Zi)
  set_target_properties(${target_name} PROPERTIES
    COMPILE_PDB_NAME "${target_name}_compile"
  )

  if(NOT _QRP_TARGET_TYPE STREQUAL "STATIC_LIBRARY")
    target_link_options(${target_name} PRIVATE /DEBUG)
    set_target_properties(${target_name} PROPERTIES
      PDB_NAME "${target_name}"
    )
  endif()
endfunction()

function(qrp_apply_coverage target_name)
  if(NOT QRP_ENABLE_COVERAGE)
    return()
  endif()

  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${target_name} PRIVATE -O0 -g --coverage)
    target_link_options(${target_name} PRIVATE --coverage)
  elseif(MSVC)
    message(WARNING "QRP_ENABLE_COVERAGE=ON is currently supported only with GCC/Clang coverage instrumentation; target ${target_name} will not be instrumented.")
  else()
    message(WARNING "QRP_ENABLE_COVERAGE=ON is not configured for compiler '${CMAKE_CXX_COMPILER_ID}'; target ${target_name} will not be instrumented.")
  endif()
endfunction()

qrp_apply_coverage(qrp_core)
qrp_apply_project_warnings(qrp_core)
qrp_apply_msvc_debug_symbols(qrp_core)

function(qrp_copy_core_runtime target_name)
  if(WIN32 AND QRP_BUILD_SHARED)
    add_custom_command(TARGET ${target_name} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:qrp_core>
        $<TARGET_FILE_DIR:${target_name}>
      COMMENT "Copying qrp_core runtime beside ${target_name}"
      VERBATIM
    )
  endif()
endfunction()

# -----------------------
# CLI (optional)
# -----------------------
if(QRP_BUILD_CLI)
  add_executable(qrp_cli cpp/cli/main.cpp)
  target_link_libraries(qrp_cli PRIVATE qrp_core fmt::fmt)
  qrp_apply_coverage(qrp_cli)
  qrp_apply_project_warnings(qrp_cli)
  qrp_apply_msvc_debug_symbols(qrp_cli)
  qrp_copy_core_runtime(qrp_cli)
else()
  message(STATUS "QRP_BUILD_CLI=OFF - CLI target is skipped")
endif()

# -----------------------
# Python bindings (optional)
# -----------------------
if(QRP_BUILD_PYTHON)
  # Windows Debug safety
  if(WIN32)
    # Windows Debug safety for Python
    # We want to avoid cases where Debug is the ONLY or the default configuration
    # if Debug is part of a multi-config generator, we allow it to proceed as long as
    # the user is aware they might need a debug Python runtime if they actually build Debug.
    # However, to avoid the FATAL_ERROR on Release presets, we check if Debug is the ONLY config or if CMAKE_BUILD_TYPE is Debug.
    if(CMAKE_CONFIGURATION_TYPES)
      if(CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
        set(_QRP_CFG_IS_ONLY_DEBUG TRUE)
      endif()
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR _QRP_CFG_IS_ONLY_DEBUG)
      if(NOT QRP_ALLOW_PYTHON_DEBUG)
        message(FATAL_ERROR "Python bindings on Windows in Debug mode require a matching debug CPython runtime (python3x_d.dll) and are not supported by default. Please use Release or RelWithDebInfo, or set QRP_ALLOW_PYTHON_DEBUG=ON if you have a debug Python installation.")
      else()
        message(WARNING "Python bindings on Windows in Debug mode enabled via QRP_ALLOW_PYTHON_DEBUG=ON. Ensure you have the matching debug CPython runtime installed.")
      endif()
    endif()
  endif()

  pybind11_add_module(quant_risk_platform
    cpp/bindings/bind_analytics.cpp
    cpp/bindings/bind_domain.cpp
    cpp/bindings/bind_io.cpp
    cpp/bindings/bind_market.cpp
    cpp/bindings/bind_revaluation.cpp
    cpp/bindings/module.cpp
  )
  qrp_apply_coverage(quant_risk_platform)
  qrp_apply_project_warnings(quant_risk_platform)
  qrp_apply_msvc_debug_symbols(quant_risk_platform)

  # Windows Debug safety
  if(MSVC)
    set_target_properties(quant_risk_platform PROPERTIES
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
  endif()

  target_link_libraries(quant_risk_platform PRIVATE qrp_core Python3::Module)
  qrp_copy_core_runtime(quant_risk_platform)

  # Put the built .pyd/.so into a predictable subfolder for smoke tests
  set(_QRP_PY_OUT ${CMAKE_BINARY_DIR}/python)
  foreach(cfg Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${cfg} CFGU)
    set_target_properties(quant_risk_platform PROPERTIES
      ARCHIVE_OUTPUT_DIRECTORY_${CFGU} "${_QRP_PY_OUT}/${cfg}"
      LIBRARY_OUTPUT_DIRECTORY_${CFGU} "${_QRP_PY_OUT}/${cfg}"
      RUNTIME_OUTPUT_DIRECTORY_${CFGU} "${_QRP_PY_OUT}/${cfg}"
    )
  endforeach()

  # Smoke tests for Python import
  if(BUILD_TESTING)
    if(CMAKE_CONFIGURATION_TYPES)
      foreach(cfg Release RelWithDebInfo)
        add_test(NAME python_import_${cfg}
          COMMAND ${Python3_EXECUTABLE} -c "import sys,importlib; sys.path.insert(0, r'${_QRP_PY_OUT}/${cfg}'); m=importlib.import_module('quant_risk_platform'); print(m.__doc__)"
        )
        set_tests_properties(python_import_${cfg} PROPERTIES CONFIGURATIONS ${cfg})
      endforeach()
    else()
      add_test(NAME python_import
        COMMAND ${Python3_EXECUTABLE} -c "import sys,importlib; sys.path.insert(0, r'${_QRP_PY_OUT}/${CMAKE_BUILD_TYPE}'); m=importlib.import_module('quant_risk_platform'); print(m.__doc__)"
      )
    endif()
  endif()
else()
  message(STATUS "QRP_BUILD_PYTHON=OFF - Python bindings are disabled")
endif()

# -----------------------
# Tests (optional via BUILD_TESTING)
# -----------------------
if(BUILD_TESTING)
  add_subdirectory(tests)
else()
  message(STATUS "BUILD_TESTING=OFF - C++ tests are disabled")
endif()

# -----------------------
# Coverage (optional via QRP_ENABLE_COVERAGE)
# -----------------------
if(QRP_ENABLE_COVERAGE AND BUILD_TESTING AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  find_program(GCOVR_EXECUTABLE gcovr)
  if(GCOVR_EXECUTABLE)
    if(NOT Python3_Interpreter_FOUND)
      find_package(Python3 COMPONENTS Interpreter REQUIRED)
    endif()
    add_custom_target(coverage
      COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} --output-on-failure
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage/cpp
      COMMAND ${GCOVR_EXECUTABLE}
        --exclude ${CMAKE_SOURCE_DIR}/tests
        --filter ${CMAKE_SOURCE_DIR}/cpp
        --html-details ${CMAKE_BINARY_DIR}/coverage/cpp/index.html
        --print-summary
        --root ${CMAKE_SOURCE_DIR}
        --xml ${CMAKE_BINARY_DIR}/coverage/cpp/coverage.xml
      COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/coverage/cpp/coverage_metric.py
        --html-report ${CMAKE_BINARY_DIR}/coverage/cpp/index.html
        --json ${CMAKE_BINARY_DIR}/coverage/cpp/coverage_metric.json
        --markdown ${CMAKE_BINARY_DIR}/coverage/cpp/coverage_metric.md
        --threshold ${QRP_COVERAGE_MIN_LINE}
        --fail-under-threshold
        --xml ${CMAKE_BINARY_DIR}/coverage/cpp/coverage.xml
      COMMENT "Running tests and generating coverage report"
      DEPENDS integration_tests unit_tests
      VERBATIM
    )
  else()
    message(WARNING "gcovr was not found; install it to use the coverage target.")
  endif()
endif()

# -----------------------
# Benchmarks (guarded)
# -----------------------
if(QRP_BUILD_BENCHMARKS)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cpp/benchmarks/benchmark_portfolio.cpp")
    add_executable(benchmark_portfolio cpp/benchmarks/benchmark_portfolio.cpp)
    target_link_libraries(benchmark_portfolio PRIVATE qrp_core)
    qrp_apply_project_warnings(benchmark_portfolio)
    qrp_apply_msvc_debug_symbols(benchmark_portfolio)
    qrp_copy_core_runtime(benchmark_portfolio)
  else()
    message(STATUS "QRP_BUILD_BENCHMARKS=ON but benchmark sources not found; skipping benchmarks")
  endif()
endif()
