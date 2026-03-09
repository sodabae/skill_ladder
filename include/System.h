
#pragma once

class System
{
public:
    virtual ~System() = default;

    virtual void initialize(){};
    virtual void shutdown  (){};
};