#pragma once

#include "System.h"
#include "eventBus.h"
#include "UpdateEvent.h"
#include "World.h"
#include "SpatialGrid.h"

class SpatialPartitionSystem : public System
{
public: 
    SpatialPartitionSystem(eventBus& bus, World& world)
        : m_bus(bus), m_world(world) {}

    void initialize() override{
        m_sub = m_bus.subscribe<UpdateEvent>(
            [this](const UpdateEvent& /*e*/){ buildGrid(); }
        );
    }

    SpatialGrid& getGrid() { return m_grid; }

private:
    void buildGrid()
    {
        m_grid.clear();

        for (auto& p: m_world.particles)
        {
            m_grid.insert(&p);
        }

    }

private:
    eventBus& m_bus;
    World& m_world;
    SpatialGrid m_grid{1.0f};
    eventBus::Subscription m_sub;
};