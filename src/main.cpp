
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

    Particle p1{0, 5, 1.0f, 0.0f};

    printf("x=%0f, y=%0f\nvx=%0f, vy=%0f\n", p1.x, p1.y, p1.vx, p1.vy);

    movement.addParticle(p1);

    for (int i=0; i<5; ++i)
    {
        UpdateEvent uEvent;
        uEvent.deltaTime = 1.0f;  //1 second per frame (pretend)

        bus.publish(uEvent);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

}