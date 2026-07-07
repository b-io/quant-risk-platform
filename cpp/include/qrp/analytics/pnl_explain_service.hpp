#pragma once

// Declares PnL explain APIs that reconcile trade PnL into cash, aging, factor, and residual components.

#include <qrp/analytics/valuation_service.hpp>
#include <qrp/domain/factors.hpp>
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
    FxTranslation,
    MarketMove,
    ModelChange,
    RollDown,
    TradeActivity,
    Residual
};

/**
 * @brief One named PnL component with factor/model diagnostics.
 */
struct PnlExplainComponent {
    int sequence = 0;         // Component order in the explain ladder.
    std::string component_id; // Stable component identifier.
    PnlExplainComponentType component_type = PnlExplainComponentType::Residual; // Business component bucket.
    std::string label;                                                          // Human-readable component label.
    double amount = 0.0;                                                        // Component PnL amount.
    std::string factor_id;         // Market factor driving the component, when available.
    std::string risk_factor_group; // Broad factor group used for residual and attribution reporting.
    std::string model_name;        // Model or approximation used.
    domain::SupportStatus support_status = domain::SupportStatus::Supported; // Component support status.
    std::string status_message;                                              // Diagnostic message.
    std::map<std::string, std::string> tags;                                 // Extra component metadata.
};

/**
 * @brief Full PnL explain result for one trade across two market snapshots.
 */
struct PnlExplainResult {
    std::string asset_class;  // Trade asset class label.
    std::string book;         // Owning book.
    std::string currency;     // Trade valuation currency.
    std::string product_type; // Product taxonomy label.
    std::string strategy;     // Strategy or mandate label.
    std::string trade_id;     // Explained trade id.
    double prev_npv = 0.0;    // Previous snapshot NPV.
    double curr_npv = 0.0;    // Current snapshot NPV.
    double total_pnl = 0.0;   // Current minus previous NPV plus realized cash.

    double carry_pnl = 0.0;          // Aging or carry contribution.
    double cash_pnl = 0.0;           // Realized cashflow contribution.
    double fx_translation_pnl = 0.0; // Reporting-currency translation contribution.
    double market_move_pnl = 0.0;    // Market-factor move contribution.
    double model_change_pnl = 0.0;   // Model or configuration change contribution.
    double residual = 0.0;           // Deprecated alias for residual_pnl.
    double residual_pnl = 0.0;       // Unexplained residual.
    double roll_down_pnl = 0.0;      // Roll-down contribution when separated from carry.
    double trade_activity_pnl = 0.0; // Trade activity contribution for new, amended, or closed trades.

    double explained_pnl = 0.0;             // Sum of explicit non-residual components.
    double reconciliation_difference = 0.0; // total_pnl minus explicit components and residual.
    bool reconciliation_passed = false;     // Whether the explain result reconciles within tolerance.

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
                                                     const domain::MarketSnapshot& curr_market_dto,
                                                     const std::vector<domain::FactorDefinition>& factors = {},
                                                     const std::vector<domain::FactorBinding>& bindings = {});
};

} // namespace qrp::analytics
