#include "event/eventBus.h"
#include <iostream>

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

    bus.unsubscribe<objectMoved>(oId);
    bus.unsubscribe<damageTaken>(dId);
}