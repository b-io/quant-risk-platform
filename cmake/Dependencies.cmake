# Find QuantLib
find_package(QuantLib REQUIRED)

# Find nlohmann_json
find_package(nlohmann_json CONFIG REQUIRED)

# Find fmt
find_package(fmt CONFIG REQUIRED)

# Find GTest for tests
find_package(GTest REQUIRED)

# Find SQLite3
find_package(SQLite3 REQUIRED)

# Find spdlog
find_package(spdlog CONFIG REQUIRED)
