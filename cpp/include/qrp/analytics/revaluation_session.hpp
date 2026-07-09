#pragma once

// Declares a stateful C++ revaluation session for reusable market and instrument caches.

#include <qrp/analytics/pricing_profiles.hpp>
#include <qrp/domain/factors.hpp>
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/portfolio.hpp>
#include <qrp/market/scenario_engine.hpp>

#include <cstddef>
#include <map>
#include <memory>
#include <ql/instrument.hpp>
#include <set>
#include <string>
#include <vector>

namespace qrp::analytics {

/**
 * @brief Quote-level movement observed during a scenario revaluation.
 */
struct QuoteMove {
    std::string quote_id;  // Market quote identifier.
    double before = 0.0;   // Base quote value before the scenario.
    double after = 0.0;    // Quote value after the scenario is applied.
    double restored = 0.0; // Quote value after resetting the session.
};

/**
 * @brief Scenario revaluation summary produced by RevaluationSession.
 */
struct RevaluationReport {
    double base_total_npv = 0.0;              // Total portfolio NPV before the scenario.
    std::vector<QuoteMove> quote_moves;       // Quote handles updated by the scenario.
    double restored_total_npv = 0.0;          // Total portfolio NPV after reset.
    std::string scenario_name;                // Scenario name.
    double scenario_pnl = 0.0;                // Shocked total NPV minus base total NPV.
    double shocked_total_npv = 0.0;           // Total portfolio NPV after the scenario.
    std::map<std::string, double> trade_pnls; // Trade-level shocked-minus-base P&L.
};

/**
 * @brief One structural link between a market quote and a cached trade.
 */
struct RevaluationDependency {
    std::string asset_class;             // Trade asset class label.
    std::string curve_id;                // Curve id when the quote feeds a curve dependency.
    std::string dependency_type;         // direct_quote, discount_curve, forecast_curve, or market_quote_match.
    std::vector<std::string> factor_ids; // Risk factors mapped to this quote when known.
    std::string product_type;            // Trade product type label.
    std::string quote_id;                // Market quote identifier.
    std::string trade_id;                // Trade identifier.
};

/**
 * @brief Read-only structural quote-to-trade dependency graph for a revaluation session.
 */
struct RevaluationDependencyGraph {
    std::vector<RevaluationDependency> dependencies; // All structural quote-to-trade links.
    std::size_t dependency_count = 0;                // Number of dependency edges.
    std::size_t quote_count = 0;                     // Number of quote ids with at least one dependency.
    std::vector<std::string> quote_ids;              // Quote ids with at least one dependency.
    std::size_t trade_count = 0;                     // Number of trade ids with at least one dependency.
    std::vector<std::string> trade_ids;              // Trade ids with at least one dependency.
};

/**
 * @brief Cheap structural preview of trades that may move when quotes are updated.
 */
struct RevaluationImpactPreview {
    std::vector<RevaluationDependency> dependencies;         // Quote-to-trade links hit by the update.
    std::size_t potentially_affected_trade_count = 0;        // Number of unique candidate trades.
    std::vector<std::string> potentially_affected_trade_ids; // Unique candidate trade ids.
    std::vector<std::string> updated_quote_ids;              // Updated quote ids included in the preview.
};

/**
 * @brief Candidate-trade before/after valuation diff.
 */
struct TradeRevaluationDiff {
    std::string asset_class;                       // Trade asset class label.
    double base_npv = 0.0;                         // Candidate trade NPV before the update.
    std::vector<std::string> dependency_quote_ids; // Updated quote ids that structurally hit this trade.
    bool moved_above_tolerance = false;            // Whether absolute P&L exceeded the requested tolerance.
    double pnl = 0.0;                              // Shocked NPV minus base NPV.
    std::string product_type;                      // Trade product type label.
    double shocked_npv = 0.0;                      // Candidate trade NPV after the update.
    std::string trade_id;                          // Trade identifier.
};

/**
 * @brief Opt-in diagnostic report that reprices only structurally affected candidate trades.
 */
struct RevaluationImpactReport {
    double candidate_base_total_npv = 0.0;                   // Sum of base NPVs for candidate trades.
    double candidate_pnl = 0.0;                              // Candidate shocked total minus base total.
    double candidate_restored_total_npv = 0.0;               // Candidate total after resetting the session.
    double candidate_shocked_total_npv = 0.0;                // Sum of shocked NPVs for candidate trades.
    std::vector<RevaluationDependency> dependencies;         // Structural links that triggered candidate repricing.
    double pnl_tolerance = 0.0;                              // Tolerance used by moved_above_tolerance.
    std::size_t potentially_affected_trade_count = 0;        // Number of unique candidate trades.
    std::vector<std::string> potentially_affected_trade_ids; // Unique candidate trade ids.
    std::vector<QuoteMove> quote_moves;                      // Updated quote before/after/restored values.
    std::string scenario_name;                               // Scenario name when report came from a scenario.
    std::vector<TradeRevaluationDiff> trade_diffs;           // Candidate-trade before/after diffs.
    std::vector<std::string> updated_quote_ids;              // Updated quote ids included in the report.
};

/**
 * @brief Reuses one built market state and one instrument cache across quote updates and scenarios.
 *
 * The session owns the mutable market state in C++. Python callers can apply quote updates
 * or factor scenarios through domain-level methods, but raw QuantLib handles are not exposed.
 */
class RevaluationSession {
public:
    /**
     * @brief Builds a reusable revaluation session from portfolio, market, factors, and bindings.
     */
    RevaluationSession(domain::Portfolio portfolio,
                       domain::MarketSnapshot base_market,
                       std::vector<domain::FactorDefinition> factors = {},
                       std::vector<domain::FactorBinding> bindings = {});

    RevaluationSession(const RevaluationSession&) = delete;
    RevaluationSession& operator=(const RevaluationSession&) = delete;
    RevaluationSession(RevaluationSession&&) = delete;
    RevaluationSession& operator=(RevaluationSession&&) = delete;

    /**
     * @brief Applies absolute quote updates to the current session state.
     */
    void apply_quote_updates(const std::map<std::string, double>& quote_updates);

    /**
     * @brief Applies a factor scenario to the current session state.
     */
    void apply_scenario(const market::ScenarioDefinition& scenario);

    /**
     * @brief Prices the portfolio from the current session state.
     */
    std::vector<ValuationResult> price() const;

    /**
     * @brief Resets all session quote handles to the base market snapshot.
     */
    void reset();

    /**
     * @brief Revalues one scenario from the base state and restores the session.
     */
    RevaluationReport revalue_scenario(const market::ScenarioDefinition& scenario);

    /**
     * @brief Returns the structurally affected trades for absolute quote updates without repricing.
     */
    RevaluationImpactPreview preview_quote_update_impact(const std::map<std::string, double>& quote_updates) const;

    /**
     * @brief Resolves a scenario to quote updates and returns the structurally affected trades.
     */
    RevaluationImpactPreview preview_scenario_impact(const market::ScenarioDefinition& scenario) const;

    /**
     * @brief Reprices only structurally affected candidate trades for quote updates and restores the session.
     */
    RevaluationImpactReport revalue_quote_update_impact(const std::map<std::string, double>& quote_updates,
                                                        double pnl_tolerance = 1.0e-8);

    /**
     * @brief Reprices only structurally affected candidate trades for a scenario and restores the session.
     */
    RevaluationImpactReport revalue_scenario_impact(const market::ScenarioDefinition& scenario,
                                                    double pnl_tolerance = 1.0e-8);

    /**
     * @brief Returns the full read-only quote-to-trade dependency graph.
     */
    RevaluationDependencyGraph dependency_graph() const;

    /**
     * @brief Returns read-only dependency edges for one quote id.
     */
    std::vector<RevaluationDependency> dependencies_for_quote(const std::string& quote_id) const;

    /**
     * @brief Returns read-only dependency edges for one trade id.
     */
    std::vector<RevaluationDependency> dependencies_for_trade(const std::string& trade_id) const;

    /**
     * @brief Returns current trade NPVs keyed by trade id.
     */
    std::map<std::string, double> trade_values() const;

    /**
     * @brief Returns the current total portfolio NPV.
     */
    double total_npv() const;

private:
    struct CachedInstrument {
        const domain::Trade* trade = nullptr;
        QuantLib::ext::shared_ptr<QuantLib::Instrument> instrument;
    };

    void build_session();
    void build_dependency_index() const;
    void ensure_quote_exists(const std::string& quote_id) const;
    std::vector<QuoteMove> quote_moves_for(const std::map<std::string, double>& before,
                                           const std::map<std::string, double>& after) const;
    std::map<std::string, double> quote_values_for(const std::vector<std::string>& quote_ids) const;
    std::map<std::string, double> trade_values_for(const std::set<std::string>& trade_ids) const;

    domain::MarketSnapshot base_market_;
    std::vector<CachedInstrument> instruments_;
    std::vector<domain::FactorBinding> bindings_;
    mutable bool dependency_index_built_ = false;
    mutable std::map<std::string, std::vector<RevaluationDependency>> dependency_index_by_quote_;
    std::vector<domain::FactorDefinition> factors_;
    domain::Portfolio portfolio_;
    std::shared_ptr<market::MarketState> state_;
};

} // namespace qrp::analytics
