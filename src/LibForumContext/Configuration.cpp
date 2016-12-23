#include <atomic>
#include <stdexcept>

#include "Configuration.h"

static std::shared_ptr<const Forum::Configuration::Config> currentConfig =
        std::make_shared<const Forum::Configuration::Config>();

std::shared_ptr<const Forum::Configuration::Config> Forum::Configuration::getGlobalConfig()
{
    return std::atomic_load(&currentConfig);
}

void Forum::Configuration::setGlobalConfig(const Config& value)
{
    std::atomic_store(&currentConfig, std::make_shared<const Config>(value));
}
