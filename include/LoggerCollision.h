#pragma once

#include <iostream>
#include <vector>

#include "System.h"
#include "eventBus.h"
#include "CollisionEvent.h"
#include "World.h"

class LoggerCollision
     : public System
{
public: 
    LoggerCollision(eventBus& bus, World& world)
        :m_bus(bus), m_world(world){}

    void initialize() override
    {
        m_subscriptions.push_back(
            m_bus.subscribe<CollisionEvent>(
                [this](const CollisionEvent& e)
                {
                    onCollision(e);
                }
            )
        );
    }

private: 
    void onCollision(const CollisionEvent& e)
    {
        std::cout << "Collision detected between particles at:"
                  << "(" << e.a->position.x << ", " << e.a->position.y << ") and"
                  << "(" << e.b->position.x << ", " << e.b->position.y << ")"
                  << std::endl;
    }

private:
    eventBus& m_bus;
    std::vector<eventBus::Subscription> m_subscriptions;
    World& m_world;
};