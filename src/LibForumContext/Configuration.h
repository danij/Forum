#pragma once

#include <cstdint>
#include <memory>

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
            int_fast32_t serializationPerThreadBufferSize = 1 << 20; //1 MiByte buffer / thread
            int_fast16_t numberOfIOServiceThreads = 4;
        };

        struct Config
        {
            UserConfig user;
            DiscussionThreadConfig discussionThread;
            DiscussionThreadMessageConfig discussionThreadMessage;
            DiscussionTagConfig discussionTag;
            DiscussionCategoryConfig discussionCategory;
            ServiceConfig service;
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
