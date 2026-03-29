#pragma once

#include <iostream>
#include <vector>

#include "eventBus.h"
#include "UpdateEvent.h"
#include "System.h"
#include "World.h"

class Logger : public System
{
public:    
    Logger(eventBus& bus, World& world) : 
        m_bus(bus), m_world(world)
    {

    }

    void initialize() override
    {
        subscribeUpdateEvent();
    }

    void subscribeUpdateEvent()
    {
        m_subscriptions.push_back(
            m_bus.subscribe<UpdateEvent>(
                [this](const UpdateEvent& e)
                {
                    onUpdate(e);
                }
            )
        );
    }

private:
    void onUpdate(const UpdateEvent& e)
    {
        std::cout << "Logger Class: deltaTime = " << e.deltaTime << std::endl;
    }

private:
    eventBus& m_bus;
    std::vector<eventBus::Subscription> m_subscriptions;
    World& m_world;

};
