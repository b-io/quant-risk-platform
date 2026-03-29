#include <fmt/format.h>
#include <qrp/app/quant_risk_platform.hpp>
#include <qrp/persistence/sqlite_storage_backend.hpp>
#include <qrp/util/logger.hpp>
#include <string>
#include <vector>
#include <memory>

void print_help() {
    fmt::print("Usage: qrp_cli <command> [options]\n");
    fmt::print("Commands:\n");
    fmt::print("  init-db\n");
    fmt::print("  import-market <json_path>\n");
    fmt::print("  import-portfolio <json_path>\n");
    fmt::print("  import-scenarios <json_path>\n");
    fmt::print("  run-valuation --portfolio <id> --snapshot <id>\n");
    fmt::print("  run-risk --portfolio <id> --snapshot <id>\n");
    fmt::print("  run-hvar --portfolio <id> --snapshot <id> --scenarios <id>\n");
    fmt::print("  report <run_id>\n");
    fmt::print("  compare <run_id_1> <run_id_2>\n");
    fmt::print("  list\n");
}

int main(int argc, char** argv) {
    qrp::util::Logger::initialize();

    if (argc < 2) {
        print_help();
        return 1;
    }

    std::string cmd = argv[1];
    
    // Initialize storage and platform
    auto storage = std::make_shared<qrp::persistence::SQLiteStorageBackend>("var/quant_risk_platform.sqlite");
    qrp::app::QuantRiskPlatform platform(storage);

    try {
        if (cmd == "init-db") {
            platform.initialize();
            return 0;
        } else if (cmd == "import-market") {
            if (argc < 3) throw std::runtime_error("Missing JSON path");
            platform.import_market_snapshot(argv[2]);
            return 0;
        } else if (cmd == "import-portfolio") {
            if (argc < 3) throw std::runtime_error("Missing JSON path");
            platform.import_portfolio(argv[2]);
            return 0;
        } else if (cmd == "import-scenarios") {
            if (argc < 3) throw std::runtime_error("Missing JSON path");
            platform.import_scenario_set(argv[2]);
            return 0;
        } else if (cmd == "run-valuation") {
            std::string portfolio, snapshot;
            for (int i = 2; i < argc; ++i) {
                if (std::string(argv[i]) == "--portfolio" && i + 1 < argc) portfolio = argv[++i];
                if (std::string(argv[i]) == "--snapshot" && i + 1 < argc) snapshot = argv[++i];
            }
            std::string run_id = platform.run_valuation(portfolio, snapshot);
            fmt::print("Run ID: {}\n", run_id);
            return 0;
        } else if (cmd == "run-risk") {
            std::string portfolio, snapshot;
            for (int i = 2; i < argc; ++i) {
                if (std::string(argv[i]) == "--portfolio" && i + 1 < argc) portfolio = argv[++i];
                if (std::string(argv[i]) == "--snapshot" && i + 1 < argc) snapshot = argv[++i];
            }
            std::string run_id = platform.run_risk(portfolio, snapshot);
            fmt::print("Run ID: {}\n", run_id);
            return 0;
        } else if (cmd == "run-hvar") {
            std::string portfolio, snapshot, scenarios;
            for (int i = 2; i < argc; ++i) {
                if (std::string(argv[i]) == "--portfolio" && i + 1 < argc) portfolio = argv[++i];
                if (std::string(argv[i]) == "--snapshot" && i + 1 < argc) snapshot = argv[++i];
                if (std::string(argv[i]) == "--scenarios" && i + 1 < argc) scenarios = argv[++i];
            }
            std::string run_id = platform.run_historical_var(portfolio, snapshot, scenarios);
            fmt::print("Run ID: {}\n", run_id);
            return 0;
        } else if (cmd == "report") {
            if (argc < 3) throw std::runtime_error("Missing run_id");
            platform.get_run_report(argv[2]);
            return 0;
        } else if (cmd == "compare") {
            if (argc < 4) throw std::runtime_error("Missing run_id_1 and run_id_2");
            platform.compare_runs(argv[2], argv[3]);
            return 0;
        } else if (cmd == "list") {
            platform.list_data();
            return 0;
        } else {
            fmt::print("Unknown command: {}\n", cmd);
            print_help();
            return 1;
        }
    } catch (const std::exception& e) {
        fmt::print(stderr, "Error: {}\n", e.what());
        return 1;
    }
}
