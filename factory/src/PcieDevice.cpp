#include "PcieDevice.h"

/////////////////////////
// Step 1: Define Impl //
/////////////////////////

class PcieDevice::Impl
{
public:
    virtual ~Impl() = default;
    virtual void perform(IMmioBackend& backend) = 0;
};



/////////////////////////////////////////////////////
// Step 2: Define variants in anonymous namespace //
/////////////////////////////////////////////////////

namespace
{
    class VariantA : public PcieDevice::Impl
    {
    public:
        void perform(IMmioBackend& backend) override
        {
            backend.write(0x10, 1);
        }
    };

    class VariantB : public PcieDevice::Impl
    {
    public:
        void perform(IMmioBackend& backend) override
        {
            backend.write(0x20, 2);
        }
    };
}

/////////////////////////////////////////
// Step 3: PcieDevice constructor & destructor
/////////////////////////////////////////

PcieDevice::PcieDevice(const nlohmann::json& cfg, std::unique_ptr<IMmioBackend> backend)
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

PcieDevice::~PcieDevice() = default;

/////////////////////////////////////////
// Step 4: Forward call to variant     //
/////////////////////////////////////////

void PcieDevice::performSequence()
{
    m_impl->perform(*m_backend);
}


uint32_t PcieDevice::readRegister(uint32_t offset) const
{
    return m_backend->read(offset);
}

void PcieDevice::writeRegister(uint32_t offset, uint32_t value)
{
    m_backend->write(offset, value);
}

void PcieDevice::burstRead(uint32_t startOffset,
                uint32_t count,
                uint32_t* buffer) const
{
    for (uint32_t i = 0; i < count; ++i)
    {
        buffer[i] = m_backend->read(startOffset + i);
    }
}
