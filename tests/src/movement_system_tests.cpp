#include <gtest/gtest.h>

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
    CoutCapture capture;
    bus.publish(UpdateEvent{1.0F});
    bus.publish(UpdateEvent{0.5F});

    EXPECT_EQ(capture.str(), "Position: 5\nPosition: 7.5\n");
}

}  // namespace
