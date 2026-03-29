#pragma once

#include <chrono>
#include <vector>
#include <memory>
#include <thread>

#include "eventBus.h"
#include "UpdateEvent.h"
#include "System.h"
#include "World.h"


class Simulation
{
public: 

    template <typename T, typename... Args>
    T& addSystem(Args&&... args)
    {
        auto system = std::make_unique<T>(m_bus, m_world, std::forward<Args>(args)...);
        T& ref = *system;

        system->initialize();

        m_systems.push_back(std::move(system));

        return ref;
    }

    void run()
    {
        m_running = true;

        auto previous = std::chrono::steady_clock::now();
        const float fixedDt = 1.0f / 60.0f;   //60Hz
        float accumulator = 0.0f;

        while(m_running)
        {
            auto now = std::chrono::steady_clock::now();

            float dt = std::chrono::duration<float>(now-previous).count();
            previous = now;

            accumulator += dt;

            while (accumulator >= fixedDt)
            {
                UpdateEvent e{fixedDt};
                m_bus.publish(e);
                accumulator -= fixedDt;
            }

            //small sleep to avoid busy-waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
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

    World& getWorld()
    {
        return m_world;
    }


private: 
    bool m_running{false};
    eventBus m_bus;
    World m_world;

    std::vector<std::unique_ptr<System>> m_systems;
};