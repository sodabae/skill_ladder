#pragma once

#include <iostream>

#include "eventBus.h"
#include "updateEvent.h"

class MovementSystem
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

private:
    void onUpdate(const UpdateEvent& e)
    {
        m_position += m_speed * e.deltaTime;
        std::cout << "Position: " << m_position << "\n";
    }

private:
    eventBus::Subscription m_subscription;
    eventBus&    m_bus;

    float m_speed   {5.f};    //units per second
    float m_position{0.f};
};