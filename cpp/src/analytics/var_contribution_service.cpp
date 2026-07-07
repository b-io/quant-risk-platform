// Implements historical VaR/ES contribution analytics.

#include <qrp/analytics/var_contribution_service.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <stdexcept>

namespace qrp::analytics {
namespace {

constexpr const char* kSignConvention = "positive_loss";
constexpr const char* kHistoricalMethod = "historical_component_var_es";

struct TailMetric {
    double var_value = 0.0;
    double expected_shortfall = 0.0;
    std::size_t var_index = 0;
    std::size_t tail_count = 0;
};

enum class ContributionMetric {
    Var,
    ExpectedShortfall,
};

std::size_t tail_index(std::size_t scenario_count, double confidence_level) {
    const double tail_probability = 1.0 - confidence_level;
    const auto raw_index = static_cast<std::size_t>(static_cast<double>(scenario_count) * tail_probability);
    return std::min(raw_index, scenario_count - 1U);
}

TailMetric calculate_tail_metric(const std::vector<double>& pnls, double confidence_level) {
    if (pnls.empty()) {
        throw std::runtime_error("VaR contribution requires at least one scenario PnL");
    }

    std::vector<double> sorted = pnls;
    std::sort(sorted.begin(), sorted.end());
    const auto index = tail_index(sorted.size(), confidence_level);
    const auto tail_count = index + 1U;
    const double tail_sum =
        std::accumulate(sorted.begin(), sorted.begin() + static_cast<std::ptrdiff_t>(tail_count), 0.0);

    TailMetric metric;
    metric.var_index = index;
    metric.tail_count = tail_count;
    metric.var_value = -sorted[index];
    metric.expected_shortfall = -(tail_sum / static_cast<double>(tail_count));
    return metric;
}

std::vector<std::size_t> sorted_portfolio_indices(const std::vector<HistoricalScenarioPnl>& scenario_pnls) {
    std::vector<std::size_t> indices(scenario_pnls.size());
    std::iota(indices.begin(), indices.end(), 0U);
    std::sort(indices.begin(), indices.end(), [&](std::size_t lhs, std::size_t rhs) {
        return scenario_pnls[lhs].portfolio_pnl < scenario_pnls[rhs].portfolio_pnl;
    });
    return indices;
}

std::string group_value(const domain::Trade& trade, const std::string& aggregation_type) {
    if (aggregation_type == "trade")
        return trade.id;
    if (aggregation_type == "book")
        return trade.book;
    if (aggregation_type == "strategy")
        return trade.strategy;
    if (aggregation_type == "currency")
        return trade.currency;
    if (aggregation_type == "asset_class")
        return trade.asset_class;
    return "";
}

std::map<std::string, std::vector<std::string>> build_trade_groups(const domain::Portfolio& portfolio,
                                                                   const std::string& aggregation_type) {
    std::map<std::string, std::vector<std::string>> groups;
    for (const auto& trade : portfolio.trades) {
        if (!trade)
            continue;
        auto key = group_value(*trade, aggregation_type);
        if (key.empty())
            key = "unassigned";
        groups[key].push_back(trade->id);
    }
    return groups;
}

std::vector<double> portfolio_pnls(const std::vector<HistoricalScenarioPnl>& scenario_pnls) {
    std::vector<double> pnls;
    pnls.reserve(scenario_pnls.size());
    for (const auto& scenario : scenario_pnls) {
        pnls.push_back(scenario.portfolio_pnl);
    }
    return pnls;
}

double group_pnl_for_scenario(const HistoricalScenarioPnl& scenario, const std::vector<std::string>& trade_ids) {
    double pnl = 0.0;
    for (const auto& trade_id : trade_ids) {
        const auto it = scenario.trade_pnls.find(trade_id);
        if (it != scenario.trade_pnls.end()) {
            pnl += it->second;
        }
    }
    return pnl;
}

std::vector<double> group_pnl_path(const std::vector<HistoricalScenarioPnl>& scenario_pnls,
                                   const std::vector<std::string>& trade_ids) {
    std::vector<double> pnls;
    pnls.reserve(scenario_pnls.size());
    for (const auto& scenario : scenario_pnls) {
        pnls.push_back(group_pnl_for_scenario(scenario, trade_ids));
    }
    return pnls;
}

std::vector<double> remove_group_path(const std::vector<HistoricalScenarioPnl>& scenario_pnls,
                                      const std::vector<double>& group_pnls) {
    std::vector<double> pnls;
    pnls.reserve(scenario_pnls.size());
    for (std::size_t i = 0; i < scenario_pnls.size(); ++i) {
        pnls.push_back(scenario_pnls[i].portfolio_pnl - group_pnls[i]);
    }
    return pnls;
}

double tail_average_component(const std::vector<double>& group_pnls,
                              const std::vector<std::size_t>& sorted_indices,
                              std::size_t tail_count) {
    double tail_sum = 0.0;
    for (std::size_t i = 0; i < tail_count; ++i) {
        tail_sum += group_pnls[sorted_indices[i]];
    }
    return -(tail_sum / static_cast<double>(tail_count));
}

double safe_ratio(double numerator, double denominator) {
    constexpr double epsilon = 1e-12;
    return std::abs(denominator) > epsilon ? numerator / denominator : 0.0;
}

VarContribution make_contribution(const std::string& aggregation_type,
                                  const std::string& aggregation_key,
                                  const std::vector<HistoricalScenarioPnl>& scenario_pnls,
                                  const std::vector<double>& group_pnls,
                                  const TailMetric& portfolio_metric,
                                  const std::vector<std::size_t>& sorted_indices,
                                  double confidence_level,
                                  const std::string& var_scenario_name,
                                  const std::string& aggregation_rule) {
    const auto standalone_metric = calculate_tail_metric(group_pnls, confidence_level);
    const auto without_group_metric =
        calculate_tail_metric(remove_group_path(scenario_pnls, group_pnls), confidence_level);
    const auto var_scenario_index = sorted_indices[portfolio_metric.var_index];

    VarContribution contribution;
    contribution.aggregation_type = aggregation_type;
    contribution.aggregation_key = aggregation_key;
    contribution.var_contribution = -group_pnls[var_scenario_index];
    contribution.expected_shortfall_contribution =
        tail_average_component(group_pnls, sorted_indices, portfolio_metric.tail_count);
    contribution.portfolio_var_share = safe_ratio(contribution.var_contribution, portfolio_metric.var_value);
    contribution.portfolio_expected_shortfall_share =
        safe_ratio(contribution.expected_shortfall_contribution, portfolio_metric.expected_shortfall);
    contribution.standalone_var = standalone_metric.var_value;
    contribution.standalone_expected_shortfall = standalone_metric.expected_shortfall;
    contribution.incremental_var = portfolio_metric.var_value - without_group_metric.var_value;
    contribution.incremental_expected_shortfall =
        portfolio_metric.expected_shortfall - without_group_metric.expected_shortfall;
    contribution.marginal_var = contribution.incremental_var;
    contribution.marginal_expected_shortfall = contribution.incremental_expected_shortfall;
    contribution.confidence_level = confidence_level;
    contribution.scenario_count = static_cast<int>(scenario_pnls.size());
    contribution.tail_scenario_count = static_cast<int>(portfolio_metric.tail_count);
    contribution.var_scenario_name = var_scenario_name;
    contribution.sign_convention = kSignConvention;
    contribution.aggregation_rule = aggregation_rule;
    contribution.calculation_method = kHistoricalMethod;
    return contribution;
}

void calculate_component_residuals(VarContributionReport& report) {
    std::map<std::string, double> var_sums;
    std::map<std::string, double> expected_shortfall_sums;
    for (const auto& contribution : report.contributions) {
        var_sums[contribution.aggregation_type] += contribution.var_contribution;
        expected_shortfall_sums[contribution.aggregation_type] += contribution.expected_shortfall_contribution;
    }

    for (const auto& [aggregation_type, value] : var_sums) {
        report.var_component_residuals[aggregation_type] = report.var_value - value;
    }
    for (const auto& [aggregation_type, value] : expected_shortfall_sums) {
        report.expected_shortfall_component_residuals[aggregation_type] = report.expected_shortfall - value;
    }
}

std::vector<VarContribution> top_contributors(const VarContributionReport& report,
                                              const std::string& aggregation_type,
                                              std::size_t limit,
                                              ContributionMetric metric) {
    std::vector<VarContribution> rows;
    for (const auto& contribution : report.contributions) {
        if (contribution.aggregation_type == aggregation_type) {
            rows.push_back(contribution);
        }
    }

    auto metric_value = [metric](const VarContribution& contribution) {
        return metric == ContributionMetric::Var ? contribution.var_contribution
                                                 : contribution.expected_shortfall_contribution;
    };
    std::sort(rows.begin(), rows.end(), [&](const VarContribution& lhs, const VarContribution& rhs) {
        const double lhs_abs = std::abs(metric_value(lhs));
        const double rhs_abs = std::abs(metric_value(rhs));
        if (lhs_abs == rhs_abs) {
            return lhs.aggregation_key < rhs.aggregation_key;
        }
        return lhs_abs > rhs_abs;
    });

    if (rows.size() > limit) {
        rows.resize(limit);
    }
    return rows;
}

std::map<std::string, std::vector<double>>
factor_allocated_pnls(const std::vector<HistoricalScenarioPnl>& scenario_pnls) {
    std::map<std::string, std::vector<double>> paths;
    for (std::size_t scenario_index = 0; scenario_index < scenario_pnls.size(); ++scenario_index) {
        const auto& scenario = scenario_pnls[scenario_index];
        double denominator = 0.0;
        for (const auto& [factor_id, shock] : scenario.factor_shocks) {
            denominator += std::abs(shock);
            if (!paths.contains(factor_id)) {
                paths[factor_id] = std::vector<double>(scenario_pnls.size(), 0.0);
            }
        }
        if (denominator <= 0.0 || scenario.factor_shocks.empty()) {
            continue;
        }
        for (const auto& [factor_id, shock] : scenario.factor_shocks) {
            paths[factor_id][scenario_index] = scenario.portfolio_pnl * (std::abs(shock) / denominator);
        }
    }
    return paths;
}

} // namespace

VarContributionReport
VarContributionService::calculate_historical_contributions(const domain::Portfolio& portfolio,
                                                           const std::vector<HistoricalScenarioPnl>& scenario_pnls,
                                                           double confidence_level) {

    if (scenario_pnls.empty()) {
        throw std::runtime_error("Historical VaR contribution requires at least one scenario");
    }
    if (confidence_level <= 0.0 || confidence_level >= 1.0) {
        throw std::runtime_error("Historical VaR contribution confidence level must be between 0 and 1");
    }

    const auto portfolio_metric = calculate_tail_metric(portfolio_pnls(scenario_pnls), confidence_level);
    const auto sorted_indices = sorted_portfolio_indices(scenario_pnls);
    const auto var_scenario_name = scenario_pnls[sorted_indices[portfolio_metric.var_index]].scenario_name;

    VarContributionReport report;
    report.confidence_level = confidence_level;
    report.var_value = portfolio_metric.var_value;
    report.expected_shortfall = portfolio_metric.expected_shortfall;
    report.scenario_count = static_cast<int>(scenario_pnls.size());
    report.tail_scenario_count = static_cast<int>(portfolio_metric.tail_count);
    report.var_scenario_name = var_scenario_name;
    report.scenario_pnls = scenario_pnls;
    report.diagnostics["sign_convention"] = kSignConvention;
    report.diagnostics["tail_rule"] =
        "sort portfolio PnL ascending; VaR is negative tail quantile; ES is negative tail mean";
    report.diagnostics["incremental_rule"] = "portfolio VaR/ES minus VaR/ES with aggregation group removed";
    report.diagnostics["factor_rule"] =
        "scenario portfolio PnL allocated across shocked factors by absolute shock magnitude";
    report.diagnostics["residual_rule"] = "aggregate VaR/ES minus summed component contribution by aggregation type";

    const std::vector<std::string> aggregation_types = {"trade", "book", "strategy", "currency", "asset_class"};
    for (const auto& aggregation_type : aggregation_types) {
        const auto groups = build_trade_groups(portfolio, aggregation_type);
        for (const auto& [group_key, trade_ids] : groups) {
            report.contributions.push_back(make_contribution(aggregation_type,
                                                             group_key,
                                                             scenario_pnls,
                                                             group_pnl_path(scenario_pnls, trade_ids),
                                                             portfolio_metric,
                                                             sorted_indices,
                                                             confidence_level,
                                                             var_scenario_name,
                                                             "trade scenario PnL summed by " + aggregation_type));
        }
    }

    for (const auto& [factor_id, factor_pnls] : factor_allocated_pnls(scenario_pnls)) {
        report.contributions.push_back(make_contribution("risk_factor",
                                                         factor_id,
                                                         scenario_pnls,
                                                         factor_pnls,
                                                         portfolio_metric,
                                                         sorted_indices,
                                                         confidence_level,
                                                         var_scenario_name,
                                                         "portfolio scenario PnL allocated by absolute factor shock"));
    }

    calculate_component_residuals(report);
    return report;
}

std::vector<VarContribution> VarContributionService::top_var_contributors(const VarContributionReport& report,
                                                                          const std::string& aggregation_type,
                                                                          std::size_t limit) {
    return top_contributors(report, aggregation_type, limit, ContributionMetric::Var);
}

std::vector<VarContribution>
VarContributionService::top_expected_shortfall_contributors(const VarContributionReport& report,
                                                            const std::string& aggregation_type,
                                                            std::size_t limit) {
    return top_contributors(report, aggregation_type, limit, ContributionMetric::ExpectedShortfall);
}

} // namespace qrp::analytics
