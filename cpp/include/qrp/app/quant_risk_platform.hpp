#pragma once

#include <memory>
#include <qrp/persistence/storage_backend.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/analytics/risk_service.hpp>

namespace qrp::app {

/**
 * @brief Application façade for the Quant Risk Platform.
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
    explicit QuantRiskPlatform(std::shared_ptr<persistence::StorageBackend> storage);

    void initialize();

    // Import methods
    void import_market_snapshot(const std::string& json_path);
    void import_portfolio(const std::string& json_path);
    void import_scenario_set(const std::string& json_path);

    // Run methods
    std::string run_valuation(const std::string& portfolio_id, const std::string& snapshot_id);
    std::string run_risk(const std::string& portfolio_id, const std::string& snapshot_id);
    std::string run_historical_var(const std::string& portfolio_id, const std::string& snapshot_id, const std::string& scenario_set_id);

    // Analysis and Reporting
    void get_run_report(const std::string& run_id);
    void compare_runs(const std::string& run_id_1, const std::string& run_id_2);
    void list_data();

private:
    std::shared_ptr<persistence::StorageBackend> storage_;
};

} // namespace qrp::app
