#pragma once

#include <cstdint>
#include <memory>

namespace Forum
{
    namespace Configuration
    {
        struct UserConfig
        {
            uint16_t minNameLength = 3;
            uint16_t maxNameLength = 20;
            /**
             * Do not update last seen more frequently than this amount (in seconds)
             */
            uint32_t lastSeenUpdatePrecision = 5000;
        };

        struct Config
        {
            UserConfig user;
        };

        typedef std::shared_ptr<const Config> ConfigConstRef;

        /**
         * Returns a thread-safe reference to an immutable configuration structure
         */
        ConfigConstRef getGlobalConfig();

        /**
         * Replaces the current configuration structure in a thread-safe manner.
         * References to the old configuration should still be valid and point to the old data.
         * Newer calls to getGlobalConfig() will receive the new configuration.
         */
        void setGlobalConfig(const Config& value);
    }
}
