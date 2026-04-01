# Dependencies
include(CTest)

# Find fmt
find_package(fmt CONFIG REQUIRED)
message(STATUS "Found fmt: ${fmt_DIR}")

# Find GTest only when tests are enabled
if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    message(STATUS "Found GTest: ${GTest_DIR}")
else()
    message(STATUS "BUILD_TESTING=OFF — GTest lookup skipped")
endif()

# Find nlohmann_json
find_package(nlohmann_json CONFIG REQUIRED)
message(STATUS "Found nlohmann_json: ${nlohmann_json_DIR}")

# Find QuantLib
find_package(QuantLib CONFIG REQUIRED)
message(STATUS "Found QuantLib: ${QuantLib_DIR}")

# Find spdlog
find_package(spdlog CONFIG REQUIRED)
message(STATUS "Found spdlog: ${spdlog_DIR}")

# Find SQLite3
find_package(unofficial-sqlite3 CONFIG REQUIRED)
if (NOT TARGET SQLite::SQLite3 AND TARGET unofficial::sqlite3::sqlite3)
    add_library(SQLite::SQLite3 ALIAS unofficial::sqlite3::sqlite3)
endif()
message(STATUS "Found SQLite3: ${unofficial-sqlite3_DIR}")
