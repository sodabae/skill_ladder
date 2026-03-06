#pragma once

#include "IMmioBackend.h"
#include "BackendRegistry.h"

#include <string>
#include <stdexcept>
#include <sstream>
#include <cstring>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <nlohmann/json.hpp>

class MmapBackend : public IMmioBackend
{
public:
    MmapBackend(const std::string& bdf,
                std::size_t bar,
                std::size_t sizeBytes)
        : size_(sizeBytes)
    {

        std::cout << "Constructor from real" << std::endl;

        std::ostringstream path;
        path << "/sys/bus/pci/devices/"
             << bdf
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

    ~MmapBackend() override
    {
        if (mappedBase_ && mappedBase_ != MAP_FAILED)
            ::munmap(mappedBase_, size_);

        if (fd_ >= 0)
            ::close(fd_);
    }

    uint32_t read(uint32_t offset) const override
    {
        checkAccess(offset, 4);

        auto base =
            reinterpret_cast<volatile uint32_t*>(mappedBase_);

        return base[offset / 4];
    }

    void write(uint32_t offset, uint32_t value) override
    {
        checkAccess(offset, 4);

        auto base =
            reinterpret_cast<volatile uint32_t*>(mappedBase_);

        base[offset / 4] = value;
    }

private:
    void checkAccess(uint32_t offset, std::size_t bytes) const
    {
        if (offset % 4 != 0)
            throw std::runtime_error("Unaligned MMIO access");

        if (offset + bytes > size_)
            throw std::runtime_error("MMIO access out of range");
    }

    int fd_ = -1;
    void* mappedBase_ = nullptr;
    std::size_t size_ = 0;
};


/* ---------- Self Registration ---------- */

namespace
{
    const bool mmap_backend_registered = []()
    {
        BackendRegistry::instance().registerBackend(
            "real",
            [](const nlohmann::json& config)
            {
                const auto device =
                    config.at("device").get<std::string>();

                const auto bar =
                    config.value("bar", 0);

                const auto size =
                    config.value("size", 0x1000);

                return std::make_unique<MmapBackend>(
                    device,
                    bar,
                    size);
            });

        return true;
    }();
}
