
#include <iostream>
#include <thread>
#include <chrono>

#include "eventBus.h"
#include "updateEvent.h"
#include "MovementSystem.h"

struct objectMoved
{
    float x;
    float y;
};

struct damageTaken
{
    int amount;
};

int main()
{
    eventBus bus;

    auto oId = bus.subscribe<objectMoved>([](const objectMoved& e)
        {
            std::cout << "Object moved to "
                      << e.x << ", " << e.y << "\n";
        });

    auto dId = bus.subscribe<damageTaken>([](const damageTaken& e)
        {
            std::cout << "Damage: "
                      << e.amount << "\n";
        });

    objectMoved ev{10.f, 20.f};
    //volatile auto* debugAddr = &ev;
    damageTaken dmg{5};
    bus.publish(ev);
    bus.publish(dmg);

    MovementSystem movement(bus);

    for (int i=0; i<5; ++i)
    {
        UpdateEvent uEvent;
        uEvent.deltaTime = 1.0f;  //1 second per frame (pretend)

        bus.publish(uEvent);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    //bus.unsubscribe<objectMoved>(oId);
    //bus.unsubscribe<damageTaken>(dId);
}