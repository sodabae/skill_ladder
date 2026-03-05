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
    //using Creator = std::function<std::unique_ptr<IMmioBackend>(const nlohmann::json&)>;

    static BackendRegistry& instance()
    {
        static BackendRegistry registry;
        return registry;
    }

    void registerBackend(const std::string& name, std::function<std::unique_ptr<IMmioBackend>(const nlohmann::json&)> creator)
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
    std::unordered_map<std::string, std::function<std::unique_ptr<IMmioBackend>(const nlohmann::json&)>> creators_;
};
