
#include <iostream>
#include <thread>
#include <chrono>

//#include "eventBus.h"
//#include "UpdateEvent.h"
//#include "MovementSystem.h"
//#include "Logger.h"
//#include "Particle.h"

#include "MovementSystem.h"
#include "Logger.h"
#include "Simulation.h"

int main()
{

    Simulation sim;

    //MovementSystem movement(sim.getBus());
    //Logger logger(sim.getBus());

    auto& movement = sim.addSystem<MovementSystem>();
    [[maybe_unused]] auto& logger   = sim.addSystem<Logger>(); 


    for (int i=0; i<10; ++i)
    {
        Particle p;
        p.position.x  = 0.5f * static_cast<float>(i);
        p.position.y  = 5.0f + static_cast<float>(i);
        p.velocity.x  = 0.5f * static_cast<float>(i);
        p.velocity.y  = 0.0f;

        movement.addParticle(p);
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