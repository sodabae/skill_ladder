#pragma once

#include <iostream>
#include <vector>

#include "eventBus.h"
#include "UpdateEvent.h"
#include "Particle.h"
#include "System.h"

class MovementSystem : public System
{
public:
    MovementSystem(eventBus& bus) : 
        m_bus(bus)
    {
        m_subscription = m_bus.subscribe<UpdateEvent>(
            [this](const UpdateEvent& e)
            {
                onUpdate(e);
            }
        );
    }

    void addParticle(const Particle& p)
    {
        m_particles.push_back(p);
    }


private:
    void onUpdate(const UpdateEvent& e)
    {
        const float gravity = -9.8f;
        const float floorY  = 0.0f;
        const float bounce  = 0.8f;

        for (auto& p : m_particles)
        {
            //applying gravity
            p.vy += gravity * e.deltaTime;

            // integrate velocity
            p.x += p.vx * e.deltaTime;
            p.y += p.vy * e.deltaTime;

            //floor collision
            if (p.y - p.radius < floorY)
            {
                p.y  = floorY + p.radius;
                p.vy = -p.vy * bounce;
            }
        }
        printParticles();
    }

    void printParticles()
    {
        for (std::size_t i=1; i<=m_particles.size(); ++i)
        {
            const auto& p = m_particles[i-1];
            std::cout << "Particle #" << i << ": pos(" << p.x << ", " << p.y
                      << ")  vel(" << p.vx << ", " << p.vy << ")" << std::endl;
        }
        std::cout << "----------" << std::endl;
    }


private:
    eventBus::Subscription m_subscription;
    eventBus&    m_bus;

    std::vector<Particle> m_particles{};
};