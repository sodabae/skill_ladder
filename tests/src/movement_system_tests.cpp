#include <gtest/gtest.h>
#include <cmath>
#include <random>

#include "MovementSystem.h"
#include "UpdateEvent.h"
#include "eventBus.h"
#include "test_helpers.h"

namespace {

class MovementSystemFixture : public ::testing::Test
{
protected:
    eventBus bus;
    MovementSystem movement{bus};
};

TEST_F(MovementSystemFixture, PrintsUpdatedPositionForSingleUpdate)
{
    CoutCapture capture;
    bus.publish(UpdateEvent{2.0F});

    EXPECT_EQ(capture.str(), "Position: 10\n");
}

TEST_F(MovementSystemFixture, PositionAccumulatesAcrossUpdates)
{
    //CoutCapture capture;
    //bus.publish(UpdateEvent{1.0F});
    //bus.publish(UpdateEvent{0.5F});
    //EXPECT_EQ(capture.str(), "Position: 5\nPosition: 7.5\n");
    {
        CoutCapture subcapture;
        bus.publish(UpdateEvent{1.0F});
        EXPECT_EQ(subcapture.str(), "Position: 5\n");
    }

    {
        CoutCapture subcapture;
        bus.publish(UpdateEvent{0.5F});
        EXPECT_EQ(subcapture.str(), "Position: 7.5\n");
    }

}

TEST_F(MovementSystemFixture, RandomUpdateEventsAccumulateAcrossRandomPublishCount)
{
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> publishCountDist(7, 25);
    std::uniform_real_distribution<float> deltaTimeDist(0.0F, 200.0F);

    const int publishCount = publishCountDist(rng);
    std::vector<float> deltaTimes;
    deltaTimes.reserve(static_cast<std::size_t>(publishCount));

    for (int i = 0; i < publishCount; ++i)
    {
        deltaTimes.push_back(deltaTimeDist(rng));
    }

    CoutCapture capture;
    for (const float deltaTime : deltaTimes)
    {
        bus.publish(UpdateEvent{deltaTime});
    }

    std::istringstream lines(capture.str());
    std::string line;

    float runningExpected = 0.0F;
    int observedLines = 0;

    for (const float deltaTime : deltaTimes)
    {
        ASSERT_TRUE(static_cast<bool>(std::getline(lines, line)));
        ASSERT_EQ(line.rfind("Position: ", 0), 0U);

        const std::string valueText = line.substr(std::string("Position: ").size());
        const float observedPosition = std::stof(valueText);

        runningExpected += 5.0F * deltaTime;
        EXPECT_NEAR(observedPosition, runningExpected, 0.01F);
        ++observedLines;
    }

    EXPECT_EQ(observedLines, publishCount);
    EXPECT_FALSE(static_cast<bool>(std::getline(lines, line)));
}

}  // namespace
