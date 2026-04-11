#pragma once
#include <qrp/domain/market_data.hpp>
#include <qrp/domain/factors.hpp>
#include <qrp/market/scenario_engine.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include <cmath>

namespace qrp::market {

/**
 * @brief Resolves generic factor shocks into specific quote-level shocks.
 * 
 * This service implements the production mapping layer between modeled risk factors 
 * and raw market quotes using FactorBindings.
 */
class FactorShockResolver {
public:
    /**
     * @brief Maps factor shocks to quote-level incremental values.
     * 
     * @param scenario The input scenario containing factor shocks.
     * @param factors The definitions of active factors.
     * @param bindings The mapping rules from factors to quotes.
     * @param base_market The base market snapshot for relative transformations.
     * @return A map from quote_id to the shocked absolute value.
     */
    static std::unordered_map<std::string, double> resolve_quote_values(
        const ScenarioDefinition& scenario,
        [[maybe_unused]] const std::vector<domain::FactorDefinition>& factors,
        const std::vector<domain::FactorBinding>& bindings,
        const domain::MarketSnapshot& base_market) {
        
        std::unordered_map<std::string, double> base_quotes;
        for (const auto& q : base_market.quotes) {
            base_quotes[q.id] = q.value;
        }

        // 1. Initialize with base values
        std::unordered_map<std::string, double> shocked_values = base_quotes;
        
        // 2. Map factor shocks to quote increments
        // We use an incremental approach to support multiple factors affecting one quote.
        std::unordered_map<std::string, double> quote_increments;

        for (const auto& [factor_id, shock] : scenario.factor_shocks) {
            // Find bindings for this factor
            for (const auto& binding : bindings) {
                if (binding.factor_id != factor_id) continue;
                
                if (!base_quotes.contains(binding.quote_id)) continue;
                
                double q = base_quotes.at(binding.quote_id);
                double increment = 0.0;

                // Apply transformation based on ShockMeasure
                switch (binding.shock_measure) {
                    case domain::ShockMeasure::Absolute:
                        increment = shock;
                        break;
                    case domain::ShockMeasure::Relative:
                        increment = q * shock;
                        break;
                    case domain::ShockMeasure::LogReturn:
                        // q_new = q * exp(s) => increment = q * (exp(s) - 1)
                        increment = q * (std::exp(shock) - 1.0);
                        break;
                    case domain::ShockMeasure::BasisPoints:
                        increment = shock / 10000.0;
                        break;
                    case domain::ShockMeasure::VolPoints:
                        increment = shock / 100.0;
                        break;
                }

                quote_increments[binding.quote_id] += binding.weight * increment;
            }
        }

        // 3. Apply increments to shocked_values
        for (const auto& [qid, inc] : quote_increments) {
            shocked_values[qid] += inc;
        }

        return shocked_values;
    }
};

} // namespace qrp::market
