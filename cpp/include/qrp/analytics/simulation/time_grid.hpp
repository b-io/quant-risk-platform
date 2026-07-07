#pragma once

// Declares simulation time-grid helpers with cached time-step lengths.

#include <cstddef>
#include <vector>

namespace qrp::analytics::simulation {

/**
 * @brief Ordered simulation times with helper access to time increments.
 */
class TimeGrid {
public:
    /**
     * @brief Creates an empty time grid.
     */
    TimeGrid() = default;

    /**
     * @brief Creates a time grid from ordered year-fraction times.
     */
    explicit TimeGrid(std::vector<double> times);

    /**
     * @brief Returns the number of time points.
     */
    std::size_t size() const;

    /**
     * @brief Returns the time point without bounds checking.
     */
    double operator[](std::size_t i) const;

    /**
     * @brief Returns the time point with bounds checking.
     */
    double at(std::size_t i) const {
        return times_.at(i);
    }

    /**
     * @brief Returns all time points.
     */
    const std::vector<double>& times() const {
        return times_;
    }

    /**
     * @brief Returns the time increment ending at index i.
     */
    double dt(std::size_t i) const;

private:
    std::vector<double> times_; // Ordered simulation times as year fractions.
};

} // namespace qrp::analytics::simulation
