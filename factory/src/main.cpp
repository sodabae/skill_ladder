#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "BackendRegistry.h"
#include "PcieDevice.h"

int main()
{
    std::ifstream file("config.json");
    nlohmann::json config;
    file >> config;

    std::cout << "Here" << std::endl;

    const auto& backendCfg = config["backend"];

    std::cout << "Type = " << backendCfg["type"] << std::endl;

    auto backend =
        BackendRegistry::instance().create(
            backendCfg["type"],
            backendCfg);

    PcieDevice device(std::move(backend));

    //device.performAccessSequence();

    return 0;
}
