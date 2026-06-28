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
    std::string component_id;
    PnlExplainComponentType component_type = PnlExplainComponentType::Residual;
    std::string label;
    double amount = 0.0;
    std::string factor_id;
    std::string model_name;
    domain::SupportStatus support_status = domain::SupportStatus::Supported;
    std::string status_message;
    std::map<std::string, std::string> tags;
};

/**
 * @brief Full PnL explain result for one trade across two market snapshots.
 */
struct PnlExplainResult {
    std::string trade_id;
    double prev_npv = 0.0;
    double curr_npv = 0.0;
    double total_pnl = 0.0;

    double carry_pnl = 0.0;
    double market_move_pnl = 0.0;
    double cash_pnl = 0.0;
    double residual = 0.0;

    bool prev_valuation_available = false;
    bool curr_valuation_available = false;
    bool rolled_valuation_available = false;
    ValuationResult prev_valuation;
    ValuationResult curr_valuation;
    ValuationResult rolled_valuation;
    CashflowExtractionResult cashflow_extraction;
    std::vector<PnlExplainComponent> components;
    std::map<std::string, std::string> diagnostics;
};

/**
 * @brief Builds PnL explain by comparing previous, rolled, and current valuations.
 */
class PnlExplainService {
public:
    /**
     * @brief Explains trade PnL between two market snapshots for a portfolio.
     */
    static std::vector<PnlExplainResult> explain_pnl(
        const domain::Portfolio& portfolio,
        const domain::MarketSnapshot& prev_market_dto,
        const domain::MarketSnapshot& curr_market_dto);
};

} // namespace qrp::analytics
