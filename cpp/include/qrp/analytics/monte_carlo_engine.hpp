#pragma once
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/portfolio.hpp>
#include <qrp/domain/factors.hpp>
#include <qrp/market/scenario_engine.hpp>
#include <ql/math/matrix.hpp>
#include <string>
#include <vector>

namespace qrp::analytics {

enum class MonteCarloMode {
    HorizonShockOnly,
    AgedHorizonRevaluation
};

struct MonteCarloConfig {
    int num_paths = 1000;
    std::uint64_t seed = 42;
    double horizon_days = 1.0;
    MonteCarloMode mode = MonteCarloMode::HorizonShockOnly;
    bool covariance_is_already_horizon_scaled = false;
    bool require_bindings = true;
};

struct PathTrace {
    int path_index;
    std::map<std::string, double> factor_shocks;
    std::map<std::string, std::pair<double, double>> quote_before_after;
    double portfolio_value_before;
    double portfolio_value_after;
    double path_pnl;

    // Aged mode extensions
    double portfolio_value_frozen_aged = 0.0;
    double aging_pnl = 0.0;
    double market_pnl = 0.0;
    double total_pnl = 0.0;
    std::string valuation_date_before;
    std::string valuation_date_after;

    // Diagnostics per path
    int num_priced = 0;
    int num_expired = 0;
    int num_failed = 0;
    int num_unsupported = 0;
};

struct SimulationResult {
    std::vector<double> portfolio_values;
    std::vector<double> portfolio_pnls;
    double base_portfolio_value = 0.0;
    double var_95 = 0.0;
    double expected_shortfall_95 = 0.0;
    std::uint64_t seed = 0;
    double horizon_days = 1.0;
    MonteCarloMode mode = MonteCarloMode::HorizonShockOnly;
    std::vector<std::string> factor_ids;
    std::vector<PathTrace> traces;

    // Diagnostics for the simulation baseline
    int num_trades_total = 0;
    int num_trades_priced_t0 = 0;
    int num_trades_failed_t0 = 0;
    int num_trades_priced_tH = 0;
    int num_trades_expired_tH = 0;
    int num_trades_failed_tH = 0;
    int num_trades_unsupported_tH = 0;

    // Failure log (trade_id -> error message)
    std::map<std::string, std::string> construction_errors;
};

class MonteCarloEngine {
public:
    /**
     * @brief Run Monte Carlo simulation using the canonical production API.
     * Requires factor definitions, bindings, and a valid covariance matrix.
     */
    static SimulationResult run_simulation(
        const domain::Portfolio& portfolio,
        const domain::MarketSnapshot& base_market_dto,
        const std::vector<domain::FactorDefinition>& factors,
        const std::vector<domain::FactorBinding>& bindings,
        const QuantLib::Matrix& covariance,
        const MonteCarloConfig& config);
};

} // namespace qrp::analytics
