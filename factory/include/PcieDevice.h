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
    struct Registers
    {

        //empty constructor
        Registers() : reg1(0), reg2(2){};

        Registers(uint32_t a, uint32_t b) : 
        reg1(a),
        reg2(b)
        {};

        const uint32_t reg1;
        const uint32_t reg2;
    };

public:
    explicit PcieDevice(const nlohmann::json& cfg, std::unique_ptr<IMmioBackend> backend)
        : m_backend(std::move(backend))
    {
        if (!m_backend)
            throw std::invalid_argument("Backend cannot be null");
            
        if(cfg["type"] == "real")
            m_impl = std::make_unique<VariantA>();
        else if(cfg["type"] == "mock")
            m_impl = std::make_unique<VariantB>();
        else
            throw std::runtime_error("Unknown variant");
    }

    void performSequence()
    {
        m_impl->perform(m_regs, *m_backend);
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
    class Impl
    {
    public:
        virtual ~Impl() = default;
        virtual void perform(const PcieDevice::Registers& myRegs, IMmioBackend& backend) = 0;
    };

    std::unique_ptr<Impl> m_impl;
    std::unique_ptr<IMmioBackend> m_backend;
    Registers m_regs;


private:
    
    class VariantA : public Impl
    {
    public:
        void perform(const PcieDevice::Registers& myRegs, IMmioBackend& backend) override
        {
            backend.write(myRegs.reg1, 1);
        }
    };

    class VariantB : public Impl
    {
    public:
        void perform(const PcieDevice::Registers& myRegs, IMmioBackend& backend) override
        {
            backend.write(myRegs.reg2, 2);
        }
    };
};
