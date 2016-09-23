#pragma once

#include <cstdint>
#include <memory>

namespace Forum
{
    namespace Configuration
    {
        struct UserConfig
        {
            uint16_t maxNameLength = 20;
        };

        struct Config
        {
            UserConfig user;
        };

        /**
         * Returns a thread-safe reference to an immutable configuration structure
         */
        std::shared_ptr<const Config> getGlobalConfig();

        /**
         * Replaces the current configuration structure in a thread-safe manner.
         * References to the old configuration should still be valid and point to the old data.
         * Newer calls to getGlobalConfig() will receive the new configuration.
         */
        void setGlobalConfig(const Config& value);
    }
}
