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
    explicit PcieDevice(const nlohmann::json& cfg, std::unique_ptr<IMmioBackend> backend);
    ~PcieDevice();

    void performSequence();

    uint32_t readRegister(uint32_t offset) const;

    void writeRegister(uint32_t offset, uint32_t value);

    void burstRead(uint32_t startOffset,
                   uint32_t count,
                   uint32_t* buffer) const;
class Impl;  // Forward declaration of private nested class
private:
    
    std::unique_ptr<Impl> m_impl;

    std::unique_ptr<IMmioBackend> m_backend;
};
