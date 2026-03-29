
#include <iostream>
#include <thread>
#include <chrono>

//#include "eventBus.h"
//#include "UpdateEvent.h"
//#include "MovementSystem.h"
//#include "Logger.h"
//#include "Particle.h"

//#include "MovementSystem.h"
#include "LoggerUpdate.h"
#include "LoggerCollision.h"
#include "Simulation.h"
#include "PhysicsIntegrationSystem.h"
#include "SpatialPartitionSystem.h"
#include "CollisionDetectionSystem.h"
#include "CollisionResolutionSystem.h"
#include "RenderSystem.h"
#include "World.h"

int main()
{

    Simulation sim;

    [[maybe_unused]] auto& physics = sim.addSystem<PhysicsIntegrationSystem>();
    [[maybe_unused]] auto& spatial = sim.addSystem<SpatialPartitionSystem>(); 
    [[maybe_unused]] auto& detect  = sim.addSystem<CollisionDetectionSystem>(spatial.getGrid());
    [[maybe_unused]] auto& resolve = sim.addSystem<CollisionResolutionSystem>();
    [[maybe_unused]] auto& logger  = sim.addSystem<LoggerUpdate>();
    [[maybe_unused]] auto& logCol  = sim.addSystem<LoggerCollision>();
    [[maybe_unused]] auto& render  = sim.addSystem<RenderSystem>();

    auto& world = sim.getWorld();

    for (int i=0; i<10; ++i)
    {
        Particle p;
        p.position  = {static_cast<float>(i), 5.0f};
        p.velocity  = { (i % 2 == 0 ? 1.0f : -1.0f), 0.0f };
        world.particles.push_back(p);
    }

    sim.run();

//    for (int i=0; i<5; ++i)
//    {
//        UpdateEvent uEvent;
//        uEvent.deltaTime = 1.0f;  //1 second per frame (pretend)
//
//        bus.publish(uEvent);
//        std::this_thread::sleep_for(std::chrono::milliseconds(500));
//    }


}