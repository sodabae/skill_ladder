#pragma once

#include <iostream>
#include <vector>

#include "eventBus.h"
#include "UpdateEvent.h"
#include "System.h"

class Logger : public System
{
public:    
    Logger(eventBus& bus) : 
        m_bus(bus)
    {

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
        std::cout << "Logger Class: deltaTime = " << e.deltaTime << "\n";
    }

private:
    eventBus& m_bus;
    std::vector<eventBus::Subscription> m_subscriptions;


};
