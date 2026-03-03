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
