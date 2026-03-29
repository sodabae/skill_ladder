#pragma once

#include "System.h"
#include "eventBus.h"
#include "UpdateEvent.h"
#include "World.h"

class PhysicsIntegrationSystem : public System
{
public:
    PhysicsIntegrationSystem(eventBus& bus, World& world)
        : m_bus(bus), m_world(world) {}

    void initialize() override
    {
        m_sub = m_bus.subscribe<UpdateEvent>(
            [this](const UpdateEvent& e){ onUpdate(e); }
        );
    }

private:
    void onUpdate(const UpdateEvent& e)
    {
        Vec2 gravity{0.0f, -9.8f};

        for (auto& p : m_world.particles)
        {
            p.force += gravity * p.mass;

            Vec2 accel = p.force * p.inverseMass;
            p.velocity += accel * e.deltaTime;
            p.position += p.velocity * e.deltaTime;

            p.force = Vec2{0,0};
        }
    }

private:
    eventBus& m_bus;
    World& m_world;
    eventBus::Subscription m_sub;
};