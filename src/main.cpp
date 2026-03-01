
#include <iostream>
#include <thread>
#include <chrono>

#include "eventBus.h"
#include "updateEvent.h"
#include "MovementSystem.h"

int main()
{
    eventBus bus;

    MovementSystem movement(bus);

    for (int i=0; i<5; ++i)
    {
        UpdateEvent uEvent;
        uEvent.deltaTime = 1.0f;  //1 second per frame (pretend)

        bus.publish(uEvent);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

}