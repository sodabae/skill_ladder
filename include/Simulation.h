#pragma once

#include <chrono>
#include <vector>
#include <memory>
#include <thread>

#include "eventBus.h"
#include "UpdateEvent.h"
#include "System.h"


class Simulation
{
public: 

    template <typename T, typename... Args>
    T& addSystem(Args&&... args)
    {
        auto system = std::make_unique<T>(m_bus, std::forward<Args>(args)...);
        T& ref = *system;

        system->initialize();

        m_systems.push_back(std::move(system));

        return ref;
    }

    void run()
    {
        m_running = true;

        auto previous = std::chrono::steady_clock::now();

        while(m_running)
        {
            auto now = std::chrono::steady_clock::now();

            float dt = std::chrono::duration<float>(now-previous).count();
            previous = now;

            UpdateEvent e{dt};
            m_bus.publish(e);

            //add artificial delay
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        shutdownSystems();
    }

    void shutdownSystems()
    {
        for (auto& s : m_systems)
        {
            s->shutdown();
        }
    }

    eventBus& getBus()
    {
        return m_bus;
    }

    void stop()
    {
        m_running = false;
    }


private: 
    bool m_running{false};
    eventBus m_bus;

    std::vector<std::unique_ptr<System>> m_systems;
};