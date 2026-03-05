#pragma once
#include <cstddef>
#include <cstdint>

class IMmioBackend
{
public:
    virtual ~IMmioBackend() = default;                
    virtual uint32_t read(uint32_t offset) const = 0;
    virtual void write(uint32_t offset, uint32_t value) = 0;
//    virtual void burstRead(std::size_t offset,
//                             uint32_t* buffer,
//                             std::size_t count) = 0;
};
