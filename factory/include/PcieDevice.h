#pragma once

#include <cstdint>
#include <memory>
#include "IMmioBackend.h"

#include "MmapBackend.h"
#include "MockBackend.h"
#include "MockBackend2.h"

class PcieDevice
{
public:
    explicit PcieDevice(std::unique_ptr<IMmioBackend> backend)
        : m_backend(std::move(backend))
    {
        if (!m_backend)
            throw std::invalid_argument("Backend cannot be null");
    }

    uint32_t readRegister(uint32_t offset) const
    {
        return m_backend->read(offset);
    }

    void writeRegister(uint32_t offset, uint32_t value)
    {
        m_backend->write(offset, value);
    }

    void burstRead(uint32_t startOffset,
                   uint32_t count,
                   uint32_t* buffer) const
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            buffer[i] = m_backend->read(startOffset + i);
        }
    }

private:
    std::unique_ptr<IMmioBackend> m_backend;
};
