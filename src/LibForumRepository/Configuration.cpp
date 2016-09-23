#include <atomic>
#include <stdexcept>

#include "Configuration.h"

static std::shared_ptr<const Forum::Configuration::Config> currentConfig =
        std::make_shared<const Forum::Configuration::Config>();

std::shared_ptr<const Forum::Configuration::Config> Forum::Configuration::getGlobalConfig()
{
    auto result = std::atomic_load(&currentConfig);
    return result;
}

void Forum::Configuration::setGlobalConfig(const Forum::Configuration::Config& value)
{
    std::atomic_store(&currentConfig, std::make_shared<const Forum::Configuration::Config>(value));
}
