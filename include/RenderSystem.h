#pragma once

#include "System.h"
#include "eventBus.h"
#include "World.h"

class RenderSystem : public System
{
public:
    RenderSystem(eventBus& bus, World& world)
        : m_bus(bus), m_world(world) {}

    void initialize() override
    {
        m_sub = m_bus.subscribe<UpdateEvent>(
            [this](const UpdateEvent&){ render();}
        );
    }
private: 
    void render()
    {
        for (size_t i=0; i<m_world.particles.size(); ++i)
        {
            const auto& p = m_world.particles[i];

            std::cout << "Particle " << i
                      << " pos(" << p.position.x << ", " << p.position.y << ") "
                      << "vel(" << p.velocity.x << ", " << p.velocity.y << ")\n";
        }
        std::cout << "------" << std::endl;
    }

private:
    eventBus& m_bus;
    World& m_world;
    eventBus::Subscription m_sub;

};