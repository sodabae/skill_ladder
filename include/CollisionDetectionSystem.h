#pragma once

#include "System.h"
#include "eventBus.h"
#include "UpdateEvent.h"
#include "CollisionEvent.h"
#include "World.h"
#include "SpatialGrid.h"

class CollisionDetectionSystem : public System
{
public:
    CollisionDetectionSystem(eventBus& bus, World& world, SpatialGrid& grid)
        : m_bus(bus), m_world(world), m_grid(grid) {}

    void initialize() override
    {
        m_sub = m_bus.subscribe<UpdateEvent>(
            [this](const UpdateEvent& /*e*/){ detect(); }
        );
    }

private:
    void detect()
    {
        for (auto& p : m_world.particles)
        {
            auto [cx, cy] = m_grid.cellCoords(p.position);

            for (int dx=-1; dx<=1; dx++)
            for (int dy=-1; dy<=1; dy++)
            {
                auto& cell = m_grid.getCell(cx+dx, cy+dy);

                for (auto* other : cell)
                {
                    if (&p >= other) continue;

                    Vec2 delta = other->position - p.position;
                    float r = p.radius + other->radius;

                    if (delta.lengthSquared() < r*r)
                    {
                        m_bus.publish(CollisionEvent{&p, other});
                    }
                }
            }
        }
    }

private:
    eventBus& m_bus;
    World& m_world;
    SpatialGrid& m_grid;
    eventBus::Subscription m_sub;
};