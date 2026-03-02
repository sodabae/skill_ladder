#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <utility>

#include "UpdateEvent.h"
#include "eventBus.h"

namespace {

using ::testing::_;

struct OtherEvent
{
    int value;
};

class EventBusFixture : public ::testing::Test
{
protected:
    eventBus bus;
};

TEST_F(EventBusFixture, PublishInvokesSubscribedHandler)
{
    testing::MockFunction<void(const UpdateEvent&)> updateHandler;

    EXPECT_CALL(updateHandler, Call(_)).Times(1);

    auto sub = bus.subscribe<UpdateEvent>(
        [&](const UpdateEvent& event)
        {
            updateHandler.Call(event);
        }
    );

    bus.publish(UpdateEvent{2.5F});
}

TEST_F(EventBusFixture, SubscriptionDestructorUnsubscribes)
{
    int calledCount = 0;

    {
        auto sub = bus.subscribe<UpdateEvent>(
            [&](const UpdateEvent&)
            {
                ++calledCount;
            }
        );

        bus.publish(UpdateEvent{1.0F});
    }

    bus.publish(UpdateEvent{1.0F});
    EXPECT_EQ(calledCount, 1);
}

TEST_F(EventBusFixture, ExplicitUnsubscribeRemovesHandler)
{
    int calledCount = 0;

    auto sub = bus.subscribe<UpdateEvent>(
        [&](const UpdateEvent&)
        {
            ++calledCount;
        }
    );

    bus.publish(UpdateEvent{1.0F});
    bus.unsubscribe<UpdateEvent>(0);
    bus.publish(UpdateEvent{1.0F});

    EXPECT_EQ(calledCount, 1);
}

TEST_F(EventBusFixture, SubscriberAddedDuringPublishStartsNextPublish)
{
    int firstCalled = 0;
    int secondCalled = 0;

    eventBus::Subscription secondSub;

    auto firstSub = bus.subscribe<UpdateEvent>(
        [&](const UpdateEvent&)
        {
            ++firstCalled;
            if (firstCalled == 1)
            {
                secondSub = bus.subscribe<UpdateEvent>(
                    [&](const UpdateEvent&)
                    {
                        ++secondCalled;
                    }
                );
            }
        }
    );

    bus.publish(UpdateEvent{1.0F});
    EXPECT_EQ(firstCalled, 1);
    EXPECT_EQ(secondCalled, 0);

    bus.publish(UpdateEvent{1.0F});
    EXPECT_EQ(firstCalled, 2);
    EXPECT_EQ(secondCalled, 1);
}

TEST_F(EventBusFixture, UnsubscribeDuringPublishDefersAndSkipsFuturePublishes)
{
    int firstCalled = 0;
    int secondCalled = 0;

    auto secondSub = bus.subscribe<UpdateEvent>(
        [&](const UpdateEvent&)
        {
            ++secondCalled;
        }
    );

    auto firstSub = bus.subscribe<UpdateEvent>(
        [&](const UpdateEvent&)
        {
            ++firstCalled;
            secondSub = eventBus::Subscription{};
        }
    );

    bus.publish(UpdateEvent{1.0F});
    EXPECT_EQ(firstCalled, 1);
    EXPECT_EQ(secondCalled, 1);

    bus.publish(UpdateEvent{1.0F});
    EXPECT_EQ(firstCalled, 2);
    EXPECT_EQ(secondCalled, 1);
}

TEST_F(EventBusFixture, SubscriptionMoveTransfersOwnership)
{
    int calledCount = 0;

    auto original = bus.subscribe<UpdateEvent>(
        [&](const UpdateEvent&)
        {
            ++calledCount;
        }
    );

    auto moved = std::move(original);
    bus.publish(UpdateEvent{1.0F});
    EXPECT_EQ(calledCount, 1);

    moved = eventBus::Subscription{};
    bus.publish(UpdateEvent{1.0F});
    EXPECT_EQ(calledCount, 1);
}

TEST_F(EventBusFixture, DifferentEventTypesAreIndependent)
{
    int updateCalled = 0;
    int otherCalled = 0;

    auto updateSub = bus.subscribe<UpdateEvent>(
        [&](const UpdateEvent&)
        {
            ++updateCalled;
        }
    );

    auto otherSub = bus.subscribe<OtherEvent>(
        [&](const OtherEvent&)
        {
            ++otherCalled;
        }
    );

    bus.publish(UpdateEvent{1.0F});

    EXPECT_EQ(updateCalled, 1);
    EXPECT_EQ(otherCalled, 0);
}

}  // namespace
