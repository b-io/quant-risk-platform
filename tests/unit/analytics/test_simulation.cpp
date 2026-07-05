// Verifies simulation primitives for time grids, market paths, and stochastic-process stepping.

#include <qrp/analytics/simulation/gbm.hpp>
#include <qrp/analytics/simulation/stochastic_process.hpp>
#include <qrp/analytics/simulation/time_grid.hpp>

#include <gtest/gtest.h>

#include <cmath>
#include <random>
#include <vector>

using namespace qrp::analytics::simulation;

TEST(SimulationTest, TimeGridComputesStepSizes) {
    TimeGrid grid({0.0, 0.25, 1.0});

    ASSERT_EQ(grid.size(), 3U);
    EXPECT_DOUBLE_EQ(grid.dt(0), 0.0);
    EXPECT_DOUBLE_EQ(grid.dt(1), 0.25);
    EXPECT_DOUBLE_EQ(grid.dt(2), 0.75);
}

TEST(SimulationTest, MarketPathStoresValuesByTimeAndDimension) {
    MarketPath path(2, 3);
    path(0, 0) = 1.0;
    path(0, 1) = 2.0;
    path(1, 2) = 5.0;

    EXPECT_EQ(path.num_steps(), 2U);
    EXPECT_EQ(path.dimension(), 3U);
    EXPECT_DOUBLE_EQ(path(0, 0), 1.0);
    EXPECT_DOUBLE_EQ(path.at(0)[1], 2.0);
    EXPECT_DOUBLE_EQ(path(1, 2), 5.0);
}

TEST(SimulationTest, GeometricBrownianMotionIsDeterministicWithZeroVolatility) {
    TimeGrid grid({0.0, 1.0, 2.0});
    GeometricBrownianMotion gbm(100.0, 0.05, 0.0);
    MarketPath path(grid.size(), gbm.dimension());
    std::mt19937 gen(123);

    gbm.simulatePath(grid, gen, path);

    EXPECT_DOUBLE_EQ(path(0, 0), 100.0);
    EXPECT_NEAR(path(1, 0), 100.0 * std::exp(0.05), 1e-12);
    EXPECT_NEAR(path(2, 0), 100.0 * std::exp(0.10), 1e-12);
}

TEST(SimulationTest, TimeGridAndMarketPathExposeContainerAccessors) {
    TimeGrid grid({0.0, 0.5, 1.0});

    EXPECT_DOUBLE_EQ(grid[1], 0.5);
    EXPECT_EQ(grid.times().size(), 3U);

    MarketPath path(2, 2);
    path(1, 1) = 7.0;
    const MarketPath& const_path = path;
    EXPECT_DOUBLE_EQ(const_path(1, 1), 7.0);

    MarketPath empty_path(0, 2);
    EXPECT_EQ(empty_path.num_steps(), 0U);
    EXPECT_EQ(empty_path.dimension(), 0U);
}