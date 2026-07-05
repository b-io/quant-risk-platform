#pragma once

// Declares factor-based Monte Carlo configuration, path diagnostics, and simulation results.

#include <qrp/domain/factors.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/portfolio.hpp>
#include <qrp/market/scenario_engine.hpp>

#include <ql/math/matrix.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace qrp::analytics {

/**
 * @brief Controls whether paths revalue only shocked markets or age to a horizon date.
 */
enum class MonteCarloMode {
    AgedHorizonRevaluation,
    HorizonShockOnly
};

/**
 * @brief Runtime configuration for factor-based Monte Carlo simulation.
 */
struct MonteCarloConfig {
    int num_paths = 1000;                                // Number of Monte Carlo paths.
    std::uint64_t seed = 42;                             // Random seed for reproducible shocks.
    double horizon_days = 1.0;                           // Risk horizon in calendar days.
    MonteCarloMode mode = MonteCarloMode::HorizonShockOnly; // Revaluation mode.
    bool covariance_is_already_horizon_scaled = false;   // Treat covariance as pre-scaled to the horizon.
    bool require_bindings = true;                        // Fail when factor-to-quote bindings are incomplete.
};

/**
 * @brief Optional per-path diagnostics for shocked factors, quote moves, and PnL.
 */
struct PathTrace {
    int path_index;                                      // Zero-based simulated path index.
    std::map<std::string, double> factor_shocks;         // Realized shock by factor id.
    std::map<std::string, std::pair<double, double>> quote_before_after; // Quote levels before and after shock.
    double portfolio_value_before;                       // Baseline portfolio value.
    double portfolio_value_after;                        // Shocked or aged portfolio value.
    double path_pnl;                                     // Total PnL for this path.

    // Aged mode extensions
    double portfolio_value_frozen_aged = 0.0;            // Aged value with market factors frozen.
    double aging_pnl = 0.0;                              // PnL from aging only.
    double market_pnl = 0.0;                             // PnL from market shocks only.
    double total_pnl = 0.0;                              // Aging plus market PnL.
    std::string valuation_date_before;                   // Baseline valuation date.
    std::string valuation_date_after;                    // Horizon valuation date.

    // Diagnostics per path
    int num_priced = 0;                                  // Trades priced successfully on this path.
    int num_expired = 0;                                 // Trades expired before horizon revaluation.
    int num_failed = 0;                                  // Trades that failed construction or pricing.
    int num_unsupported = 0;                             // Trades skipped because support is not implemented.
};

/**
 * @brief Aggregated Monte Carlo output including VaR, ES, paths, and diagnostics.
 */
struct SimulationResult {
    std::vector<double> portfolio_values;                // Simulated portfolio values by path.
    std::vector<double> portfolio_pnls;                  // Simulated PnL values by path.
    double base_portfolio_value = 0.0;                   // Baseline portfolio value before shocks.
    double var_95 = 0.0;                                 // 95% value-at-risk.
    double expected_shortfall_95 = 0.0;                  // 95% expected shortfall.
    std::uint64_t seed = 0;                              // Seed used for the run.
    double horizon_days = 1.0;                           // Risk horizon in calendar days.
    MonteCarloMode mode = MonteCarloMode::HorizonShockOnly; // Revaluation mode used.
    std::vector<std::string> factor_ids;                 // Factor order used by the covariance matrix.
    std::vector<PathTrace> traces;                       // Optional per-path diagnostics.

    // Diagnostics for the simulation baseline
    int num_trades_total = 0;                            // Total trades in the input portfolio.
    int num_trades_priced_t0 = 0;                        // Trades priced at baseline.
    int num_trades_failed_t0 = 0;                        // Trades that failed at baseline.
    int num_trades_priced_tH = 0;                        // Trades priced at horizon.
    int num_trades_expired_tH = 0;                       // Trades expired by horizon.
    int num_trades_failed_tH = 0;                        // Trades that failed at horizon.
    int num_trades_unsupported_tH = 0;                   // Trades unsupported at horizon.

    // Failure log (trade_id -> error message)
    std::map<std::string, std::string> construction_errors; // Trade construction/pricing errors by trade id.
};

/**
 * @brief Runs factor-shock Monte Carlo simulations with valuation-based PnL.
 */
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
