#include <gtest/gtest.h>
#include <qrp/analytics/simulation/gbm.hpp>
#include <qrp/analytics/simulation/stochastic_process.hpp>
#include <qrp/analytics/simulation/time_grid.hpp>
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
