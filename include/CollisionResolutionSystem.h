#pragma once

#include "System.h"
#include "eventBus.h"
#include "CollisionEvent.h"

class CollisionResolutionSystem : public System
{
public:
    CollisionResolutionSystem(eventBus& bus, World& world)
        : m_bus(bus), m_world(world) {}

    void initialize() override
    {
        m_sub = m_bus.subscribe<CollisionEvent>(
            [this](const CollisionEvent& e){ resolve(e); }
        );
    }

private:
    void resolve(const CollisionEvent& e)
    {
        Particle& a = *(e.a);
        Particle& b = *(e.b);

        Vec2 delta = b.position - a.position;
        float dist = std::sqrt(delta.lengthSquared());
        if (dist == 0.0f) return;

        Vec2 normal = delta / dist;

        float totalInvMass = a.inverseMass + b.inverseMass;
        if (totalInvMass == 0) return;

        float penetration = (a.radius + b.radius) -dist;
        Vec2 correction = normal * (penetration / totalInvMass);

        a.position -= correction * a.inverseMass;
        b.position += correction * b.inverseMass;
    }

private: 
    eventBus& m_bus;
    eventBus::Subscription m_sub;
    World& m_world;
};
