// Implements simulation time-grid and market-path primitives.

#include <qrp/analytics/simulation/stochastic_process.hpp>
#include <qrp/analytics/simulation/time_grid.hpp>

#include <utility>

namespace qrp::analytics::simulation {

TimeGrid::TimeGrid(std::vector<double> times) : times_(std::move(times)) {}

std::size_t TimeGrid::size() const {
    return times_.size();
}

double TimeGrid::operator[](std::size_t i) const {
    return times_[i];
}

double TimeGrid::dt(std::size_t i) const {
    if (i == 0)
        return times_[0];
    return times_[i] - times_[i - 1];
}

MarketPath::MarketPath(std::size_t num_steps, std::size_t dimension)
    : data_(num_steps, std::vector<double>(dimension)) {}

std::size_t MarketPath::num_steps() const {
    return data_.size();
}

std::size_t MarketPath::dimension() const {
    return data_.empty() ? 0 : data_[0].size();
}

double& MarketPath::operator()(std::size_t time_index, std::size_t dim_index) {
    return data_[time_index][dim_index];
}

double MarketPath::operator()(std::size_t time_index, std::size_t dim_index) const {
    return data_[time_index][dim_index];
}

const std::vector<double>& MarketPath::at(std::size_t time_index) const {
    return data_.at(time_index);
}

} // namespace qrp::analytics::simulation
