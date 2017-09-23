#pragma once

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>

namespace Forum
{
    namespace Configuration
    {
        struct UserConfig
        {
            int_fast16_t minNameLength = 3;
            int_fast16_t maxNameLength = 20;
            int_fast16_t minInfoLength = 0;
            int_fast16_t maxInfoLength = 1024;
            /**
             * Do not update last seen more frequently than this amount (in seconds)
             */
            int_fast32_t lastSeenUpdatePrecision = 300;
            int_fast32_t maxUsersPerPage = 20;
        };

        struct DiscussionThreadConfig
        {
            int_fast16_t minNameLength = 3;
            int_fast16_t maxNameLength = 128;
            /**
             * Do not store more than this amount of users in the visited since last collection of a discussion thread
             */
            int_fast32_t maxUsersInVisitedSinceLastChange = 1024;
            int_fast32_t maxThreadsPerPage = 25;
        };

        struct DiscussionThreadMessageConfig
        {
            int_fast32_t minContentLength = 5;
            int_fast32_t maxContentLength = 65535;
            int_fast16_t minChangeReasonLength = 0;
            int_fast16_t maxChangeReasonLength = 64;
            int_fast32_t maxMessagesPerPage = 20;
            int_fast16_t minCommentLength = 3;
            int_fast16_t maxCommentLength = 1024;
            int_fast32_t maxMessagesCommentsPerPage = 20;
        };

        struct DiscussionTagConfig
        {
            int_fast16_t minNameLength = 2;
            int_fast16_t maxNameLength = 128;
            int_fast16_t maxUiBlobSize = 10000;
        };

        struct DiscussionCategoryConfig
        {
            int_fast16_t minNameLength = 2;
            int_fast16_t maxNameLength = 128;
            int_fast16_t maxDescriptionLength = 1024;
        };

        struct ServiceConfig
        {
            //changing the following values requires rebooting the application
            int_fast16_t numberOfIOServiceThreads = 4;
            int_fast32_t numberOfReadBuffers = 8192;
            int_fast32_t numberOfWriteBuffers = 8192;
            std::string listenIPAddress = "127.0.0.1";
            uint16_t listenPort = 8081;
            int_fast16_t connectionTimeoutSeconds = 20;
            bool trustIpFromXForwardedFor = false;
            bool disableCommands = false;
        };

        struct LoggingConfig
        {
            std::string settingsFile = "log.settings";
        };

        struct PersistenceConfig
        {
            std::string inputFolder = "";
            std::string outputFolder = "";
            bool validateChecksum = true;
            int_fast32_t createNewOutputFileEverySeconds = 3600 * 24;
        };

        struct Config
        {
            UserConfig user;
            DiscussionThreadConfig discussionThread;
            DiscussionThreadMessageConfig discussionThreadMessage;
            DiscussionTagConfig discussionTag;
            DiscussionCategoryConfig discussionCategory;
            ServiceConfig service;
            LoggingConfig logging;
            PersistenceConfig persistence;
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

        /**
         * Loads the configuration data from a stream and sets it globally.
         * Any exception is thrown up the call stack
         */
        void loadGlobalConfigFromStream(std::ifstream& stream);
    }
}
