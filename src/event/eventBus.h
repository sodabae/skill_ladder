#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <typeindex>

#include "event.h" 

class eventBus
{
public:
    template<typename EventT>
    using Handler = std::function<void(const EventT&)>;

    template<typename EventT>
    void subscribe(Handler<EventT> handler)
    {
        auto wrapper = [handler](const void* eventPtr)
        {
            handler(*static_cast<const EventT*>(eventPtr));
        };
        m_subscribers[typeid(EventT)].push_back(wrapper);
    }

    template<typename EventT>
    void publish(const EventT& event)
    {
        //find the subscribers
        auto it = m_subscribers.find(typeid(EventT));

        //if any are subscribers are found, loop through them
        if (it != m_subscribers.end())
        {        
            for (auto& handler : it->second)
            {
                handler(&event);
            }
        }
    }

private:
    using RawHandler = std::function<void(const void*)>;

    std::unordered_map<std::type_index,
                       std::vector<RawHandler>> m_subscribers;
};