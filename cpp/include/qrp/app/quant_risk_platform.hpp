#pragma once

// Declares the high-level application facade that coordinates storage, analytics, and reporting workflows.

#include <qrp/analytics/risk_service.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/persistence/storage_backend.hpp>

#include <memory>

namespace qrp::app {

/**
 * @brief Application facade for the Quant Risk Platform.
 *
 * Responsibilities:
 * - Load config and resolve storage backend.
 * - Import market data and portfolios.
 * - Build pricing contexts from stored snapshots.
 * - Execute analytics (valuation, risk, etc.) and persist results.
 * - Expose stable entry points for CLI and Python.
 */
class QuantRiskPlatform {
public:
    /**
     * @brief Creates the platform facade over an injected storage backend.
     */
    explicit QuantRiskPlatform(std::shared_ptr<persistence::StorageBackend> storage);

    /**
     * @brief Initializes persistent schema and shared platform infrastructure.
     */
    void initialize();

    // Import workflows
    /**
     * @brief Imports a market snapshot JSON file into persistent storage.
     */
    void import_market_snapshot(const std::string& json_path);

    /**
     * @brief Imports a portfolio JSON file into persistent storage.
     */
    void import_portfolio(const std::string& json_path);

    /**
     * @brief Imports a scenario set, factor definitions, and factor bindings.
     */
    void import_scenario_set(const std::string& json_path);

    // Analytics workflows
    /**
     * @brief Runs valuation for a stored portfolio and market snapshot.
     */
    std::string run_valuation(const std::string& portfolio_id, const std::string& snapshot_id);

    /**
     * @brief Runs deterministic risk for a stored portfolio and market snapshot.
     */
    std::string run_risk(const std::string& portfolio_id, const std::string& snapshot_id);

    /**
     * @brief Runs historical VaR over a stored scenario set.
     */
    std::string run_historical_var(const std::string& portfolio_id, const std::string& snapshot_id, const std::string& scenario_set_id);

    // Reporting workflows
    /**
     * @brief Prints persisted valuation results for a run.
     */
    void get_run_report(const std::string& run_id);

    /**
     * @brief Prints a comparison of two persisted analysis runs.
     */
    void compare_runs(const std::string& run_id_1, const std::string& run_id_2);

    /**
     * @brief Prints stored portfolios, snapshots, and analysis runs.
     */
    void list_data();

private:
    /** @brief Storage backend used for all persisted inputs and analytics outputs. */
    std::shared_ptr<persistence::StorageBackend> storage_;
};

} // namespace qrp::app
