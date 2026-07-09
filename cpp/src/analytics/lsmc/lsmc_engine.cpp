// Implements backward induction for least-squares Monte Carlo exercise decisions.

#include <qrp/analytics/lsmc/lsmc_engine.hpp>
#include <qrp/analytics/simulation/gbm.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>

namespace qrp::analytics::lsmc {

namespace {

std::mt19937 make_generator(std::uint64_t seed) {
    std::seed_seq sequence{
        static_cast<std::uint32_t>(seed & 0xffffffffULL),
        static_cast<std::uint32_t>((seed >> 32U) & 0xffffffffULL),
    };
    return std::mt19937(sequence);
}

double path_count_as_double(std::size_t path_count) {
    return static_cast<double>(path_count);
}

std::vector<std::string> fallback_basis_names(std::size_t count) {
    std::vector<std::string> names;
    names.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        names.push_back("basis_" + std::to_string(i));
    }
    return names;
}

std::vector<double> make_uniform_time_grid(double maturity, std::size_t exercise_steps) {
    std::vector<double> times;
    times.reserve(exercise_steps + 1U);
    for (std::size_t i = 0; i <= exercise_steps; ++i) {
        times.push_back(maturity * static_cast<double>(i) / static_cast<double>(exercise_steps));
    }
    return times;
}

std::vector<double> make_request_exercise_times(double maturity, std::size_t exercise_steps) {
    if (exercise_steps == 0U) {
        return {0.0};
    }
    if (maturity <= 0.0) {
        return {0.0};
    }
    return make_uniform_time_grid(maturity, exercise_steps);
}

std::vector<std::string> resolve_basis_names(const dynamic_programming::DecisionProblem& problem,
                                             const dynamic_programming::State& state,
                                             std::size_t time_index) {
    auto features = problem.regressionFeatures(state, time_index);
    if (features.empty()) {
        throw std::invalid_argument("LsmcEngine requires at least one regression feature");
    }

    auto names = problem.regressionFeatureNames(time_index);
    if (names.empty() || names.size() != features.size()) {
        names = fallback_basis_names(features.size());
    }
    return names;
}

double vanilla_option_intrinsic(double spot, double strike, bool is_put) {
    return is_put ? std::max(strike - spot, 0.0) : std::max(spot - strike, 0.0);
}

LsmcResult deterministic_result(const AmericanOptionLsmcRequest& request, const std::string& status) {
    LsmcResult result;
    const double value = vanilla_option_intrinsic(request.spot, request.strike, request.is_put);
    result.value = value;
    result.standard_error = 0.0;
    result.path_values = {value};
    result.sorted_path_values = {value};
    result.var_95 = value;
    result.expected_shortfall_95 = value;
    result.exercise_times = make_request_exercise_times(request.maturity, request.exercise_steps);
    result.basis_function_names =
        exercise::VanillaOptionExercisePolicy(request.strike, request.is_put, request.basis_degree)
            .regressionFeatureNames(0);
    result.config_tags = {
        {"basis_degree", std::to_string(request.basis_degree)},
        {"discount_rate", std::to_string(request.config.discount_rate)},
        {"exercise_steps", std::to_string(request.exercise_steps)},
        {"maturity", std::to_string(request.maturity)},
        {"num_paths", std::to_string(request.config.num_paths)},
        {"seed", std::to_string(request.config.seed)},
        {"status", status},
    };
    return result;
}

} // namespace

LsmcResult LsmcEngine::run(const simulation::TimeGrid& time_grid,
                           const simulation::StochasticProcess& process,
                           const dynamic_programming::DecisionProblem& problem,
                           const dynamic_programming::State& initial_state) const {
    if (config_.num_paths == 0U) {
        throw std::invalid_argument("LsmcEngine requires at least one path");
    }
    if (time_grid.size() < 2U) {
        throw std::invalid_argument("LsmcEngine requires at least two time points");
    }
    if (process.dimension() == 0U) {
        throw std::invalid_argument("LsmcEngine requires a positive process dimension");
    }

    std::mt19937 gen = make_generator(config_.seed);
    const std::size_t N = config_.num_paths;
    const std::size_t T = time_grid.size();
    std::vector<std::string> basis_function_names;
    std::vector<LsmcRegressionDiagnostic> regression_diagnostics;

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

    for (int t = static_cast<int>(T) - 2; t >= 1; --t) {
        const auto time_index = static_cast<std::size_t>(t);
        double dt = time_grid.dt(time_index + 1U);
        double df = std::exp(-config_.discount_rate * dt);

        // Continuation values from t+1, discounted to t
        Eigen::VectorXd Y(N);
        for (std::size_t i = 0; i < N; ++i) {
            Y(i) = path_values[i] * df;
        }

        // Regression features at time t
        std::vector<std::vector<double>> features_list;
        std::vector<std::size_t> in_the_money_indices;
        features_list.reserve(N);
        in_the_money_indices.reserve(N);

        for (std::size_t i = 0; i < N; ++i) {
            dynamic_programming::State s = initial_state;
            s.market_variables = market_paths[i].at(time_index);
            auto f = problem.regressionFeatures(s, time_index);
            features_list.push_back(f);
            in_the_money_indices.push_back(i);
        }

        if (!in_the_money_indices.empty()) {
            const std::size_t num_f = features_list[0].size();
            if (num_f == 0U) {
                throw std::invalid_argument("LsmcEngine requires at least one regression feature");
            }
            if (basis_function_names.empty()) {
                basis_function_names = resolve_basis_names(problem, initial_state, time_index);
            }
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
            LsmcRegressionDiagnostic diagnostic;
            diagnostic.time_index = time_index;
            diagnostic.time = time_grid[time_index];
            diagnostic.sample_count = in_the_money_indices.size();
            diagnostic.basis_function_count = num_f;
            diagnostic.r_squared = reg_result.r_squared;
            diagnostic.residual_sum_of_squares = reg_result.residual_sum_of_squares;

            for (std::size_t j = 0; j < in_the_money_indices.size(); ++j) {
                std::size_t i = in_the_money_indices[j];
                dynamic_programming::State s = initial_state;
                s.market_variables = market_paths[i].at(time_index);

                auto actions = problem.feasibleActions(s, time_index);
                if (actions.empty()) {
                    path_values[i] = Y(i);
                    continue;
                }

                double best_val = -std::numeric_limits<double>::infinity();
                int best_action_id = 0;
                bool best_is_terminal = false;

                for (const auto& action : actions) {
                    double immediate = problem.immediateCashflow(s, action, time_index);
                    const bool terminal_action = problem.isTerminalAction(s, action, time_index);
                    double continuation = terminal_action ? 0.0 : continuation_values(j);
                    double val = immediate + continuation;
                    if (val > best_val) {
                        best_val = val;
                        best_action_id = action.id;
                        best_is_terminal = terminal_action;
                    }
                }

                if (best_action_id == 0) {
                    ++diagnostic.continuation_count;
                } else {
                    ++diagnostic.exercise_count;
                }
                if (best_is_terminal) {
                    ++diagnostic.terminal_exercise_count;
                }
                path_values[i] = best_val;
            }
            regression_diagnostics.push_back(diagnostic);
        }
    }

    if (basis_function_names.empty()) {
        basis_function_names = resolve_basis_names(problem, initial_state, 0U);
    }

    const double initial_df = std::exp(-config_.discount_rate * time_grid.dt(1U));
    std::vector<double> continuation_path_values;
    continuation_path_values.reserve(N);
    double continuation_sum = 0.0;
    for (double value : path_values) {
        const double discounted = value * initial_df;
        continuation_path_values.push_back(discounted);
        continuation_sum += discounted;
    }
    const double continuation_mean = continuation_sum / path_count_as_double(N);

    auto initial_actions = problem.feasibleActions(initial_state, 0U);
    if (!initial_actions.empty()) {
        double best_value = -std::numeric_limits<double>::infinity();
        double best_immediate = 0.0;
        bool best_is_terminal = false;
        for (const auto& action : initial_actions) {
            const double immediate = problem.immediateCashflow(initial_state, action, 0U);
            const bool terminal_action = problem.isTerminalAction(initial_state, action, 0U);
            const double value = immediate + (terminal_action ? 0.0 : continuation_mean);
            if (value > best_value) {
                best_value = value;
                best_immediate = immediate;
                best_is_terminal = terminal_action;
            }
        }

        if (best_is_terminal) {
            path_values.assign(N, best_value);
        } else {
            path_values = std::move(continuation_path_values);
            if (best_immediate != 0.0) {
                for (double& value : path_values) {
                    value += best_immediate;
                }
            }
        }
    } else {
        path_values = std::move(continuation_path_values);
    }

    double sum = 0;
    double sum2 = 0;
    for (double v : path_values) {
        sum += v;
        sum2 += v * v;
    }

    const double path_count = path_count_as_double(N);
    double mean = sum / path_count;
    double variance = (sum2 / path_count) - (mean * mean);

    std::vector<double> sorted_path_values = path_values;
    std::sort(sorted_path_values.begin(), sorted_path_values.end());
    const std::size_t var_index = std::min(N - 1, N / 20);
    double var_95 = sorted_path_values[var_index];

    double es_sum = 0;
    const std::size_t es_count = std::max<std::size_t>(1, var_index);
    for (std::size_t i = 0; i < es_count; ++i) {
        es_sum += sorted_path_values[i];
    }
    double es_95 = (es_count > 0) ? es_sum / static_cast<double>(es_count) : var_95;

    LsmcResult result;
    result.value = mean;
    result.standard_error = std::sqrt(std::max(variance, 0.0) / path_count);
    result.path_values = std::move(path_values);
    result.sorted_path_values = std::move(sorted_path_values);
    result.var_95 = var_95;
    result.expected_shortfall_95 = es_95;
    result.exercise_times = time_grid.times();
    result.basis_function_names = std::move(basis_function_names);
    result.regression_diagnostics = std::move(regression_diagnostics);
    result.config_tags = {{"discount_rate", std::to_string(config_.discount_rate)},
                          {"num_paths", std::to_string(config_.num_paths)},
                          {"seed", std::to_string(config_.seed)}};

    return result;
}

LsmcResult price_american_option(const AmericanOptionLsmcRequest& request) {
    if (request.maturity <= 0.0) {
        return deterministic_result(request, "expired_or_zero_maturity");
    }
    if (request.spot <= 0.0 || request.strike <= 0.0) {
        return deterministic_result(request, "invalid_spot_or_strike");
    }
    if (request.volatility <= 0.0 || request.exercise_steps == 0U || request.config.num_paths == 0U) {
        return deterministic_result(request, "deterministic_intrinsic_fallback");
    }

    const auto times = make_request_exercise_times(request.maturity, request.exercise_steps);
    const double drift = request.risk_free_rate - request.dividend_yield + request.borrow_rate;
    simulation::TimeGrid time_grid(times);
    simulation::GeometricBrownianMotion process(request.spot, drift, request.volatility);

    auto policy =
        std::make_shared<exercise::VanillaOptionExercisePolicy>(request.strike, request.is_put, request.basis_degree);
    exercise::ExercisePolicyDecisionProblem problem(policy, request.exercise_steps);

    LsmcEngine engine(request.config);
    auto result = engine.run(time_grid, process, problem, {{request.spot}, {}});
    result.config_tags["basis_degree"] = std::to_string(request.basis_degree);
    result.config_tags["borrow_rate"] = std::to_string(request.borrow_rate);
    result.config_tags["dividend_yield"] = std::to_string(request.dividend_yield);
    result.config_tags["exercise_steps"] = std::to_string(request.exercise_steps);
    result.config_tags["maturity"] = std::to_string(request.maturity);
    result.config_tags["option_type"] = request.is_put ? "put" : "call";
    result.config_tags["risk_free_rate"] = std::to_string(request.risk_free_rate);
    result.config_tags["spot"] = std::to_string(request.spot);
    result.config_tags["strike"] = std::to_string(request.strike);
    result.config_tags["volatility"] = std::to_string(request.volatility);
    return result;
}

} // namespace qrp::analytics::lsmc
