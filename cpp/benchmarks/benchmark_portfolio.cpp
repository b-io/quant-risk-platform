// Benchmarks end-to-end portfolio valuation and risk throughput over the demo multi-asset book.

#include <qrp/analytics/risk_service.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/domain/factors.hpp>
#include <qrp/io/json_loader.hpp>
#include <qrp/market/market_snapshot.hpp>

#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

struct BenchmarkConfig {
    std::filesystem::path data_root = "data";
    int iterations = 100;
};

void print_usage(const char* executable) {
    std::cout << "Usage: " << executable << " [--iterations N] [--data-root PATH]\n";
}

BenchmarkConfig parse_args(int argc, char** argv) {
    BenchmarkConfig config;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            std::exit(0);
        }
        if (arg == "--iterations" || arg == "-n") {
            if (++i >= argc) {
                throw std::invalid_argument("--iterations requires a positive integer");
            }
            config.iterations = std::stoi(argv[i]);
            if (config.iterations <= 0) {
                throw std::invalid_argument("--iterations must be positive");
            }
            continue;
        }
        if (arg == "--data-root") {
            if (++i >= argc) {
                throw std::invalid_argument("--data-root requires a path");
            }
            config.data_root = argv[i];
            continue;
        }
        throw std::invalid_argument("Unknown benchmark argument: " + arg);
    }
    return config;
}

std::pair<std::vector<qrp::domain::FactorDefinition>, std::vector<qrp::domain::FactorBinding>>
load_factor_configuration(const std::filesystem::path& scenario_path) {
    std::vector<qrp::domain::FactorDefinition> factors;
    std::vector<qrp::domain::FactorBinding> bindings;

    std::ifstream scenario_stream(scenario_path);
    if (!scenario_stream.is_open()) {
        throw std::runtime_error("Unable to open scenario configuration: " + scenario_path.string());
    }

    nlohmann::json scenario_data;
    scenario_stream >> scenario_data;

    for (const auto& factor_json : scenario_data.at("factors")) {
        qrp::domain::FactorDefinition factor;
        factor.factor_id = factor_json.at("factor_id").get<std::string>();
        factor.factor_type = qrp::domain::parse_factor_type(factor_json.at("factor_type").get<std::string>());
        factor.shock_measure = qrp::domain::parse_shock_measure(factor_json.at("shock_measure").get<std::string>());
        factor.currency = qrp::domain::from_string(factor_json.value("currency", "UNKNOWN"));
        factor.curve_id = factor_json.value("curve_id", "");
        factor.tenor = factor_json.value("tenor", "");
        factor.description = factor_json.value("description", "");
        if (factor_json.contains("quote_ids")) {
            factor.quote_ids = factor_json.at("quote_ids").get<std::vector<std::string>>();
        }
        factors.push_back(factor);
    }

    for (const auto& binding_json : scenario_data.at("bindings")) {
        qrp::domain::FactorBinding binding;
        binding.factor_id = binding_json.at("factor_id").get<std::string>();
        binding.quote_id = binding_json.at("quote_id").get<std::string>();
        binding.shock_measure = qrp::domain::parse_shock_measure(binding_json.at("shock_measure").get<std::string>());
        binding.weight = binding_json.value("weight", 1.0);
        binding.transform = binding_json.value("transform", "");
        binding.selector_json = binding_json.value("selector_json", "");
        bindings.push_back(binding);
    }

    return {factors, bindings};
}

double elapsed_ms(std::chrono::high_resolution_clock::time_point start,
                  std::chrono::high_resolution_clock::time_point end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

} // namespace

int main(int argc, char** argv) {
    try {
        const auto config = parse_args(argc, argv);
        const auto market_path = config.data_root / "market" / "demo_market.json";
        const auto portfolio_path = config.data_root / "portfolios" / "demo_portfolio.json";
        const auto scenario_path = config.data_root / "scenarios" / "demo_scenarios.json";

        std::cout << "Starting Portfolio Benchmark..." << std::endl;
        std::cout << "Data root: " << config.data_root.string() << std::endl;
        std::cout << "Iterations: " << config.iterations << std::endl;

        auto market_dto = qrp::io::load_market(market_path.string());
        auto portfolio = qrp::io::load_portfolio(portfolio_path.string());
        auto [factors, bindings] = load_factor_configuration(scenario_path);
        qrp::market::MarketSnapshot market(market_dto);
        qrp::analytics::PricingContext context(market.built_state());

        std::cout << "Trades: " << portfolio.trades.size() << std::endl;
        std::cout << "Factors: " << factors.size() << ", bindings: " << bindings.size() << std::endl;

        auto start = std::chrono::high_resolution_clock::now();
        std::size_t valuation_count = 0;
        for (int i = 0; i < config.iterations; ++i) {
            auto results = qrp::analytics::ValuationService::price_portfolio(portfolio, context);
            valuation_count += results.size();
        }
        auto end = std::chrono::high_resolution_clock::now();
        const double valuation_ms = elapsed_ms(start, end);
        const double valuation_seconds = valuation_ms / 1000.0;

        std::cout << "Valuation Benchmark (" << config.iterations << " iterations): " << valuation_ms << " ms"
                  << std::endl;
        std::cout << "Valuation results: " << valuation_count << std::endl;
        if (valuation_seconds > 0.0) {
            std::cout << "Trades per second: "
                      << (static_cast<double>(config.iterations) * static_cast<double>(portfolio.trades.size()) /
                          valuation_seconds)
                      << std::endl;
        }

        start = std::chrono::high_resolution_clock::now();
        auto risk_results = qrp::analytics::RiskService::compute_risk(portfolio, market_dto, factors, bindings);
        end = std::chrono::high_resolution_clock::now();
        const double risk_ms = elapsed_ms(start, end);

        std::cout << "Risk Benchmark (PV01 + CS01 + FX + bucketed): " << risk_ms << " ms" << std::endl;
        std::cout << "Risk results: " << risk_results.size() << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
