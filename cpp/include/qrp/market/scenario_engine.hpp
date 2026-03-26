#pragma once
#include <qrp/domain/market_data.hpp>
#include <qrp/market/market_state.hpp>
#include <map>
#include <string>

namespace qrp::market {

class MarketState; // Forward declaration

struct ScenarioDefinition {
    std::string name;
    std::map<std::string, double> parallel_shocks; // currency -> bump
    std::map<std::string, std::map<std::string, double>> node_shocks; // currency -> {tenor -> bump}
    
    // Twist shocks: shift = base_shift + slope * (tenor_in_years - pivot_tenor)
    struct TwistShock {
        double pivot_tenor; // e.g., 5.0 (years)
        double slope;       // e.g., 0.0001 (1bp per year)
    };
    std::map<std::string, TwistShock> twist_shocks;

    // Credit spread shocks
    std::map<std::string, double> credit_shocks; // issuer/group -> bump
};

class ScenarioEngine {
public:
    static domain::MarketSnapshot apply_scenario(
        const domain::MarketSnapshot& base_market,
        const ScenarioDefinition& scenario);

    /**
     * @brief Apply a scenario directly to a built MarketState using handles.
     * This is much more efficient than apply_scenario(DTO) as it doesn't rebuild curves.
     */
    static void apply_scenario_to_state(
        MarketState& state,
        const domain::MarketSnapshot& base_dto,
        const ScenarioDefinition& scenario);
};

} // namespace qrp::market
