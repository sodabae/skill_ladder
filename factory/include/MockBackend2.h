

#include <vector>
#include <iostream>

#include "BackendRegistry.h"
#include "IMmioBackend.h"


class MockBackend2 : public IMmioBackend
{
public:
    explicit MockBackend2(size_t registerCount)
        : m_registers(registerCount, 0)
    {
        std::cout << "Constructor from Mock2" << std::endl;
    }

    uint32_t read(uint32_t offset) const override
    {
        return m_registers.at(offset);
    }

    void write(uint32_t offset, uint32_t value) override
    {
        m_registers.at(offset) = value;
    }

    std::vector<uint32_t>& raw()
    {
        return m_registers;
    }

private:
    std::vector<uint32_t> m_registers;
};

namespace
{
 const bool registeredMock2 = []()
 {
    BackendRegistry::instance().registerBackend(
        "mock2",
        [](const nlohmann::json config){return std::make_unique<MockBackend2>(200);}
    );
    return true;
 }();
}
