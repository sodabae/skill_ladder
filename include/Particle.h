#pragma once

#include "math/Vec2.h"

struct Particle
{
    Particle (float m = 1.0f)
        : mass(m), inverseMass(1.0f/m)
    {}
    Vec2 position;
    Vec2 velocity;

    Vec2 force;

    float radius = 0.2f;

    float mass       = 1.0f;
    float inverseMass = 1.0f/mass;    
};