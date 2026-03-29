#pragma once

#include <iostream>
#include <vector>

#include "eventBus.h"
#include "UpdateEvent.h"
#include "Particle.h"
#include "System.h"
#include "SpatialGrid.h"
#include "CollisionEvent.h"

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

        //-------------------------------------------------
        // 1) Integrate motiion and handle floor collision
        //-------------------------------------------------
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

        }
        //-------------------------------------------------
        // 2) Build the spatial grid
        //-------------------------------------------------
        grid.clear();

        for (auto& p : m_particles)
        {
            grid.insert(&p);
        }

        //-------------------------------------------------
        // 3) Particle-particle collisions using the grid
        //-------------------------------------------------
        //particle collision
        for (auto& p : m_particles)
        {
            auto [cx, cy] = grid.cellCoords(p.position);

            for (int dx=-1; dx<=1; dx++)
            {
                for (int dy=-1; dy<=1; dy++)
                {
                    auto& cell = grid.getCell(cx+dx, cy+dy);

                    for (auto* other : cell)
                    {
                        if(&p >= other)
                            continue;
                        
                        resolveCollision(p, *other);
                    }
                }
            }
        }
      
        for (auto& p : m_particles)
        {
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

    void resolveCollision(Particle& a, Particle& b)
    {

        Vec2 delta = b.position - a.position;

        float dist2 = delta.lengthSquared();
        float radiusSum = a.radius + b.radius;

        if(dist2 >= radiusSum * radiusSum)
            return;

        float distance = std::sqrt(dist2);
        if (0.0f == distance)
            return;

        Vec2 normal = delta/distance;

        float penetration = radiusSum - distance;

        float totalInvMass = a.inverseMass + b.inverseMass;

        if(totalInvMass == 0)
            return;

        Vec2 correction = normal * (penetration/totalInvMass);

        //Seperate particles
        a.position -= correction * a.inverseMass;
        b.position += correction * b.inverseMass;

        // Relative velocity
        Vec2 rv = b.velocity - a.velocity;

        float velAlongNormal = Vec2::dot(rv, normal);

        //Ignore if moving apart
        if (velAlongNormal > 0)
            return;

        float restitution = 0.8f;

        float impulseMag = 
            -(1 + restitution) * velAlongNormal / totalInvMass;

        Vec2 impulse = normal * impulseMag;

        a.velocity -= impulse * a.inverseMass;
        b.velocity += impulse * b.inverseMass;
        
        m_bus.publish(CollisionEvent{&a, &b});
    }


private:
    eventBus::Subscription m_subscription;
    eventBus&    m_bus;

    std::vector<Particle> m_particles{};

    SpatialGrid grid{1.0f};
};