cmake_minimum_required(VERSION 3.20)
project(quant-risk-platform VERSION 0.1.0 LANGUAGES CXX)

# -----------------------
# Build options
# -----------------------
option(QRP_BUILD_CLI "Build the C++ CLI" ON)
option(QRP_BUILD_PYTHON "Build Python bindings" OFF)
option(QRP_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
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
message(STATUS "Build Python         : ${QRP_BUILD_PYTHON}")
message(STATUS "Build tests          : ${BUILD_TESTING}")
if(DEFINED CMAKE_TOOLCHAIN_FILE)
  message(STATUS "Toolchain file       : ${CMAKE_TOOLCHAIN_FILE}")
endif()
if(DEFINED VCPKG_TARGET_TRIPLET)
  message(STATUS "vcpkg triplet        : ${VCPKG_TARGET_TRIPLET}")
endif()
# Python interpreter (may be empty if bindings disabled)
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
add_library(qrp_core
    cpp/src/analytics/monte_carlo_engine.cpp
    cpp/src/analytics/pnl_explain_service.cpp
    cpp/src/analytics/risk_service.cpp
    cpp/src/analytics/stress_engine.cpp
    cpp/src/analytics/valuation_service.cpp
    cpp/src/app/quant_risk_platform.cpp
    cpp/src/conventions/market_convention_registry.cpp
    cpp/src/instruments/instrument_factory.cpp
    cpp/src/io/json_loader.cpp
    cpp/src/market/market_snapshot.cpp
    cpp/src/market/scenario_engine.cpp
    cpp/src/persistence/repositories.cpp
    cpp/src/persistence/sqlite_storage_backend.cpp
    cpp/src/util/logger.cpp
)
if(MSVC)
  target_compile_options(qrp_core PRIVATE /FS)
endif()
target_include_directories(qrp_core PUBLIC ${QRP_INCLUDE_DIR})
# PUBLIC if headers expose these types; keep PRIVATE for link-only deps
target_link_libraries(qrp_core PRIVATE fmt::fmt nlohmann_json::nlohmann_json QuantLib::QuantLib spdlog::spdlog SQLite::SQLite3)

target_compile_features(qrp_core PUBLIC cxx_std_20)

# Warnings
if(MSVC)
  target_compile_options(qrp_core PRIVATE /W4 $<$<BOOL:${QRP_WARNINGS_AS_ERRORS}>:/WX>)
else()
  target_compile_options(qrp_core PRIVATE -Wall -Wextra -Wconversion -Wpedantic $<$<BOOL:${QRP_WARNINGS_AS_ERRORS}>:-Werror>)
endif()

# -----------------------
# CLI (optional)
# -----------------------
if(QRP_BUILD_CLI)
  add_executable(qrp_cli cpp/cli/main.cpp)
  target_link_libraries(qrp_cli PRIVATE qrp_core fmt::fmt)
else()
  message(STATUS "QRP_BUILD_CLI=OFF — CLI target is skipped")
endif()

# -----------------------
# Python bindings (optional)
# -----------------------
if(QRP_BUILD_PYTHON)
  set(PYBIND11_FINDPYTHON ON)

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

  find_package(Python3 3.8 COMPONENTS Interpreter Development.Module REQUIRED)
  message(STATUS "Python interpreter   : ${Python3_EXECUTABLE}")

  find_package(pybind11 CONFIG REQUIRED)
  pybind11_add_module(quant_risk_platform cpp/bindings/module.cpp)

  # Ensure dynamic CRT
  if(MSVC)
    set_target_properties(quant_risk_platform PROPERTIES
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL"
    )
  endif()

  target_link_libraries(quant_risk_platform PRIVATE qrp_core Python3::Module)

  # Put the built .pyd/.so into a predictable subfolder for smoke tests
  set(_QRP_PY_OUT ${CMAKE_BINARY_DIR}/python)
  foreach(cfg Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${cfg} CFGU)
    set_target_properties(quant_risk_platform PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY_${CFGU} "${_QRP_PY_OUT}/${cfg}"
      RUNTIME_OUTPUT_DIRECTORY_${CFGU} "${_QRP_PY_OUT}/${cfg}"
      ARCHIVE_OUTPUT_DIRECTORY_${CFGU} "${_QRP_PY_OUT}/${cfg}"
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
  message(STATUS "QRP_BUILD_PYTHON=OFF — Python bindings are disabled")
endif()

# -----------------------
# Tests (optional via BUILD_TESTING)
# -----------------------
if(BUILD_TESTING)
  add_subdirectory(tests)
else()
  message(STATUS "BUILD_TESTING=OFF — C++ tests are disabled")
endif()

# -----------------------
# Benchmarks (guarded)
# -----------------------
option(QRP_BUILD_BENCHMARKS "Build benchmark targets" OFF)
if(QRP_BUILD_BENCHMARKS)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmarks/benchmark_portfolio.cpp")
    add_executable(benchmark_portfolio benchmarks/benchmark_portfolio.cpp)
    target_link_libraries(benchmark_portfolio PRIVATE qrp_core)
  else()
    message(STATUS "QRP_BUILD_BENCHMARKS=ON but benchmark sources not found; skipping benchmarks")
  endif()
endif()
