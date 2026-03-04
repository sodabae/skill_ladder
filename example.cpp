#include <cstdint>
#include <iostream>
#include <thread>
#include <chrono>
#include <array>
#include <stdexcept>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

class PcieDevice
{
public:
    explicit PcieDevice(const std::string& bdf)
    {
        mapBar(bdf);
    }

    ~PcieDevice()
    {
        if (m_bar && m_bar != MAP_FAILED)
        {
            munmap(m_bar, m_barSize);
        }
    }

    uint32_t readRegister(uint32_t offset)
    {
        return m_regs[offset];
    }

    void writeRegister(uint32_t offset, uint32_t value)
    {
        m_regs[offset] = value;
    }

    void burstRead(uint32_t startOffset,
                   uint32_t count,
                   uint32_t* buffer)
    {
        volatile uint32_t* src = m_regs + startOffset;

        for (uint32_t i = 0; i < count; ++i)
        {
            buffer[i] = src[i];
        }
    }

private:
    void* m_bar = nullptr;
    size_t m_barSize = 0x1000;  // Adjust based on actual BAR size
    volatile uint32_t* m_regs = nullptr;

    void mapBar(const std::string& bdf)
    {
        std::string path =
            "/sys/bus/pci/devices/" + bdf + "/resource0";

        int fd = open(path.c_str(), O_RDWR | O_SYNC);
        if (fd < 0)
        {
            throw std::runtime_error("Failed to open PCI resource");
        }

        m_bar = mmap(nullptr,
                     m_barSize,
                     PROT_READ | PROT_WRITE,
                     MAP_SHARED,
                     fd,
                     0);

        close(fd);

        if (m_bar == MAP_FAILED)
        {
            throw std::runtime_error("Failed to mmap BAR");
        }

        m_regs = reinterpret_cast<volatile uint32_t*>(m_bar);
    }
};

int main()
{
    // Example BDF (Bus:Device.Function)
    // You can find it via: lspci
    const std::string BDF = "0000:01:00.0";

    PcieDevice device(BDF);

    constexpr uint32_t READ_REG1 = 0;
    constexpr uint32_t READ_REG2 = 1;
    constexpr uint32_t READ_REG3 = 2;

    constexpr uint32_t BURST_START = 10;
    constexpr uint32_t BURST_COUNT = 100;

    constexpr uint32_t WRITE_REG = 200;
    constexpr uint32_t WRITE_VALUE = 0xCAFEBABE;

    std::array<uint32_t, BURST_COUNT> burstBuffer{};

    for (int iteration = 0; iteration < 3; ++iteration)
    {
        std::cout << "Iteration " << iteration + 1 << "\n";

        // 1️⃣ Read 3 registers
        uint32_t r1 = device.readRegister(READ_REG1);
        uint32_t r2 = device.readRegister(READ_REG2);
        uint32_t r3 = device.readRegister(READ_REG3);

        std::cout << "Read values: "
                  << r1 << ", "
                  << r2 << ", "
                  << r3 << "\n";

        // 2️⃣ Burst read 100 contiguous registers
        device.burstRead(BURST_START,
                         BURST_COUNT,
                         burstBuffer.data());

        std::cout << "Burst read complete\n";

        // 3️⃣ Write 1 register
        device.writeRegister(WRITE_REG, WRITE_VALUE);

        std::cout << "Write complete\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    return 0;
}




--------------------------------------------------------------
--------------------------------------------------------------
--------------------------------------------------------------
RAII
--------------------------------------------------------------

#include <unistd.h>
#include <stdexcept>

class FileDescriptor
{
public:
    explicit FileDescriptor(int fd = -1)
        : m_fd(fd)
    {
    }

    ~FileDescriptor()
    {
        if (m_fd >= 0)
        {
            ::close(m_fd);
        }
    }

    // Non-copyable
    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    // Movable
    FileDescriptor(FileDescriptor&& other) noexcept
        : m_fd(other.m_fd)
    {
        other.m_fd = -1;
    }

    FileDescriptor& operator=(FileDescriptor&& other) noexcept
    {
        if (this != &other)
        {
            if (m_fd >= 0)
                ::close(m_fd);

            m_fd = other.m_fd;
            other.m_fd = -1;
        }
        return *this;
    }

    int get() const { return m_fd; }

    explicit operator bool() const
    {
        return m_fd >= 0;
    }

private:
    int m_fd;
};


//--------------

#include <sys/mman.h>

class MemoryMapping
{
public:
    MemoryMapping() = default;

    MemoryMapping(void* addr, size_t size)
        : m_addr(addr), m_size(size)
    {
    }

    ~MemoryMapping()
    {
        if (m_addr && m_addr != MAP_FAILED)
        {
            ::munmap(m_addr, m_size);
        }
    }

    // Non-copyable
    MemoryMapping(const MemoryMapping&) = delete;
    MemoryMapping& operator=(const MemoryMapping&) = delete;

    // Movable
    MemoryMapping(MemoryMapping&& other) noexcept
        : m_addr(other.m_addr), m_size(other.m_size)
    {
        other.m_addr = nullptr;
        other.m_size = 0;
    }

    MemoryMapping& operator=(MemoryMapping&& other) noexcept
    {
        if (this != &other)
        {
            if (m_addr && m_addr != MAP_FAILED)
                ::munmap(m_addr, m_size);

            m_addr = other.m_addr;
            m_size = other.m_size;

            other.m_addr = nullptr;
            other.m_size = 0;
        }
        return *this;
    }

    void* get() const { return m_addr; }

private:
    void*  m_addr = nullptr;
    size_t m_size = 0;
};

//---------


#include <cstdint>
#include <string>
#include <array>
#include <iostream>
#include <thread>
#include <chrono>
#include <fcntl.h>

class PcieDevice
{
public:
    explicit PcieDevice(const std::string& bdf)
    {
        mapBar(bdf);
    }

    uint32_t readRegister(uint32_t offset) const
    {
        return m_regs[offset];
    }

    void writeRegister(uint32_t offset, uint32_t value)
    {
        m_regs[offset] = value;
    }

    void burstRead(uint32_t startOffset,
                   uint32_t count,
                   uint32_t* buffer) const
    {
        volatile uint32_t* src = m_regs + startOffset;

        for (uint32_t i = 0; i < count; ++i)
        {
            buffer[i] = src[i];
        }
    }

private:
    static constexpr size_t BAR_SIZE = 0x1000;

    volatile uint32_t* m_regs = nullptr;
    MemoryMapping      m_mapping;

    void mapBar(const std::string& bdf)
    {
        std::string path =
            "/sys/bus/pci/devices/" + bdf + "/resource0";

        int rawFd = ::open(path.c_str(), O_RDWR | O_SYNC);
        if (rawFd < 0)
            throw std::runtime_error("Failed to open PCI resource");

        FileDescriptor fd(rawFd);

        void* addr = ::mmap(nullptr,
                            BAR_SIZE,
                            PROT_READ | PROT_WRITE,
                            MAP_SHARED,
                            fd.get(),
                            0);

        if (addr == MAP_FAILED)
            throw std::runtime_error("Failed to mmap BAR");

        m_mapping = MemoryMapping(addr, BAR_SIZE);
        m_regs = reinterpret_cast<volatile uint32_t*>(addr);
    }
};


--------------------------------------------------------------
--------------------------------------------------------------
--------------------------------------------------------------
--------------------------------------------------------------
gtest
--------------------------------------------------------------


#pragma once
#include <cstdint>

class IMmioBackend
{
public:
    virtual ~IMmioBackend() = default;

    virtual uint32_t read(uint32_t offset) const = 0;
    virtual void write(uint32_t offset, uint32_t value) = 0;
};


//--------------


#pragma once
#include <cstdint>

class PcieDevice
{
public:
    explicit PcieDevice(IMmioBackend& backend)
        : m_backend(backend)
    {
    }

    uint32_t readRegister(uint32_t offset) const
    {
        return m_backend.read(offset);
    }

    void writeRegister(uint32_t offset, uint32_t value)
    {
        m_backend.write(offset, value);
    }

    void burstRead(uint32_t startOffset,
                   uint32_t count,
                   uint32_t* buffer) const
    {
        for (uint32_t i = 0; i < count; ++i)
        {
            buffer[i] = m_backend.read(startOffset + i);
        }
    }

private:
    IMmioBackend& m_backend;
};



//---------


#include <stdexcept>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

class MmapBackend : public IMmioBackend
{
public:
    explicit MmapBackend(const std::string& bdf)
    {
        std::string path =
            "/sys/bus/pci/devices/" + bdf + "/resource0";

        int fd = ::open(path.c_str(), O_RDWR | O_SYNC);
        if (fd < 0)
            throw std::runtime_error("Failed to open PCI resource");

        m_size = 0x1000;

        m_addr = ::mmap(nullptr,
                        m_size,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED,
                        fd,
                        0);

        ::close(fd);

        if (m_addr == MAP_FAILED)
            throw std::runtime_error("mmap failed");

        m_regs = reinterpret_cast<volatile uint32_t*>(m_addr);
    }

    ~MmapBackend()
    {
        if (m_addr && m_addr != MAP_FAILED)
            ::munmap(m_addr, m_size);
    }

    uint32_t read(uint32_t offset) const override
    {
        return m_regs[offset];
    }

    void write(uint32_t offset, uint32_t value) override
    {
        m_regs[offset] = value;
    }

private:
    void* m_addr = nullptr;
    size_t m_size = 0;
    volatile uint32_t* m_regs = nullptr;
};


//---------


#include <vector>

class MockBackend : public IMmioBackend
{
public:
    explicit MockBackend(size_t registerCount)
        : m_registers(registerCount, 0)
    {
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


//----------

#include <gtest/gtest.h>

TEST(PcieDeviceTest, ReadsSingleRegister)
{
    MockBackend backend(256);
    backend.write(5, 42);

    PcieDevice device(backend);

    EXPECT_EQ(device.readRegister(5), 42);
}

TEST(PcieDeviceTest, WritesRegister)
{
    MockBackend backend(256);
    PcieDevice device(backend);

    device.writeRegister(10, 99);

    EXPECT_EQ(backend.read(10), 99);
}

TEST(PcieDeviceTest, BurstReadReadsContiguousRegisters)
{
    MockBackend backend(256);

    for (int i = 0; i < 100; ++i)
    {
        backend.write(10 + i, i);
    }

    PcieDevice device(backend);

    uint32_t buffer[100] = {};

    device.burstRead(10, 100, buffer);

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(buffer[i], i);
    }
}


//----------
//real main.cpp

#include <iostream>
#include <array>
#include <thread>
#include <chrono>

#include "IMmioBackend.hpp"
#include "PcieDevice.hpp"
#include "MmapBackend.hpp"

int main()
{
    try
    {
        // Bus:Device.Function
        const std::string BDF = "0000:01:00.0";

        MmapBackend backend(BDF);
        PcieDevice device(backend);

        constexpr uint32_t READ_REG1 = 0;
        constexpr uint32_t READ_REG2 = 1;
        constexpr uint32_t READ_REG3 = 2;

        constexpr uint32_t BURST_START = 10;
        constexpr uint32_t BURST_COUNT = 100;

        constexpr uint32_t WRITE_REG = 200;
        constexpr uint32_t WRITE_VALUE = 0xCAFEBABE;

        std::array<uint32_t, BURST_COUNT> burstBuffer{};

        for (int iteration = 0; iteration < 3; ++iteration)
        {
            std::cout << "Iteration " << iteration + 1 << "\n";

            //  Read 3 registers
            uint32_t r1 = device.readRegister(READ_REG1);
            uint32_t r2 = device.readRegister(READ_REG2);
            uint32_t r3 = device.readRegister(READ_REG3);

            std::cout << "Read values: "
                      << r1 << ", "
                      << r2 << ", "
                      << r3 << "\n";

            // Burst read
            device.burstRead(BURST_START,
                             BURST_COUNT,
                             burstBuffer.data());

            std::cout << "Burst read complete\n";

            // Write register
            device.writeRegister(WRITE_REG, WRITE_VALUE);

            std::cout << "Write complete\n";

            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }

        std::cout << "Finished successfully.\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}


//----------
//test main.cpp


#include <iostream>
#include <array>
#include <thread>
#include <chrono>

#include "PcieDevice.hpp"
#include "MockBackend.hpp"

int main()
{
    MockBackend backend(512);

    // Preload some fake register values
    backend.write(0, 11);
    backend.write(1, 22);
    backend.write(2, 33);

    for (int i = 0; i < 100; ++i)
        backend.write(10 + i, i);

    PcieDevice device(backend);

    constexpr uint32_t BURST_COUNT = 100;
    std::array<uint32_t, BURST_COUNT> buffer{};

    for (int iteration = 0; iteration < 3; ++iteration)
    {
        std::cout << "Iteration " << iteration + 1 << "\n";

        std::cout << device.readRegister(0) << ", "
                  << device.readRegister(1) << ", "
                  << device.readRegister(2) << "\n";

        device.burstRead(10, 100, buffer.data());

        device.writeRegister(200, 0xCAFEBABE);

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    return 0;
}


------------------------------------------------------------
------------------------------------------------------------
------------------------------------------------------------
------------------------------------------------------------
------------------------------------------------------------
------------------------------------------------------------
------------------------------------------------------------

pcie_app/
│
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── IMmioBackend.hpp
│   ├── PcieDevice.hpp
│   ├── PcieDevice.cpp
│   ├── MmapBackend.hpp
│   ├── MmapBackend.cpp
│   ├── MockBackend.hpp
│
└── tests/
    ├── CMakeLists.txt
    └── PcieDeviceTest.cpp


cmake_minimum_required(VERSION 3.16)

project(PcieApp
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------
# Core library (hardware-independent)
# ---------------------------------------
add_library(pcie_core
    src/PcieDevice.cpp
)

target_include_directories(pcie_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ---------------------------------------
# Hardware backend (Linux mmap)
# ---------------------------------------
add_library(pcie_hw
    src/MmapBackend.cpp
)

target_include_directories(pcie_hw
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(pcie_hw
    PUBLIC
        pcie_core
)

# ---------------------------------------
# Application executable
# ---------------------------------------
add_executable(pcie_app
    src/main.cpp
)

target_link_libraries(pcie_app
    PRIVATE
        pcie_core
        pcie_hw
)

# ---------------------------------------
# Enable testing
# ---------------------------------------
enable_testing()
add_subdirectory(tests)




--------------------------------------------------------------
tests/Cmake
--------------------------------------------------------------
# Fetch GoogleTest automatically
include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

# Prevent GoogleTest from overriding compiler options
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_executable(pcie_tests
    PcieDeviceTest.cpp
)

target_link_libraries(pcie_tests
    PRIVATE
        pcie_core
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(pcie_tests)



---------------------------------------------------------
tests/PcieDeviceTest.cpp
---------------------------------------------------------

#include <gtest/gtest.h>
#include "PcieDevice.hpp"
#include "MockBackend.hpp"

TEST(PcieDeviceTest, BurstReadWorks)
{
    MockBackend backend(256);

    for (int i = 0; i < 100; ++i)
        backend.write(10 + i, i);

    PcieDevice device(backend);

    uint32_t buffer[100] = {};

    device.burstRead(10, 100, buffer);

    for (int i = 0; i < 100; ++i)
        EXPECT_EQ(buffer[i], i);
}

----------------------------------------------------------------
----------------------------------------------------------------
----------------------------------------------------------------
----------------------------------------------------------------
----------------------------------------------------------------


{
  "backend_type": "mock",
  "register_space_size": 512,
  "burst_start_offset": 10,
  "burst_count": 100,
  "write_offset": 200,
  "bdf": "0000:01:00.0"
}

//---------------
// main.cpp

#include <iostream>
#include <fstream>
#include <memory>
#include <thread>
#include <chrono>
#include <array>

#include <nlohmann/json.hpp>

#include "PcieDevice.hpp"
#include "MmapBackend.hpp"
#include "MockBackend.hpp"

using json = nlohmann::json;

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: ./pcie_app config.json\n";
        return 1;
    }

    std::ifstream configFile(argv[1]);
    if (!configFile)
    {
        std::cerr << "Failed to open config file\n";
        return 1;
    }

    json config;
    configFile >> config;

    std::string backendType = config["backend_type"];
    uint32_t registerSpaceSize = config["register_space_size"];
    uint32_t burstStart = config["burst_start_offset"];
    uint32_t burstCount = config["burst_count"];
    uint32_t writeOffset = config["write_offset"];
    std::string bdf = config["bdf"];

    std::unique_ptr<IMmioBackend> backend;

    if (backendType == "mock")
    {
        backend = std::make_unique<MockBackend>(registerSpaceSize);

        // Optional: preload fake values
        for (uint32_t i = 0; i < burstCount; ++i)
        {
            backend->write(burstStart + i, i);
        }
    }
    else if (backendType == "hardware")
    {
        backend = std::make_unique<MmapBackend>(bdf);
    }
    else
    {
        std::cerr << "Invalid backend_type\n";
        return 1;
    }

    PcieDevice device(std::move(backend));

    std::array<uint32_t, 3> singleReads{};
    std::vector<uint32_t> burstBuffer(burstCount);

    for (int iteration = 0; iteration < 3; ++iteration)
    {
        std::cout << "Iteration " << iteration + 1 << "\n";

        singleReads[0] = device.readRegister(0);
        singleReads[1] = device.readRegister(1);
        singleReads[2] = device.readRegister(2);

        std::cout << "Single reads: "
                  << singleReads[0] << ", "
                  << singleReads[1] << ", "
                  << singleReads[2] << "\n";

        device.burstRead(burstStart, burstCount, burstBuffer.data());

        std::cout << "Burst read complete\n";

        device.writeRegister(writeOffset, 0xCAFEBABE);

        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    std::cout << "Done.\n";
    return 0;
}


----------------------------------------------------------------
----------------------------------------------------------------
----------------------------------------------------------------
----------------------------------------------------------------
//factory
----------------------------------------------------------------

#pragma once

#include <memory>
#include <string>
#include <stdexcept>

#include "IMmioBackend.h"
#include "MmapBackend.h"
#include "MockBackend.h"

class BackendFactory
{
public:
    enum class Type
    {
        Real,
        Mock
    };

    static std::unique_ptr<IMmioBackend>
    create(Type type,
           const std::string& deviceId,
           std::size_t bar,
           std::size_t size)
    {
        switch (type)
        {
            case Type::Real:
                return std::make_unique<MmapBackend>(deviceId, bar, size);

            case Type::Mock:
                return std::make_unique<MockBackend>(size);

            default:
                throw std::runtime_error("Unknown backend type");
        }
    }

    static Type fromString(const std::string& typeStr)
    {
        if (typeStr == "real")
            return Type::Real;

        if (typeStr == "mock")
            return Type::Mock;

        throw std::runtime_error("Invalid backend type string: " + typeStr);
    }
};


//-----------------

{
  "backend": {
    "type": "real",
    "device": "0000:01:00.0",
    "bar": 0,
    "size": 4096
  },
  "access": {
    "offset": 0,
    "burst_count": 100
  }
}
//-----------------
//main.cpp

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "BackendFactory.h"
#include "PcieDevice.h"

int main()
{
    std::ifstream file("config.json");
    nlohmann::json config;
    file >> config;

    const auto& backendConfig = config["backend"];

    auto type = BackendFactory::fromString(
        backendConfig["type"].get<std::string>());

    std::string deviceId = backendConfig["device"];
    std::size_t bar = backendConfig["bar"];
    std::size_t size = backendConfig["size"];

    auto backend = BackendFactory::create(type, deviceId, bar, size);

    PcieDevice device(std::move(backend));

    device.performAccessSequence();

    return 0;
}


----------------------------------------------------------------
----------------------------------------------------------------
----------------------------------------------------------------
----------------------------------------------------------------
//self backend classes
----------------------------------------------------------------

#pragma once

#include <memory>
#include <functional>
#include <unordered_map>
#include <string>
#include <stdexcept>

#include "IMmioBackend.h"
#include <nlohmann/json.hpp>

class BackendRegistry
{
public:
    using Creator =
        std::function<std::unique_ptr<IMmioBackend>(const nlohmann::json&)>;

    static BackendRegistry& instance()
    {
        static BackendRegistry registry;
        return registry;
    }

    void registerBackend(const std::string& name, Creator creator)
    {
        creators_[name] = std::move(creator);
    }

    std::unique_ptr<IMmioBackend>
    create(const std::string& name,
           const nlohmann::json& config) const
    {
        auto it = creators_.find(name);
        if (it == creators_.end())
            throw std::runtime_error("Unknown backend: " + name);

        return it->second(config);
    }

private:
    std::unordered_map<std::string, Creator> creators_;
};


//--------------
// MmapBackend.cpp
#include "BackendRegistry.h"
#include "MmapBackend.h"

namespace
{
    const bool registered = []()
    {
        BackendRegistry::instance().registerBackend(
            "real",
            [](const nlohmann::json& config)
            {
                return std::make_unique<MmapBackend>(
                    config["device"],
                    config["bar"],
                    config["size"]);
            });

        return true;
    }();
}


//--------------
// MockBackend.cpp

namespace
{
    const bool registered = []()
    {
        BackendRegistry::instance().registerBackend(
            "mock",
            [](const nlohmann::json& config)
            {
                return std::make_unique<MockBackend>(
                    config["size"]);
            });

        return true;
    }();
}


//--------------
//main.cpp

int main()
{
    std::ifstream file("config.json");
    nlohmann::json config;
    file >> config;

    const auto& backendCfg = config["backend"];

    auto backend =
        BackendRegistry::instance().create(
            backendCfg["type"],
            backendCfg);

    PcieDevice device(std::move(backend));

    device.performAccessSequence();

    return 0;
}


----------------------------------------------------------------
----------------------------------------------------------------
----------------------------------------------------------------
----------------------------------------------------------------
//Full code...
----------------------------------------------------------------
class IMmioBackend
{
public:
    virtual ~IMmioBackend() = default;

    virtual uint32_t read32(std::size_t offset) = 0;
    virtual void write32(std::size_t offset, uint32_t value) = 0;
    virtual void burstRead32(std::size_t offset,
                             uint32_t* buffer,
                             std::size_t count) = 0;
};

//------------
// MockBackend.h

#pragma once

#include "IMmioBackend.h"
#include <vector>
#include <cstddef>
#include <cstdint>

class MockBackend : public IMmioBackend
{
public:
    explicit MockBackend(std::size_t sizeBytes);

    uint32_t read32(std::size_t offset) override;
    void write32(std::size_t offset, uint32_t value) override;
    void burstRead32(std::size_t offset,
                     uint32_t* buffer,
                     std::size_t count) override;

private:
    std::vector<uint32_t> memory_;
};


//------------
// MockBackend.cpp

#include "MockBackend.h"
#include "BackendRegistry.h"

#include <stdexcept>
#include <cstring>

MockBackend::MockBackend(std::size_t sizeBytes)
{
    if (sizeBytes % 4 != 0)
        throw std::runtime_error("MockBackend size must be multiple of 4");

    memory_.resize(sizeBytes / 4, 0);
}

uint32_t MockBackend::read32(std::size_t offset)
{
    if (offset % 4 != 0)
        throw std::runtime_error("Unaligned read32");

    std::size_t index = offset / 4;

    if (index >= memory_.size())
        throw std::runtime_error("Read out of range");

    return memory_[index];
}

void MockBackend::write32(std::size_t offset, uint32_t value)
{
    if (offset % 4 != 0)
        throw std::runtime_error("Unaligned write32");

    std::size_t index = offset / 4;

    if (index >= memory_.size())
        throw std::runtime_error("Write out of range");

    memory_[index] = value;
}

void MockBackend::burstRead32(std::size_t offset,
                              uint32_t* buffer,
                              std::size_t count)
{
    if (offset % 4 != 0)
        throw std::runtime_error("Unaligned burstRead32");

    std::size_t index = offset / 4;

    if (index + count > memory_.size())
        throw std::runtime_error("Burst read out of range");

    std::memcpy(buffer,
                &memory_[index],
                count * sizeof(uint32_t));
}

/* ---------- Self Registration ---------- */

namespace
{
    const bool registered = []()
    {
        BackendRegistry::instance().registerBackend(
            "mock",
            [](const nlohmann::json& config)
            {
                return std::make_unique<MockBackend>(
                    config["size"]);
            });

        return true;
    }();
}

//-------------------
// MmapBackend.h

#pragma once

#include "IMmioBackend.h"
#include <string>
#include <cstddef>
#include <cstdint>

class MmapBackend : public IMmioBackend
{
public:
    MmapBackend(const std::string& deviceId,
                std::size_t bar,
                std::size_t sizeBytes);

    ~MmapBackend();

    uint32_t read32(std::size_t offset) override;
    void write32(std::size_t offset, uint32_t value) override;
    void burstRead32(std::size_t offset,
                     uint32_t* buffer,
                     std::size_t count) override;

private:
    int fd_ = -1;
    void* mappedBase_ = nullptr;
    std::size_t size_;
};

//-----------
//MmapBackend.cpp

#include "MmapBackend.h"
#include "BackendRegistry.h"

#include <stdexcept>
#include <sstream>
#include <cstring>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

MmapBackend::MmapBackend(const std::string& deviceId,
                         std::size_t bar,
                         std::size_t sizeBytes)
    : size_(sizeBytes)
{
    std::ostringstream path;
    path << "/sys/bus/pci/devices/"
         << deviceId
         << "/resource"
         << bar;

    fd_ = ::open(path.str().c_str(), O_RDWR | O_SYNC);
    if (fd_ < 0)
        throw std::runtime_error("Failed to open PCI resource");

    mappedBase_ = ::mmap(nullptr,
                         size_,
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED,
                         fd_,
                         0);

    if (mappedBase_ == MAP_FAILED)
    {
        ::close(fd_);
        throw std::runtime_error("mmap failed");
    }
}

MmapBackend::~MmapBackend()
{
    if (mappedBase_ && mappedBase_ != MAP_FAILED)
        ::munmap(mappedBase_, size_);

    if (fd_ >= 0)
        ::close(fd_);
}

uint32_t MmapBackend::read32(std::size_t offset)
{
    if (offset % 4 != 0)
        throw std::runtime_error("Unaligned read32");

    if (offset >= size_)
        throw std::runtime_error("Read out of range");

    volatile uint32_t* base =
        reinterpret_cast<volatile uint32_t*>(mappedBase_);

    return base[offset / 4];
}

void MmapBackend::write32(std::size_t offset, uint32_t value)
{
    if (offset % 4 != 0)
        throw std::runtime_error("Unaligned write32");

    if (offset >= size_)
        throw std::runtime_error("Write out of range");

    volatile uint32_t* base =
        reinterpret_cast<volatile uint32_t*>(mappedBase_);

    base[offset / 4] = value;
}

void MmapBackend::burstRead32(std::size_t offset,
                              uint32_t* buffer,
                              std::size_t count)
{
    if (offset % 4 != 0)
        throw std::runtime_error("Unaligned burstRead32");

    if (offset + count * 4 > size_)
        throw std::runtime_error("Burst read out of range");

    volatile uint32_t* base =
        reinterpret_cast<volatile uint32_t*>(mappedBase_);

    std::memcpy(buffer,
                const_cast<uint32_t*>(&base[offset / 4]),
                count * sizeof(uint32_t));
}

/* ---------- Self Registration ---------- */

namespace
{
    const bool registered = []()
    {
        BackendRegistry::instance().registerBackend(
            "real",
            [](const nlohmann::json& config)
            {
                return std::make_unique<MmapBackend>(
                    config["device"],
                    config["bar"],
                    config["size"]);
            });

        return true;
    }();
}



----------------------------------------------------------------
----------------------------------------------------------------
----------------------------------------------------------------
----------------------------------------------------------------
gtest
----------------------------------------------------------------

//mock_backend

#include <gtest/gtest.h>
#include "MockBackend.h"

TEST(MockBackendTest, ReadWrite32)
{
    MockBackend backend(1024); // 1 KB

    backend.write32(0, 0x12345678);
    uint32_t value = backend.read32(0);

    EXPECT_EQ(value, 0x12345678);
}

TEST(MockBackendTest, BurstRead32)
{
    MockBackend backend(1024);

    for (int i = 0; i < 100; ++i)
        backend.write32(i * 4, i);

    uint32_t buffer[100] = {};

    backend.burstRead32(0, buffer, 100);

    for (int i = 0; i < 100; ++i)
        EXPECT_EQ(buffer[i], static_cast<uint32_t>(i));
}

TEST(MockBackendTest, OutOfRangeThrows)
{
    MockBackend backend(16);

    EXPECT_THROW(backend.read32(32), std::runtime_error);
    EXPECT_THROW(backend.write32(32, 0), std::runtime_error);
}



//--------------
//registry

#include <gtest/gtest.h>
#include "BackendRegistry.h"
#include "MockBackend.h"
#include <nlohmann/json.hpp>

TEST(BackendRegistryTest, CreateMockBackend)
{
    nlohmann::json config = {
        {"type", "mock"},
        {"size", 1024}
    };

    auto backend =
        BackendRegistry::instance().create("mock", config);

    ASSERT_NE(backend, nullptr);

    backend->write32(0, 42);
    EXPECT_EQ(backend->read32(0), 42);
}

TEST(BackendRegistryTest, UnknownBackendThrows)
{
    nlohmann::json config;

    EXPECT_THROW(
        BackendRegistry::instance().create("does_not_exist", config),
        std::runtime_error);
}


//------------------
// pcieDevice

//assuming....
void PcieDevice::performAccessSequence()
{
    backend_->read32(0);
    backend_->read32(4);
    backend_->read32(8);

    uint32_t buffer[100];
    backend_->burstRead32(12, buffer, 100);

    backend_->write32(412, 0xDEADBEEF);
}

//test
TEST(PcieDeviceTest, AccessSequenceWritesCorrectValue)
{
    auto mock = std::make_unique<MockBackend>(2048);
    MockBackend* raw = mock.get();

    PcieDevice device(std::move(mock));

    device.performAccessSequence();

    EXPECT_EQ(raw->read32(412), 0xDEADBEEF);
}

//---------------------------
//real mock for pcieDevice functionality above

#include <gmock/gmock.h>

class FakeBackend : public IMmioBackend
{
public:
    MOCK_METHOD(uint32_t, read32, (std::size_t), (override));
    MOCK_METHOD(void, write32, (std::size_t, uint32_t), (override));
    MOCK_METHOD(void, burstRead32,
                (std::size_t, uint32_t*, std::size_t),
                (override));
};

//-----------
//test
using ::testing::_;
using ::testing::Return;
using ::testing::Exactly;

TEST(PcieDeviceTest, CallsCorrectSequence)
{
    auto mock = std::make_unique<FakeBackend>();
    FakeBackend* raw = mock.get();

    EXPECT_CALL(*raw, read32(_)).Times(3);
    EXPECT_CALL(*raw, burstRead32(_, _, 100)).Times(1);
    EXPECT_CALL(*raw, write32(_, _)).Times(1);

    PcieDevice device(std::move(mock));
    device.performAccessSequence();
}
