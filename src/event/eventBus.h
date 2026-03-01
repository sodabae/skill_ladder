#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <typeindex>
#include <algorithm>

#include "event.h" 

struct handlerCB
{
    std::size_t id;
    std::function<void(const void*)> fn;
    bool alive{true};
};

class eventBus
{
public:
    template<typename EventT>
    using Handler = std::function<void(const EventT&)>;

    
    template<typename EventT>
    [[nodiscard]]size_t subscribe(Handler<EventT> handler)
    {
        auto wrapper = [handler](const void* eventPtr)
        {
            handler(*static_cast<const EventT*>(eventPtr));
        };
        m_subscribers[typeid(EventT)].push_back(handlerCB{m_nextId, wrapper});
        return m_nextId++;

    }

    template<typename EventT>
    void unsubscribe(std::size_t id)
    {
        //get the map iterators of the std::vector from the map
        auto it = m_subscribers.find(typeid(EventT));
        if (it != m_subscribers.end())
        {
            auto& vec = it->second;

            // find the item in the vector that has the id being search for, and set to "not alive"
            for (auto &item : vec)
            {
                if (item.id == id)
                    item.alive = false;
            }


            if (m_publishDepth == 0)
            {
                cleanup<EventT>(vec);
            }
        }
    }

    template<typename EventT>
    void publish(const EventT& event)
    {
        //find the subscribers
        auto it = m_subscribers.find(typeid(EventT));

        //if any are subscribers are found, loop through them
        if (it != m_subscribers.end())
        {        
            ++m_publishDepth;
            for (auto& handler : it->second)
            {
                if (handler.alive)
                {
                    handler.fn(&event);
                }
            }
            --m_publishDepth;

            if(m_publishDepth == 0)
            {
                cleanup<EventT>(it->second);
            }
        }
    }

    template<typename EventT>
    void cleanup(std::vector<handlerCB>& cbs)
    {
        cbs.erase(
            std::remove_if(
                cbs.begin(), 
                cbs.end(),
                [](const handlerCB h){return h.alive == false;}),
            cbs.end()
        );
    }

private:
    using RawHandler = std::function<void(const void*)>;

    std::unordered_map<std::type_index,
                       std::vector<handlerCB>> m_subscribers;
    
    std::size_t m_nextId{0};
    std::size_t m_publishDepth{0};
};