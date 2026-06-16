#include <qrp/analytics/lsmc/lsmc_engine.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>

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

    // 2. Build state paths
    // For simplicity, we assume state transitions are deterministic given market variables.
    // In many LSMC problems, we only need the current market variables and some simple operational state.
    // To handle general path-dependency, we would need to store the full state path.
    // However, if we work backwards, we might need a way to reach any state at time t.
    // For American options, state is just (price, time).
    // For storage, it's (price, inventory, time).
    
    // terminal_values[path]
    std::vector<double> path_values(N);
    std::vector<dynamic_programming::State> current_states(N);
    
    // To implement LSMC properly for complex assets, we often need "optimal exercise" logic.
    // If the state transition depends on the action, we can't easily pre-simulate the state path 
    // without knowing the policy.
    // But LSMC typically assumes we can estimate the continuation value V(state, t) 
    // from a sample of states at time t.
    
    // For now, let's implement the backward induction assuming a fixed state path (e.g. for American options).
    // Or better, handle the state as (market_variables, operational_variables).
    
    // Initial approach: assume market variables are simulated, and we find the optimal policy.
    // If action affects operational state, we have a problem with backward induction because we don't know 
    // which operational state we will be in at time t unless we simulate forward.
    // Solution: either discrete states for operational variables or including them in regression.
    
    // Let's assume for now that we can represent the value function V(market, operational, t).
    
    // Terminal value
    for (std::size_t i = 0; i < N; ++i) {
        dynamic_programming::State final_state;
        final_state.market_variables = market_paths[i].at(T - 1);
        // How to get final operational state? 
        // For American option, it doesn't matter much (either exercised or not).
        // For storage, it depends on all previous actions.
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
            dynamic_programming::State s;
            s.market_variables = market_paths[i].at(t);
            // This is where it gets tricky for storage. 
            // For now, let's assume features only depend on market variables at t.
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
                dynamic_programming::State s;
                s.market_variables = market_paths[i].at(t);
                
                auto actions = problem.feasibleActions(s, t);
                double best_val = -1e308;
                double chosen_immediate = 0;
                
                for (const auto& action : actions) {
                    double immediate = problem.immediateCashflow(s, action, t);
                    // This is a simplification: we assume continuation value is same for all actions.
                    // Actually, nextState depends on action.
                    // For American Option: Exercise -> next state is "Extinguished", Continue -> next state is "Active".
                    // V(state, t) = max(Immediate(action) + df * E[V(next_state, t+1)])
                    
                    // TODO: Improve this to handle next_state properly.
                    double val = immediate + continuation_values(j); 
                    if (val > best_val) {
                        best_val = val;
                        chosen_immediate = immediate;
                    }
                }
                
                // For American Option style:
                double immediate_exercise = problem.immediateCashflow(s, {1, "Exercise", {}}, t);
                if (immediate_exercise > continuation_values(j) && immediate_exercise > 0) {
                    path_values[i] = immediate_exercise;
                } else {
                    path_values[i] = Y(i);
                }
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
