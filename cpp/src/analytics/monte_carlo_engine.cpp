#include <qrp/analytics/monte_carlo_engine.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <ql/math/randomnumbers/boxmullergaussianrng.hpp>
#include <ql/math/randomnumbers/mt19937uniformrng.hpp>
#include <ql/math/matrix.hpp>
#include <ql/math/matrixutilities/choleskydecomposition.hpp>
#include <algorithm>
#include <numeric>

/*
Design note (see docs/design/ANALYTICS_SERVICES.md and docs/risk/MONTE_CARLO.md):
- We implement a one-step, factor-based Monte Carlo to generate coherent shocks across currencies.
- The factor covariance is represented by a (mocked) symmetric positive-definite matrix and sampled via
  Cholesky decomposition (L·Z). This produces correlated Gaussian shocks consistent with the covariance.
- We do not rebuild curves for each path; instead we bump quote handles and rely on QuantLib's Observer pattern
  to lazily reprice cached instruments. This drastically improves throughput.
*/
namespace qrp::analytics {

SimulationResult MonteCarloEngine::run_simulation(
    const domain::Portfolio& portfolio,
    const domain::MarketSnapshot& base_market_dto,
    int num_paths) {

    SimulationResult result;
    result.portfolio_values.reserve(num_paths);

    // 1. Setup Base Market and Instrument Cache
    market::MarketSnapshot base_market(base_market_dto);
    auto state = base_market.built_state();
    PricingContext context(state);

    std::vector<std::pair<std::string, std::shared_ptr<QuantLib::Instrument>>> instruments;
    double base_total_npv = 0.0;
    for (const auto& trade : portfolio.trades) {
        auto inst = instruments::InstrumentFactory::create_instrument(trade, context);
        if (inst) {
            instruments.push_back({trade.id, inst});
            base_total_npv += inst->NPV();
        }
    }

    // 2. Identify currencies to shock
    std::vector<std::string> currencies;
    for (const auto& t : portfolio.trades) {
        if (std::find(currencies.begin(), currencies.end(), t.currency) == currencies.end()) {
            currencies.push_back(t.currency);
        }
    }
    size_t num_factors = currencies.size();

    // 3. Setup Factor Model (Mocked Covariance for MVP)
    // In a real system, this would be calibrated from historical data
    QuantLib::Matrix cov(num_factors, num_factors, 0.0);
    for (size_t i = 0; i < num_factors; ++i) {
        cov[i][i] = 0.0001; // 100bp annual variance (10bp vol)
        for (size_t j = 0; j < i; ++j) {
            cov[i][j] = cov[j][i] = 0.00005; // 0.5 correlation
        }
    }
    QuantLib::Matrix L = QuantLib::CholeskyDecomposition(cov).L();

    // 4. Setup Random Number Generator
    QuantLib::MersenneTwisterUniformRng baseRng(42);
    QuantLib::BoxMullerGaussianRng<QuantLib::MersenneTwisterUniformRng> rng(baseRng);

    // 5. Run paths
    for (int i = 0; i < num_paths; ++i) {
        // Generate correlated shocks
        std::vector<double> z(num_factors);
        for (size_t j = 0; j < num_factors; ++j) z[j] = rng.next().value;
        
        std::vector<double> shocks(num_factors, 0.0);
        for (size_t row = 0; row < num_factors; ++row) {
            for (size_t col = 0; col <= row; ++col) {
                shocks[row] += L[row][col] * z[col];
            }
        }

        // Apply to state handles
        market::ScenarioDefinition path_scenario;
        for (size_t j = 0; j < num_factors; ++j) {
            path_scenario.parallel_shocks[currencies[j]] = shocks[j];
        }
        
        market::ScenarioEngine::apply_scenario_to_state(*state, base_market_dto, path_scenario);

        double total_npv = 0.0;
        for (auto& [id, inst] : instruments) {
            total_npv += inst->NPV();
        }
        result.portfolio_values.push_back(total_npv);
        
        // Reset state for next path
        for (const auto& q : base_market_dto.quotes) state->add_quote(q.id, q.value);
    }

    // 6. Calculate VaR and ES
    std::vector<double> pnls;
    for (double v : result.portfolio_values) pnls.push_back(v - base_total_npv);
    std::sort(pnls.begin(), pnls.end());

    int var_idx = static_cast<int>(num_paths * 0.05);
    if (var_idx < (int)pnls.size()) {
        result.var_95 = -pnls[var_idx];
        double es_sum = 0.0;
        for (int i = 0; i <= var_idx; ++i) es_sum += pnls[i];
        result.expected_shortfall_95 = -(es_sum / (var_idx + 1));
    }

    return result;
}

} // namespace qrp::analytics
