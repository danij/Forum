/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <cstdint>
#include <iosfwd>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <boost/property_tree/ptree.hpp>

namespace Forum::Configuration
{
    struct UserConfig
    {
        int_fast16_t minNameLength = 3;
        int_fast16_t maxNameLength = 20;
        int_fast16_t minInfoLength = 0;
        int_fast16_t maxInfoLength = 1024;
        int_fast16_t minTitleLength = 0;
        int_fast16_t maxTitleLength = 64;
        int_fast16_t minSignatureLength = 0;
        int_fast16_t maxSignatureLength = 256;
        /**
         * Do not update last seen more frequently than this amount (in seconds)
         */
        int_fast32_t lastSeenUpdatePrecision = 300;
        int_fast32_t maxUsersPerPage = 20;
        /**
         * When returning the currently online users, look for users last seen within the specified seconds
         */
        uint_fast32_t onlineUsersIntervalSeconds = 15 * 60;
        uint_fast32_t maxLogoBinarySize = 32768;
        uint_fast32_t maxLogoWidth = 128;
        uint_fast32_t maxLogoHeight = 128;
        int16_t defaultPrivilegeValueForLoggedInUser = 1;
        uint_fast32_t resetVoteExpiresInSeconds = 3600;
        uint_fast32_t visitorOnlineForSeconds = 300;
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

    struct PrivateMessageConfig
    {
        int_fast32_t minContentLength = 5;
        int_fast32_t maxContentLength = 65535;
        int_fast32_t maxMessagesPerPage = 20;
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
    
    struct AttachmentConfig
    {
        int_fast16_t minNameLength = 2;
        int_fast16_t maxNameLength = 128;
        int_fast32_t maxAttachmentsPerPage = 50;
        int_fast64_t defaultUserQuota = 104857600;
    };

    struct ServiceConfig
    {
        //changing the following values requires rebooting the application
        int_fast16_t numberOfIOServiceThreads = 4;
        int_fast32_t numberOfReadBuffers = 512;
        int_fast32_t numberOfWriteBuffers = 512;
        int_fast32_t connectionPoolSize = 100;
        std::string listenIPAddress = "127.0.0.1";
        uint16_t listenPort = 8081;
        std::string authListenIPAddress = "127.0.0.1";
        uint16_t authListenPort = 18081;
        size_t connectionTimeoutSeconds = 20;
        bool trustIpFromXForwardedFor = false;
        bool disableCommands = false;
        bool disableCommandsForAnonymousUsers = false;
        bool disableThrottling = false;
        std::string responsePrefix = "";
        std::string expectedOriginReferer = "";
    };

    struct LoggingConfig
    {
        std::string settingsFile = "log.settings";
    };

    struct PersistenceConfig
    {
        std::string inputFolder = "";
        std::string outputFolder = "";
        std::string messagesFile = "";
        bool validateChecksum = true;
        int_fast32_t createNewOutputFileEverySeconds = 3600 * 24;
        bool persistIPAddresses = false;
    };

    struct PluginEntry
    {
        std::string libraryPath = "";
        boost::property_tree::ptree configuration;
    };

    typedef uint_fast16_t PrivilegeValueType;
    typedef int64_t PrivilegeDurationType;
    constexpr auto DenyPrivilegeValue = std::numeric_limits<PrivilegeValueType>::max();

    struct DefaultPrivilegesConfig
    {
        struct
        {
            PrivilegeValueType view                     = DenyPrivilegeValue;
            PrivilegeValueType viewUnapproved           = DenyPrivilegeValue;
            PrivilegeValueType viewRequiredPrivileges   = DenyPrivilegeValue;
            PrivilegeValueType viewAssignedPrivileges   = DenyPrivilegeValue;
            PrivilegeValueType viewCreatorUser          = DenyPrivilegeValue;
            PrivilegeValueType viewIpAddress            = DenyPrivilegeValue;
            PrivilegeValueType viewVotes                = DenyPrivilegeValue;
            PrivilegeValueType viewAttachment           = DenyPrivilegeValue;
            PrivilegeValueType viewUnapprovedAttachment = DenyPrivilegeValue;
            PrivilegeValueType upVote                   = DenyPrivilegeValue;
            PrivilegeValueType downVote                 = DenyPrivilegeValue;
            PrivilegeValueType resetVote                = DenyPrivilegeValue;
            PrivilegeValueType addComment               = DenyPrivilegeValue;
            PrivilegeValueType setCommentToSolved       = DenyPrivilegeValue;
            PrivilegeValueType getMessageComments       = DenyPrivilegeValue;
            PrivilegeValueType changeContent            = DenyPrivilegeValue;
            PrivilegeValueType changeApproval           = DenyPrivilegeValue;
            PrivilegeValueType deleteThreadMessage      = DenyPrivilegeValue;
            PrivilegeValueType move                     = DenyPrivilegeValue;
            PrivilegeValueType addAttachment            = DenyPrivilegeValue;
            PrivilegeValueType removeAttachment         = DenyPrivilegeValue;
            PrivilegeValueType adjustPrivilege          = DenyPrivilegeValue;
        } threadMessage;

        struct
        {
            PrivilegeValueType view                   = DenyPrivilegeValue;
            PrivilegeValueType viewUnapproved         = DenyPrivilegeValue;
            PrivilegeValueType viewRequiredPrivileges = DenyPrivilegeValue;
            PrivilegeValueType viewAssignedPrivileges = DenyPrivilegeValue;
            PrivilegeValueType getSubscribedUsers     = DenyPrivilegeValue;
            PrivilegeValueType subscribe              = DenyPrivilegeValue;
            PrivilegeValueType unsubscribe            = DenyPrivilegeValue;
            PrivilegeValueType addMessage             = DenyPrivilegeValue;
            PrivilegeValueType autoApproveMessage     = DenyPrivilegeValue;
            PrivilegeValueType changeName             = DenyPrivilegeValue;
            PrivilegeValueType changePinDisplayOrder  = DenyPrivilegeValue;
            PrivilegeValueType changeApproval         = DenyPrivilegeValue;
            PrivilegeValueType addTag                 = DenyPrivilegeValue;
            PrivilegeValueType removeTag              = DenyPrivilegeValue;
            PrivilegeValueType deleteThread           = DenyPrivilegeValue;
            PrivilegeValueType merge                  = DenyPrivilegeValue;
            PrivilegeValueType adjustPrivilege        = DenyPrivilegeValue;
        } thread;

        struct
        {
            PrivilegeValueType view                   = DenyPrivilegeValue;
            PrivilegeValueType viewRequiredPrivileges = DenyPrivilegeValue;
            PrivilegeValueType viewAssignedPrivileges = DenyPrivilegeValue;
            PrivilegeValueType getDiscussionThreads   = DenyPrivilegeValue;
            PrivilegeValueType changeName             = DenyPrivilegeValue;
            PrivilegeValueType changeUiblob           = DenyPrivilegeValue;
            PrivilegeValueType deleteTag              = DenyPrivilegeValue;
            PrivilegeValueType merge                  = DenyPrivilegeValue;
            PrivilegeValueType adjustPrivilege        = DenyPrivilegeValue;
        } tag;

        struct
        {
            PrivilegeValueType view                   = DenyPrivilegeValue;
            PrivilegeValueType viewRequiredPrivileges = DenyPrivilegeValue;
            PrivilegeValueType viewAssignedPrivileges = DenyPrivilegeValue;
            PrivilegeValueType getDiscussionThreads   = DenyPrivilegeValue;
            PrivilegeValueType changeName             = DenyPrivilegeValue;
            PrivilegeValueType changeDescription      = DenyPrivilegeValue;
            PrivilegeValueType changeParent           = DenyPrivilegeValue;
            PrivilegeValueType changeDisplayorder     = DenyPrivilegeValue;
            PrivilegeValueType addTag                 = DenyPrivilegeValue;
            PrivilegeValueType removeTag              = DenyPrivilegeValue;
            PrivilegeValueType deleteCategory         = DenyPrivilegeValue;
            PrivilegeValueType adjustPrivilege        = DenyPrivilegeValue;
        } category;

        struct
        {
            PrivilegeValueType addUser                              = DenyPrivilegeValue;
            PrivilegeValueType getEntitiesCount                     = DenyPrivilegeValue;
            PrivilegeValueType getVersion                           = DenyPrivilegeValue;
            PrivilegeValueType getAllUsers                          = DenyPrivilegeValue;
            PrivilegeValueType getUserInfo                          = DenyPrivilegeValue;
            PrivilegeValueType getDiscussionThreadsOfUser           = DenyPrivilegeValue;
            PrivilegeValueType getDiscussionThreadMessagesOfUser    = DenyPrivilegeValue;
            PrivilegeValueType getSubscribedDiscussionThreadsOfUser = DenyPrivilegeValue;
            PrivilegeValueType getAllDiscussionCategories           = DenyPrivilegeValue;
            PrivilegeValueType getDiscussionCategoriesFromRoot      = DenyPrivilegeValue;
            PrivilegeValueType getAllDiscussionTags                 = DenyPrivilegeValue;
            PrivilegeValueType getAllDiscussionThreads              = DenyPrivilegeValue;
            PrivilegeValueType getAllMessageComments                = DenyPrivilegeValue;
            PrivilegeValueType getMessageCommentsOfUser             = DenyPrivilegeValue;
            PrivilegeValueType addDiscussionCategory                = DenyPrivilegeValue;
            PrivilegeValueType addDiscussionTag                     = DenyPrivilegeValue;
            PrivilegeValueType addDiscussionThread                  = DenyPrivilegeValue;
            PrivilegeValueType autoApproveDiscussionThread          = DenyPrivilegeValue;
            PrivilegeValueType sendPrivateMessage                   = DenyPrivilegeValue;
            PrivilegeValueType viewPrivateMessageIpAddress          = DenyPrivilegeValue;
            PrivilegeValueType changeOwnUserName                    = DenyPrivilegeValue;
            PrivilegeValueType changeOwnUserInfo                    = DenyPrivilegeValue;
            PrivilegeValueType changeAnyUserName                    = DenyPrivilegeValue;
            PrivilegeValueType changeAnyUserInfo                    = DenyPrivilegeValue;
            PrivilegeValueType deleteAnyUser                        = DenyPrivilegeValue;
            PrivilegeValueType viewForumWideRequiredPrivileges      = DenyPrivilegeValue;
            PrivilegeValueType viewForumWideAssignedPrivileges      = DenyPrivilegeValue;
            PrivilegeValueType viewUserAssignedPrivileges           = DenyPrivilegeValue;
            PrivilegeValueType adjustForumWidePrivilege             = DenyPrivilegeValue;
            PrivilegeValueType changeOwnUserTitle                   = DenyPrivilegeValue;
            PrivilegeValueType changeAnyUserTitle                   = DenyPrivilegeValue;
            PrivilegeValueType changeOwnUserSignature               = DenyPrivilegeValue;
            PrivilegeValueType changeAnyUserSignature               = DenyPrivilegeValue;
            PrivilegeValueType changeOwnUserLogo                    = DenyPrivilegeValue;
            PrivilegeValueType changeAnyUserLogo                    = DenyPrivilegeValue;
            PrivilegeValueType deleteOwnUserLogo                    = DenyPrivilegeValue;
            PrivilegeValueType deleteAnyUserLogo                    = DenyPrivilegeValue;
            PrivilegeValueType changeAnyUserAttachmentQuota         = DenyPrivilegeValue;
            PrivilegeValueType getAllAttachments                    = DenyPrivilegeValue;
            PrivilegeValueType getAttachmentsOfUser                 = DenyPrivilegeValue;
            PrivilegeValueType viewAttachmentIpAddress              = DenyPrivilegeValue;
            PrivilegeValueType createAttachment                     = DenyPrivilegeValue;
            PrivilegeValueType autoApproveAttachment                = DenyPrivilegeValue;
            PrivilegeValueType changeAnyAttachmentName              = DenyPrivilegeValue;
            PrivilegeValueType changeAnyAttachmentApproval          = DenyPrivilegeValue;
            PrivilegeValueType deleteAttachment                     = DenyPrivilegeValue;
            PrivilegeValueType noThrottling                         = DenyPrivilegeValue;
        } forumWide;
    };

    struct DefaultPrivilegeDurationConfig
    {
        struct
        {
            struct
            {
                PrivilegeValueType value = 0;
                PrivilegeDurationType duration = 0;
            } create;
        } thread;
        struct
        {
            struct
            {
                PrivilegeValueType value = 0;
                PrivilegeDurationType duration = 0;
            } create;
        } threadMessage;
    };

    struct Config
    {
        UserConfig user;
        DiscussionThreadConfig discussionThread;
        DiscussionThreadMessageConfig discussionThreadMessage;
        PrivateMessageConfig privateMessage;
        DiscussionTagConfig discussionTag;
        DiscussionCategoryConfig discussionCategory;
        AttachmentConfig attachment;
        ServiceConfig service;
        LoggingConfig logging;
        PersistenceConfig persistence;
        std::vector<PluginEntry> plugins;
        DefaultPrivilegesConfig defaultPrivileges;
        DefaultPrivilegeDurationConfig defaultPrivilegeGrants;
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
    void loadGlobalConfigFromStream(std::istream& stream);
}
