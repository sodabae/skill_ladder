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
        //const float gravity = -9.8f;
        const float floorY  = 0.0f;
        const float bounce  = 0.8f;
        
        float dt = e.deltaTime;
        Vec2 gravity{0.0f, -9.8f};

        for (auto& p : m_particles)
        {
            //applying gravity as a force
            p.force += gravity * p.mass;

            // convert force -> acceleration
            Vec2 acceleration = p.force * p.inverseMass;

            //Integrate velocity
            p.velocity += acceleration * dt;

            //Integrate position
            p.position += p.velocity * dt;

            //floor collision
            if (p.position.y - p.radius < floorY)
            {
                p.position.y  = floorY + p.radius;
                p.velocity.y = -p.velocity.y * bounce;
            }

            //Clear accumulated forces
            p.force = Vec2{0,0};
        }
        printParticles();
    }

    void printParticles()
    {
        for (std::size_t i=1; i<=m_particles.size(); ++i)
        {
            const auto& p = m_particles[i-1];
            std::cout << "Particle #" << i << ": pos(" << p.position.x << ", " << p.position.y
                      << ")  vel(" << p.velocity.x << ", " << p.velocity.y << ")" << std::endl;
        }
        std::cout << "----------" << std::endl;
    }


private:
    eventBus::Subscription m_subscription;
    eventBus&    m_bus;

    std::vector<Particle> m_particles{};
};