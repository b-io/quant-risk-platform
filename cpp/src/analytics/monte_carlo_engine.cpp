#include <qrp/analytics/monte_carlo_engine.hpp>
#include <qrp/analytics/covariance_estimator.hpp>
#include <qrp/analytics/pricing_context.hpp>
#include <qrp/analytics/valuation_service.hpp>
#include <qrp/instruments/instrument_factory.hpp>
#include <qrp/market/market_snapshot.hpp>
#include <qrp/util/logger.hpp>
#include <ql/indexes/ibor/usdlibor.hpp>
#include <ql/instrument.hpp>
#include <ql/math/matrix.hpp>
#include <ql/math/matrixutilities/choleskydecomposition.hpp>
#include <ql/math/randomnumbers/boxmullergaussianrng.hpp>
#include <ql/math/randomnumbers/mt19937uniformrng.hpp>
#include <ql/settings.hpp>
#include <ql/termstructures/yield/discountcurve.hpp>
#include <ql/time/calendars/target.hpp>
#include <fmt/format.h>
#include <algorithm>
#include <map>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <tuple>

/*
Design note (see docs/design/ANALYTICS_SERVICES.md and docs/risk/MONTE_CARLO.md):
- We implement a one-step, factor-based Monte Carlo to generate coherent shocks across currencies.
- The factor covariance is represented by a symmetric positive-definite matrix and sampled via
  Cholesky decomposition (L·Z). This produces correlated Gaussian shocks consistent with the covariance.
- We do not rebuild curves for each path; instead we bump quote handles and rely on QuantLib's Observer pattern
  to lazily reprice cached instruments. This drastically improves throughput.
*/
namespace qrp::analytics {

SimulationResult MonteCarloEngine::run_simulation(
    const domain::Portfolio& portfolio,
    const domain::MarketSnapshot& base_market_dto,
    const std::vector<domain::FactorDefinition>& factors,
    const std::vector<domain::FactorBinding>& bindings,
    const QuantLib::Matrix& covariance,
    const MonteCarloConfig& config) {

    if (config.num_paths <= 0) return {};

    size_t num_factors = factors.size();
    if (num_factors == 0) {
        throw std::runtime_error("MonteCarloEngine: factors are required");
    }

    if (config.require_bindings && bindings.empty()) {
        throw std::runtime_error("MonteCarloEngine: factor bindings are required");
    }

    // Validate covariance dimensions
    if (covariance.rows() != num_factors || covariance.columns() != num_factors) {
        throw std::runtime_error("MonteCarloEngine: Covariance matrix dimensions must match factor count (" 
            + std::to_string(num_factors) + "x" + std::to_string(num_factors) + ")");
    }

    SimulationResult result;
    result.portfolio_values.reserve(config.num_paths);
    result.portfolio_pnls.reserve(config.num_paths);
    result.seed = config.seed;
    result.horizon_days = config.horizon_days;
    result.mode = config.mode;
    for (const auto& f : factors) result.factor_ids.push_back(f.factor_id);

    // 1. Setup Base Market and Instrument Cache
    qrp::market::MarketSnapshot base_market(base_market_dto);
    auto state = base_market.built_state();
    QuantLib::Date t0 = state->valuation_date();

    std::shared_ptr<market::MarketState> horizon_state = nullptr;
    domain::MarketSnapshot frozen_aged_market_dto = base_market_dto;
    QuantLib::Date tH = t0;

    if (config.mode == MonteCarloMode::AgedHorizonRevaluation) {
        // Compute business-day horizon date
        QuantLib::TARGET calendar;
        tH = calendar.advance(t0, static_cast<int>(config.horizon_days), QuantLib::Days);
        
        // Build frozen-aged horizon market
        horizon_state = std::make_shared<market::MarketState>(tH);

        // Copy fixings to horizon state
        for (const auto& [index_name, date_fixings] : state->fixings()) {
            for (const auto& [date, value] : date_fixings) {
                horizon_state->add_fixing(index_name, date, value);
            }
        }
        
        // Preserve reactive repricing by rebuilding curves in the horizon state.
        // Frozen-aging policy: advance the valuation date while keeping market quote
        // levels unchanged. Forward-projected market states should be supplied by a
        // dedicated market-data evolution model when that workflow is introduced.
        frozen_aged_market_dto.valuation_date = fmt::format("{:4d}-{:02d}-{:02d}", (int)tH.year(), (int)tH.month(), (int)tH.dayOfMonth());
        
        // Step 2: Re-build curves in the horizon_state using these quotes.
        std::map<std::string, domain::MarketQuote> quote_map;
        for (const auto& q : base_market_dto.quotes) quote_map[q.id] = q;

        for (const auto& curve_spec : base_market_dto.curves) {
            auto curve = market::CurveBuilder::build_curve(curve_spec, quote_map, tH, horizon_state);
            if (curve) {
                horizon_state->add_curve(curve_spec.id, curve);
            }
        }

        // Initialize any quotes not part of curves
        for (const auto& q : base_market_dto.quotes) {
            if (!horizon_state->get_quote_handle(q.id)) {
                horizon_state->add_quote(q.id, q.value);
            }
        }
        
        // Final pass: ensure all quotes from snapshot are in the state to avoid construction failures
        for (const auto& q : base_market_dto.quotes) {
            if (!horizon_state->get_quote_handle(q.id)) {
                horizon_state->add_quote(q.id, q.value);
            }
        }
        
        // Capture the final frozen-aged quote baseline for MC reset
        frozen_aged_market_dto = horizon_state->capture_snapshot();
        // Ensure valuation date is preserved in the snapshot for resolver consistency
        frozen_aged_market_dto.valuation_date = fmt::format("{:4d}-{:02d}-{:02d}", (int)tH.year(), (int)tH.month(), (int)tH.dayOfMonth());
    }

    // Pricing context for base and optionally horizon
    PricingContext base_context(state);
    std::unique_ptr<PricingContext> horizon_context;
    if (horizon_state) {
        horizon_context = std::make_unique<PricingContext>(horizon_state);
    }

    std::vector<std::pair<std::string, std::tuple<QuantLib::ext::shared_ptr<QuantLib::Instrument>, double, double>>> base_instruments;
    std::vector<std::pair<std::string, std::tuple<QuantLib::ext::shared_ptr<QuantLib::Instrument>, double, double>>> horizon_instruments;

    double base_total_npv = 0.0;
    double frozen_aged_total_npv = 0.0;

    result.num_trades_total = static_cast<int>(portfolio.trades.size());

    for (const auto& trade_ptr : portfolio.trades) {
        const auto& trade = *trade_ptr;
        const auto profile = ValuationService::pricing_profile(trade);

        QuantLib::Settings::instance().evaluationDate() = t0;
        try {
            auto inst = instruments::InstrumentFactory::create_instrument(trade, base_context);
            if (inst) {
                base_instruments.push_back({trade.id, {inst, profile.multiplier, profile.additive_npv}});
                base_total_npv += ValuationService::price_instrument(trade, *inst);
                result.num_trades_priced_t0++;
            } else {
                result.num_trades_failed_t0++;
                result.construction_errors[trade.id] = "InstrumentFactory returned nullptr (unsupported trade type?)";
            }
        } catch (const std::exception& e) {
            result.num_trades_failed_t0++;
            result.construction_errors[trade.id] = e.what();
            util::Logger::get()->warn("Failed to create base instrument for trade {}: {}", trade.id, e.what());
        } catch (...) {
            result.num_trades_failed_t0++;
            result.construction_errors[trade.id] = "Unknown exception during base instrument construction";
            util::Logger::get()->warn("Failed to create base instrument for trade {}: Unknown exception", trade.id);
        }
        
        if (horizon_context) {
            QuantLib::Settings::instance().evaluationDate() = tH;
            try {
                auto h_inst = instruments::InstrumentFactory::create_instrument(trade, *horizon_context);
                if (h_inst) {
                    horizon_instruments.push_back({trade.id, {h_inst, profile.multiplier, profile.additive_npv}});
                    frozen_aged_total_npv += ValuationService::price_instrument(trade, *h_inst);
                    result.num_trades_priced_tH++;
                } else {
                    result.num_trades_failed_tH++;
                    result.construction_errors[trade.id + "_tH"] = "InstrumentFactory returned nullptr at tH";
                }
            } catch (const std::exception& e) {
                std::string msg = e.what();
                if (msg.find("is after maturity date") != std::string::npos || 
                    msg.find("is past maturity date") != std::string::npos ||
                    msg.find("already matured") != std::string::npos) {
                    result.num_trades_expired_tH++;
                } else {
                    result.num_trades_failed_tH++;
                    result.construction_errors[trade.id + "_tH"] = msg;
                    util::Logger::get()->warn("Failed to create horizon instrument for trade {}: {}", trade.id, msg);
                }
            } catch (...) {
                result.num_trades_failed_tH++;
                result.construction_errors[trade.id + "_tH"] = "Unknown exception at tH";
                util::Logger::get()->warn("Failed to create horizon instrument for trade {}: Unknown exception at tH", trade.id);
            }
        }
    }
    result.base_portfolio_value = base_total_npv;
    double aging_pnl = frozen_aged_total_npv - base_total_npv;

    // Use current active state and reference DTO for simulation
    auto active_state = (config.mode == MonteCarloMode::AgedHorizonRevaluation) ? horizon_state : state;
    auto& active_instruments = (config.mode == MonteCarloMode::AgedHorizonRevaluation) ? horizon_instruments : base_instruments;
    const auto& active_reference_dto = (config.mode == MonteCarloMode::AgedHorizonRevaluation) ? frozen_aged_market_dto : base_market_dto;
    double active_base_npv = (config.mode == MonteCarloMode::AgedHorizonRevaluation) ? frozen_aged_total_npv : base_total_npv;
    QuantLib::Date active_val_date = (config.mode == MonteCarloMode::AgedHorizonRevaluation) ? tH : t0;

    // 2. Setup Factor Model
    // Scale covariance if requested
    QuantLib::Matrix scaled_cov = covariance;
    if (!config.covariance_is_already_horizon_scaled && config.horizon_days != 1.0) {
        scaled_cov *= config.horizon_days;
    }

    // Ensure the matrix is symmetric positive definite before Cholesky.
    QuantLib::Matrix repaired_cov = CovarianceEstimator::repair_psd(scaled_cov, 1e-10);

    // Cholesky Decomposition: Find lower triangular matrix L such that Σ = L * L^T.
    QuantLib::Matrix L = QuantLib::CholeskyDecomposition(repaired_cov);

    // 3. Setup Random Number Generator
    QuantLib::MersenneTwisterUniformRng baseRng(static_cast<unsigned long>(config.seed));
    QuantLib::BoxMullerGaussianRng<QuantLib::MersenneTwisterUniformRng> rng(baseRng);

        // 4. Run paths
        for (int i = 0; i < config.num_paths; ++i) {
            std::vector<double> z(num_factors);
            for (size_t j = 0; j < num_factors; ++j) z[j] = rng.next().value;
            
            std::vector<double> shocks(num_factors, 0.0);
            for (size_t row = 0; row < num_factors; ++row) {
                for (size_t col = 0; col <= row; ++col) {
                    shocks[row] += L[row][col] * z[col];
                }
            }

            market::ScenarioDefinition path_scenario;
            for (size_t j = 0; j < num_factors; ++j) {
                path_scenario.factor_shocks[factors[j].factor_id] = shocks[j];
            }
            
            // Capture trace for first 3 paths
            bool capture_trace = (i < 3);
            PathTrace trace;
            if (capture_trace) {
                trace.path_index = i;
                trace.portfolio_value_before = base_total_npv;
                trace.portfolio_value_frozen_aged = frozen_aged_total_npv;
                trace.aging_pnl = aging_pnl;
                trace.factor_shocks = path_scenario.factor_shocks;
                trace.valuation_date_before = fmt::format("{:4d}-{:02d}-{:02d}", (int)t0.year(), (int)t0.month(), (int)t0.dayOfMonth());
                trace.valuation_date_after = fmt::format("{:4d}-{:02d}-{:02d}", (int)tH.year(), (int)tH.month(), (int)tH.dayOfMonth());
            }
                
            // Reset to path baseline before applying shocks
            active_state->reset_to_snapshot(active_reference_dto);

            // Get initial quote values for tracked factors from active_state (already aged if in Aged mode)
            if (capture_trace) {
                for (const auto& binding : bindings) {
                    if (path_scenario.factor_shocks.count(binding.factor_id)) {
                        double val = active_state->get_quote(binding.quote_id);
                        trace.quote_before_after[binding.quote_id] = {val, 0.0};
                    }
                }
            }

            // Set evaluation date before applying scenario
            QuantLib::Settings::instance().evaluationDate() = active_val_date;

            market::ScenarioEngine::apply_scenario_to_state(*active_state, active_reference_dto, path_scenario, factors, bindings);

            if (capture_trace) {
                for (auto& [quote_id, values] : trace.quote_before_after) {
                    values.second = active_state->get_quote(quote_id);
                }
            }

            double total_npv_path = 0.0;
            int path_priced = 0;
            int path_failed = 0;
            for (auto& [id, inst_tuple] : active_instruments) {
                auto& inst = std::get<0>(inst_tuple);
                double q = std::get<1>(inst_tuple);
                double offset = std::get<2>(inst_tuple);
                try {
                    total_npv_path += inst->NPV() * q + offset;
                    path_priced++;
                } catch (const std::exception& e) {
                    path_failed++;
                    if (capture_trace) {
                        util::Logger::get()->debug("Path {} failed to price trade {}: {}", i, id, e.what());
                    }
                }
            }
            
            if (capture_trace) {
                trace.portfolio_value_after = total_npv_path;
                trace.path_pnl = total_npv_path - active_base_npv; // Market-only P&L relative to aged portfolio
                trace.num_priced = path_priced;
                trace.num_failed = path_failed;
                
                if (config.mode == MonteCarloMode::AgedHorizonRevaluation) {
                    trace.market_pnl = total_npv_path - frozen_aged_total_npv;
                    trace.total_pnl = total_npv_path - base_total_npv;
                } else {
                    trace.market_pnl = trace.path_pnl;
                    trace.total_pnl = trace.path_pnl;
                }

                result.traces.push_back(trace);
            }

            result.portfolio_values.push_back(total_npv_path);
            
            // For AgedHorizonRevaluation, we want to report the TOTAL P&L relative to t0 in the summary,
            // consistent with how a risk manager would look at it.
            if (config.mode == MonteCarloMode::AgedHorizonRevaluation) {
                result.portfolio_pnls.push_back(total_npv_path - base_total_npv);
            } else {
                result.portfolio_pnls.push_back(total_npv_path - active_base_npv);
            }
        }

    // 5. Final Reset: Restore state to base_market_dto before returning.
    // This is polite to the caller who might use the state afterwards.
    active_state->reset_to_snapshot(base_market_dto);
    QuantLib::Settings::instance().evaluationDate() = t0;

    // 5. Calculate Risk Metrics: VaR and ES
    std::vector<double> sorted_pnls = result.portfolio_pnls;
    std::sort(sorted_pnls.begin(), sorted_pnls.end());

    int var_idx = static_cast<int>(config.num_paths * 0.05);
    if (var_idx < (int)sorted_pnls.size()) {
        result.var_95 = -sorted_pnls[var_idx];
        double es_sum = 0.0;
        for (int j = 0; j <= var_idx; ++j) es_sum += sorted_pnls[j];
        result.expected_shortfall_95 = -(es_sum / (var_idx + 1));
    }

    return result;
}

} // namespace qrp::analytics
