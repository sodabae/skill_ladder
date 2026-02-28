#pragma once

#include <typeindex>


struct Event
{
    virtual ~Event() = default;
};

using EventType = std::type_index;