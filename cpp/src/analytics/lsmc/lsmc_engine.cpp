// Implements backward induction for least-squares Monte Carlo exercise decisions.

#include <qrp/analytics/lsmc/lsmc_engine.hpp>
#include <algorithm>
#include <cmath>
#include <limits>

namespace qrp::analytics::lsmc {

LsmcResult LsmcEngine::run(
    const simulation::TimeGrid& time_grid,
    const simulation::StochasticProcess& process,
    const dynamic_programming::DecisionProblem& problem,
    const dynamic_programming::State& initial_state
) const {
    std::mt19937 gen(config_.seed);
    std::size_t N = config_.num_paths;
    std::size_t T = time_grid.size();

    // 1. Simulate market paths
    std::vector<simulation::MarketPath> market_paths(N, simulation::MarketPath(T, process.dimension()));
    for (std::size_t i = 0; i < N; ++i) {
        process.simulatePath(time_grid, gen, market_paths[i]);
    }

    // 2. Backward induction over simulated market paths.
    // The decision problem owns the mapping from market variables to problem-specific state features.
    std::vector<double> path_values(N);
    
    // Terminal value
    for (std::size_t i = 0; i < N; ++i) {
        dynamic_programming::State final_state = initial_state;
        final_state.market_variables = market_paths[i].at(T - 1);
        path_values[i] = problem.terminalValue(final_state);
    }

    regression::OrdinaryLeastSquares ols;

    for (int t = static_cast<int>(T) - 2; t >= 0; --t) {
        double dt = time_grid.dt(t + 1);
        double df = std::exp(-config_.discount_rate * dt);

        // Continuation values from t+1, discounted to t
        Eigen::VectorXd Y(N);
        for (std::size_t i = 0; i < N; ++i) {
            Y(i) = path_values[i] * df;
        }

        // Regression features at time t
        std::vector<std::vector<double>> features_list;
        std::vector<std::size_t> in_the_money_indices;
        
        for (std::size_t i = 0; i < N; ++i) {
            dynamic_programming::State s = initial_state;
            s.market_variables = market_paths[i].at(t);
            auto f = problem.regressionFeatures(s, t);
            features_list.push_back(f);
            in_the_money_indices.push_back(i); 
        }

        if (!in_the_money_indices.empty()) {
            std::size_t num_f = features_list[0].size();
            Eigen::MatrixXd X(in_the_money_indices.size(), num_f);
            Eigen::VectorXd y_sub(in_the_money_indices.size());
            
            for (std::size_t j = 0; j < in_the_money_indices.size(); ++j) {
                std::size_t path_idx = in_the_money_indices[j];
                for (std::size_t k = 0; k < num_f; ++k) {
                    X(j, k) = features_list[path_idx][k];
                }
                y_sub(j) = Y(path_idx);
            }

            auto reg_result = ols.fit(X, y_sub);
            Eigen::VectorXd continuation_values = ols.predict(X, reg_result.coefficients);

            for (std::size_t j = 0; j < in_the_money_indices.size(); ++j) {
                std::size_t i = in_the_money_indices[j];
                dynamic_programming::State s = initial_state;
                s.market_variables = market_paths[i].at(t);
                
                auto actions = problem.feasibleActions(s, t);
                if (actions.empty()) {
                    path_values[i] = Y(i);
                    continue;
                }

                double best_val = -std::numeric_limits<double>::infinity();
                
                for (const auto& action : actions) {
                    double immediate = problem.immediateCashflow(s, action, t);
                    double continuation = problem.isTerminalAction(s, action, t) ? 0.0 : continuation_values(j);
                    double val = immediate + continuation;
                    if (val > best_val) {
                        best_val = val;
                    }
                }

                path_values[i] = best_val;
            }
        }
    }

    double sum = 0;
    double sum2 = 0;
    for (double v : path_values) {
        sum += v;
        sum2 += v * v;
    }
    
    double mean = sum / N;
    double variance = (sum2 / N) - (mean * mean);
    
    std::sort(path_values.begin(), path_values.end());
    double var_95 = path_values[static_cast<std::size_t>(0.05 * N)];
    
    double es_sum = 0;
    std::size_t es_count = 0;
    for (std::size_t i = 0; i < static_cast<std::size_t>(0.05 * N); ++i) {
        es_sum += path_values[i];
        es_count++;
    }
    double es_95 = (es_count > 0) ? es_sum / es_count : var_95;

    LsmcResult result;
    result.value = mean; 
    result.standard_error = std::sqrt(variance / N);
    result.path_values = path_values;
    result.var_95 = var_95;
    result.expected_shortfall_95 = es_95;
    
    return result;
}

} // namespace qrp::analytics::lsmc
