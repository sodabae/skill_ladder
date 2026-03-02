#include <gtest/gtest.h>

#include "Logger.h"
#include "UpdateEvent.h"
#include "eventBus.h"
#include "test_helpers.h"

namespace {

class LoggerFixture : public ::testing::Test
{
protected:
    eventBus bus;
    Logger logger{bus};
};

TEST_F(LoggerFixture, DoesNotLogBeforeSubscribing)
{
    CoutCapture capture;
    bus.publish(UpdateEvent{1.0F});

    EXPECT_TRUE(capture.str().empty());
}

TEST_F(LoggerFixture, LogsUpdateAfterSubscribing)
{
    logger.subscribeUpdateEvent();

    CoutCapture capture;
    bus.publish(UpdateEvent{3.5F});

    EXPECT_EQ(capture.str(), "Logger Class: deltaTime = 3.5\n");
}

TEST_F(LoggerFixture, MultipleSubscriptionsReceiveMultipleLogs)
{
    logger.subscribeUpdateEvent();
    logger.subscribeUpdateEvent();

    CoutCapture capture;
    bus.publish(UpdateEvent{2.0F});

    EXPECT_EQ(capture.str(), "Logger Class: deltaTime = 2\nLogger Class: deltaTime = 2\n");
}

}  // namespace
