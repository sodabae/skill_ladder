#pragma once

#include <iostream>
#include <vector>

#include "eventBus.h"
#include "UpdateEvent.h"
#include "Particle.h"

class MovementSystem
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
        for (auto& p : m_particles)
        {
            p.x += p.vx * e.deltaTime;
            p.y += p.vy * e.deltaTime;
        }
        printParticles();
    }

    void printParticles()
    {
        for (const auto& p : m_particles)
        {
            std::cout << "Particle: " << p.x << ", " << p.y << "\n";
        }
        std::cout << "----------" << std::endl;
    }


private:
    eventBus::Subscription m_subscription;
    eventBus&    m_bus;

    std::vector<Particle> m_particles{};
};