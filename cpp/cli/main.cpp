#include <fmt/format.h>
#include <qrp/analytics/monte_carlo_engine.hpp>
#include <qrp/analytics/pnl_explain_service.hpp>
#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/risk_service.hpp>
#include <qrp/analytics/stress_engine.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/io/json_loader.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <iostream>
#include <string>
#include <vector>

void print_help() {
    fmt::print("Usage: qrp_cli <command> [options]\n");
    fmt::print("Commands:\n");
    fmt::print("  price --market <path> --portfolio <path>\n");
    fmt::print("  risk  --market <path> --portfolio <path>\n");
    fmt::print("  pnl-explain --prev-market <path> --curr-market <path> --portfolio <path>\n");
    fmt::print("  stress --market <path> --portfolio <path> --scenarios <path>\n");
    fmt::print("  mc --market <path> --portfolio <path> --paths <number>\n");
}

int handle_price(const std::string& market_path, const std::string& portfolio_path) {
    auto market_dto = qrp::io::load_market(market_path);
    auto portfolio = qrp::io::load_portfolio(portfolio_path);
    qrp::market::MarketSnapshot market(market_dto);
    qrp::analytics::PricingContext context(market.built_state());

    auto results = qrp::analytics::ValuationService::price_portfolio(portfolio, context);

    fmt::print("{:<20} | {:<10} | {:<10}\n", "Trade ID", "NPV", "Currency");
    fmt::print("----------------------------------------------------\n");
    for (const auto& res : results) {
        fmt::print("{:<20} | {:<10.2f} | {:<10}\n", res.trade_id, res.npv, res.currency);
    }
    return 0;
}

int handle_risk(const std::string& market_path, const std::string& portfolio_path) {
    auto market_dto = qrp::io::load_market(market_path);
    auto portfolio = qrp::io::load_portfolio(portfolio_path);

    auto results = qrp::analytics::RiskService::compute_risk(portfolio, market_dto);

    fmt::print("{:<20} | {:<10} | {:<10}\n", "Trade ID", "PV01", "Key Tenors");
    fmt::print("----------------------------------------------------\n");
    for (const auto& res : results) {
        fmt::print("{:<20} | {:<10.4f} | ", res.trade_id, res.pv01);
        for (const auto& [tenor, risk] : res.bucketed_risk) {
            fmt::print("{}: {:.4f} ", tenor, risk);
        }
        fmt::print("\n");
    }
    return 0;
}

int handle_pnl_explain(const std::string& prev_market_path, const std::string& curr_market_path, const std::string& portfolio_path) {
    auto prev_market_dto = qrp::io::load_market(prev_market_path);
    auto curr_market_dto = qrp::io::load_market(curr_market_path);
    auto portfolio = qrp::io::load_portfolio(portfolio_path);

    auto results = qrp::analytics::PnlExplainService::explain_pnl(portfolio, prev_market_dto, curr_market_dto);

    fmt::print("{:<20} | {:<10} | {:<10} | {:<10} | {:<10}\n", "Trade ID", "Total P&L", "Carry", "Market", "Cash");
    fmt::print("--------------------------------------------------------------------------------\n");
    for (const auto& res : results) {
        fmt::print("{:<20} | {:<10.2f} | {:<10.2f} | {:<10.2f} | {:<10.2f}\n", 
                   res.trade_id, res.total_pnl, res.carry_pnl, res.market_move_pnl, res.cash_pnl);
    }
    return 0;
}

int handle_stress(const std::string& market_path, const std::string& portfolio_path, const std::string& scenarios_path) {
    auto market_dto = qrp::io::load_market(market_path);
    auto portfolio = qrp::io::load_portfolio(portfolio_path);
    // Placeholder for scenario loader
    std::vector<qrp::market::ScenarioDefinition> scenarios;
    // ... logic to load scenarios from JSON ...

    auto results = qrp::analytics::StressEngine::run_historical_stress(portfolio, market_dto, scenarios);
    fmt::print("Stress run completed. (Historical scenarios loading placeholder)\n");
    return 0;
}

int handle_mc(const std::string& market_path, const std::string& portfolio_path, int paths) {
    auto market_dto = qrp::io::load_market(market_path);
    auto portfolio = qrp::io::load_portfolio(portfolio_path);

    auto res = qrp::analytics::MonteCarloEngine::run_simulation(portfolio, market_dto, paths);
    fmt::print("Monte Carlo Simulation Results ({} paths):\n", paths);
    fmt::print("95% VaR: {:.2f}\n", res.var_95);
    fmt::print("95% ES : {:.2f}\n", res.expected_shortfall_95);
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_help();
        return 1;
    }

    std::string cmd = argv[1];
    std::vector<std::string> args;
    for (int i = 2; i < argc; ++i) args.push_back(argv[i]);

    try {
        if (cmd == "price") {
            std::string market, portfolio;
            for (size_t i = 0; i < args.size(); ++i) {
                if (args[i] == "--market" && i + 1 < args.size()) market = args[++i];
                if (args[i] == "--portfolio" && i + 1 < args.size()) portfolio = args[++i];
            }
            return handle_price(market, portfolio);
        } else if (cmd == "risk") {
            std::string market, portfolio;
            for (size_t i = 0; i < args.size(); ++i) {
                if (args[i] == "--market" && i + 1 < args.size()) market = args[++i];
                if (args[i] == "--portfolio" && i + 1 < args.size()) portfolio = args[++i];
            }
            return handle_risk(market, portfolio);
        } else if (cmd == "pnl-explain") {
            std::string prev, curr, portfolio;
            for (size_t i = 0; i < args.size(); ++i) {
                if (args[i] == "--prev-market" && i + 1 < args.size()) prev = args[++i];
                if (args[i] == "--curr-market" && i + 1 < args.size()) curr = args[++i];
                if (args[i] == "--portfolio" && i + 1 < args.size()) portfolio = args[++i];
            }
            return handle_pnl_explain(prev, curr, portfolio);
        } else if (cmd == "mc") {
            std::string market, portfolio;
            int paths = 100;
            for (size_t i = 0; i < args.size(); ++i) {
                if (args[i] == "--market" && i + 1 < args.size()) market = args[++i];
                if (args[i] == "--portfolio" && i + 1 < args.size()) portfolio = args[++i];
                if (args[i] == "--paths" && i + 1 < args.size()) paths = std::stoi(args[++i]);
            }
            return handle_mc(market, portfolio, paths);
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
