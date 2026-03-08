
#include <iostream>
#include <thread>
#include <chrono>

#include "eventBus.h"
#include "UpdateEvent.h"
#include "MovementSystem.h"
#include "Logger.h"
#include "Particle.h"

int main()
{
    eventBus bus;

    MovementSystem movement(bus);
    Logger logger(bus);
    logger.subscribeUpdateEvent();

    Particle p1{0, 0, 1.0f, 0.5f};

    movement.addParticle(p1);

    for (int i=0; i<5; ++i)
    {
        UpdateEvent uEvent;
        uEvent.deltaTime = 1.0f;  //1 second per frame (pretend)

        bus.publish(uEvent);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

}