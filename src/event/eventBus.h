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

            //erase from the iterator to the end of the vector
            //  Note: the remove_if returns an iterator and puts the matching element at the end
            vec.erase(
                //remove_if - puts the item to be erased at the end
                std::remove_if(vec.begin(), vec.end(), [id](const handlerCB &item){return item.id==id;}),
                vec.end() 
            );
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
            for (auto& handler : it->second)
            {
                handler.fn(&event);
            }
        }
    }

private:
    using RawHandler = std::function<void(const void*)>;

    std::unordered_map<std::type_index,
                       std::vector<handlerCB>> m_subscribers;
    
    std::size_t m_nextId{0};
};