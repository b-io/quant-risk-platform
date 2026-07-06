#pragma once

// Declares PnL explain APIs that separate base value, aging, market move, and residual components.

#include <qrp/analytics/valuation_service.hpp>
#include <qrp/domain/market_data.hpp>

#include <map>
#include <string>
#include <vector>

namespace qrp::analytics {

/**
 * @brief Business component categories used in trade-level PnL explain.
 */
enum class PnlExplainComponentType {
    Carry,
    Cash,
    MarketMove,
    Residual
};

/**
 * @brief One named PnL component with factor/model diagnostics.
 */
struct PnlExplainComponent {
    std::string component_id;                                                   // Stable component identifier.
    PnlExplainComponentType component_type = PnlExplainComponentType::Residual; // Business component bucket.
    std::string label;                                                          // Human-readable component label.
    double amount = 0.0;                                                        // Component PnL amount.
    std::string factor_id;  // Market factor driving the component, when available.
    std::string model_name; // Model or approximation used.
    domain::SupportStatus support_status = domain::SupportStatus::Supported; // Component support status.
    std::string status_message;                                              // Diagnostic message.
    std::map<std::string, std::string> tags;                                 // Extra component metadata.
};

/**
 * @brief Full PnL explain result for one trade across two market snapshots.
 */
struct PnlExplainResult {
    std::string trade_id;   // Explained trade id.
    double prev_npv = 0.0;  // Previous snapshot NPV.
    double curr_npv = 0.0;  // Current snapshot NPV.
    double total_pnl = 0.0; // Current minus previous NPV plus realized cash handling.

    double carry_pnl = 0.0;       // Aging or carry contribution.
    double market_move_pnl = 0.0; // Market-factor move contribution.
    double cash_pnl = 0.0;        // Realized cashflow contribution.
    double residual = 0.0;        // Unexplained residual.

    bool prev_valuation_available = false;          // Previous valuation availability flag.
    bool curr_valuation_available = false;          // Current valuation availability flag.
    bool rolled_valuation_available = false;        // Rolled valuation availability flag.
    ValuationResult prev_valuation;                 // Previous snapshot valuation detail.
    ValuationResult curr_valuation;                 // Current snapshot valuation detail.
    ValuationResult rolled_valuation;               // Previous trade aged to current date.
    CashflowExtractionResult cashflow_extraction;   // Cashflows realized between snapshots.
    std::vector<PnlExplainComponent> components;    // Explained PnL components.
    std::map<std::string, std::string> diagnostics; // Result-level diagnostics.
};

/**
 * @brief Builds PnL explain by comparing previous, rolled, and current valuations.
 */
class PnlExplainService {
public:
    /**
     * @brief Explains trade PnL between two market snapshots for a portfolio.
     */
    static std::vector<PnlExplainResult> explain_pnl(const domain::Portfolio& portfolio,
                                                     const domain::MarketSnapshot& prev_market_dto,
                                                     const domain::MarketSnapshot& curr_market_dto);
};

} // namespace qrp::analytics
