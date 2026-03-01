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

    //###############################################################
    // RAII Subscription Token
    //###############################################################
    class Subscription
    {
    public:
        Subscription() = default;
        Subscription(eventBus*       bus,
                     std::type_index type,
                     std::size_t     id) : 
            m_bus(bus),
            m_type(type), 
            m_id(id)
        {}

        ~Subscription(){unsubscribe();}

        //disable copying (or may try to unsubscribe for every copy)
        Subscription(const Subscription&)            = delete;
        Subscription& operator=(const Subscription&) = delete;

        //move semantics functions
        Subscription(Subscription&& other) noexcept
        {
            moveFrom(other);
        }
        Subscription& operator=(Subscription&& other) noexcept
        {
            if (this != &other)
            {
                unsubscribe();
                moveFrom(other);
            }
            return *this;
        }
    private: 

        void unsubscribe(){
            if (m_bus)
            {
                m_bus->unsubscribeByType(m_type, m_id);
                m_bus = nullptr;
            }
        }

        void moveFrom(Subscription& other)
        {
            m_bus  = other.m_bus;
            m_type = other.m_type;
            m_id   = other.m_id;

            other.m_bus = nullptr;
        }

        eventBus*       m_bus {nullptr};
        std::type_index m_type{typeid(void)};
        std::size_t     m_id  {0};
    };
    //###############################################################


    //create an alias for the call back type
    template<typename EventT>
    using Handler = std::function<void(const EventT&)>;

    
    template<typename EventT>
    Subscription subscribe(Handler<EventT> handler)
    {
        auto wrapper = [handler](const void* eventPtr)
        {
            handler(*static_cast<const EventT*>(eventPtr));
        };
        m_subscribers[typeid(EventT)].push_back(handlerCB{m_nextId, wrapper});
        return Subscription(this, typeid(EventT), m_nextId++);

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
                cleanup(vec);
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
                cleanup(it->second);
            }
        }
    }

    void cleanup(std::vector<handlerCB>& cbs)
    {
        cbs.erase(
            std::remove_if(
                cbs.begin(), 
                cbs.end(),
                [](const handlerCB& h){return h.alive == false;}),
            cbs.end()
        );
    }
private:
    void unsubscribeByType(std::type_index type, std::size_t id)
    {
        auto it = m_subscribers.find(type);
        if (it != m_subscribers.end())
        {
            //mark the subscriber for deletion
            auto& vec = it->second;
            for (auto& item : vec)
            {
                if (item.id == id)
                {
                    item.alive = false;
                }
            }

            if (m_publishDepth == 0)
            {
                cleanup(vec);
            }
        }
    }

private:

    std::unordered_map<std::type_index,
                       std::vector<handlerCB>> m_subscribers;
    
    std::size_t m_nextId{0};
    std::size_t m_publishDepth{0};
};