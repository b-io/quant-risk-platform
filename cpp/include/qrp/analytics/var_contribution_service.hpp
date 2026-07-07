#pragma once

// Declares historical VaR/ES contribution analytics over scenario PnL paths.

#include <qrp/domain/factors.hpp>
#include <qrp/domain/portfolio.hpp>

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace qrp::analytics {

/**
 * @brief Scenario-level PnL path used by VaR contribution analytics.
 */
struct HistoricalScenarioPnl {
    std::string scenario_name;                   // Scenario or path identifier.
    double portfolio_pnl = 0.0;                  // Total portfolio PnL under the scenario.
    std::map<std::string, double> trade_pnls;    // Trade id to scenario PnL.
    std::map<std::string, double> factor_shocks; // Risk factor id to scenario shock.
};

/**
 * @brief One VaR/ES contribution row for a reporting aggregation.
 */
struct VarContribution {
    std::string aggregation_type;                    // trade, book, strategy, currency, asset_class, risk_factor.
    std::string aggregation_key;                     // Group key within the aggregation type.
    double var_contribution = 0.0;                   // Component VaR in positive-loss convention.
    double expected_shortfall_contribution = 0.0;    // Component ES in positive-loss convention.
    double portfolio_var_share = 0.0;                // Component VaR divided by aggregate portfolio VaR.
    double portfolio_expected_shortfall_share = 0.0; // Component ES divided by aggregate portfolio ES.
    double standalone_var = 0.0;                     // VaR of the group's own PnL distribution.
    double standalone_expected_shortfall = 0.0;      // ES of the group's own PnL distribution.
    double incremental_var = 0.0;                    // Portfolio VaR minus VaR with group removed.
    double incremental_expected_shortfall = 0.0;     // Portfolio ES minus ES with group removed.
    double marginal_var = 0.0;                       // Remove-group marginal VaR approximation.
    double marginal_expected_shortfall = 0.0;        // Remove-group marginal ES approximation.
    double confidence_level = 0.95;                  // VaR/ES confidence level.
    int scenario_count = 0;                          // Number of scenarios in the distribution.
    int tail_scenario_count = 0;                     // Number of scenarios in the ES tail.
    std::string var_scenario_name;                   // Scenario selected by portfolio VaR.
    std::string sign_convention;                     // Contribution sign convention.
    std::string aggregation_rule;                    // Grouping and calculation rule.
    std::string calculation_method;                  // Historical component/standalone/incremental method.
};

/**
 * @brief Historical VaR/ES contribution report.
 */
struct VarContributionReport {
    std::string method = "HISTORICAL";                                    // VaR methodology label.
    double confidence_level = 0.95;                                       // VaR/ES confidence level.
    double var_value = 0.0;                                               // Aggregate portfolio VaR.
    double expected_shortfall = 0.0;                                      // Aggregate portfolio ES.
    int scenario_count = 0;                                               // Number of scenarios.
    int tail_scenario_count = 0;                                          // Number of ES tail scenarios.
    std::string var_scenario_name;                                        // Scenario selected by portfolio VaR.
    std::vector<HistoricalScenarioPnl> scenario_pnls;                     // Original scenario PnL paths.
    std::vector<VarContribution> contributions;                           // Contribution rows.
    std::map<std::string, double> var_component_residuals;                // Residual by aggregation type.
    std::map<std::string, double> expected_shortfall_component_residuals; // ES residual by aggregation type.
    std::map<std::string, std::string> diagnostics;                       // Calculation metadata.
};

/**
 * @brief Calculates contribution analytics from historical scenario PnL paths.
 */
class VarContributionService {
public:
    /**
     * @brief Builds historical VaR/ES totals and contribution rows.
     */
    static VarContributionReport
    calculate_historical_contributions(const domain::Portfolio& portfolio,
                                       const std::vector<HistoricalScenarioPnl>& scenario_pnls,
                                       double confidence_level = 0.95);

    /**
     * @brief Returns largest absolute component VaR contributors for one aggregation type.
     */
    static std::vector<VarContribution> top_var_contributors(const VarContributionReport& report,
                                                             const std::string& aggregation_type = "trade",
                                                             std::size_t limit = 10);

    /**
     * @brief Returns largest absolute component ES contributors for one aggregation type.
     */
    static std::vector<VarContribution>
    top_expected_shortfall_contributors(const VarContributionReport& report,
                                        const std::string& aggregation_type = "trade",
                                        std::size_t limit = 10);
};

} // namespace qrp::analytics
