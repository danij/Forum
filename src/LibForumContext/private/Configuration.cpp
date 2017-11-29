/*
Fast Forum Backend
Copyright (C) 2016-2017 Daniel Jurcau

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

#include "Configuration.h"

#include <boost/property_tree/json_parser.hpp>
#include "ContextProviders.h"

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

#define CONCAT_MEMBER(structure, member) structure.member
#define LOAD_CONFIG_VALUE(path) \
    CONCAT_MEMBER(config, path) = tree.get<decltype(CONCAT_MEMBER(config, path))>(#path, CONCAT_MEMBER(config, path))

void Forum::Configuration::loadGlobalConfigFromStream(std::ifstream& stream)
{
    boost::property_tree::ptree tree;
    boost::property_tree::read_json(stream, tree);

    Config config;

    LOAD_CONFIG_VALUE(user.minNameLength);
    LOAD_CONFIG_VALUE(user.maxNameLength);
    LOAD_CONFIG_VALUE(user.minInfoLength);
    LOAD_CONFIG_VALUE(user.maxInfoLength);
    LOAD_CONFIG_VALUE(user.minTitleLength);
    LOAD_CONFIG_VALUE(user.maxTitleLength);
    LOAD_CONFIG_VALUE(user.minSignatureLength);
    LOAD_CONFIG_VALUE(user.maxSignatureLength);
    LOAD_CONFIG_VALUE(user.lastSeenUpdatePrecision);
    LOAD_CONFIG_VALUE(user.maxUsersPerPage);
    LOAD_CONFIG_VALUE(user.onlineUsersIntervalSeconds);
    LOAD_CONFIG_VALUE(user.maxLogoBinarySize);
    LOAD_CONFIG_VALUE(user.maxLogoWidth);
    LOAD_CONFIG_VALUE(user.maxLogoHeight);
    LOAD_CONFIG_VALUE(user.defaultPrivilegeValueForLoggedInUser);
    LOAD_CONFIG_VALUE(user.resetVoteExpiresInSeconds);

    LOAD_CONFIG_VALUE(discussionThread.minNameLength);
    LOAD_CONFIG_VALUE(discussionThread.maxNameLength);
    LOAD_CONFIG_VALUE(discussionThread.maxUsersInVisitedSinceLastChange);
    LOAD_CONFIG_VALUE(discussionThread.maxThreadsPerPage);

    LOAD_CONFIG_VALUE(discussionThreadMessage.minContentLength);
    LOAD_CONFIG_VALUE(discussionThreadMessage.maxContentLength);
    LOAD_CONFIG_VALUE(discussionThreadMessage.minChangeReasonLength);
    LOAD_CONFIG_VALUE(discussionThreadMessage.maxChangeReasonLength);
    LOAD_CONFIG_VALUE(discussionThreadMessage.maxMessagesPerPage);
    LOAD_CONFIG_VALUE(discussionThreadMessage.minCommentLength);
    LOAD_CONFIG_VALUE(discussionThreadMessage.maxCommentLength);
    LOAD_CONFIG_VALUE(discussionThreadMessage.maxMessagesCommentsPerPage);

    LOAD_CONFIG_VALUE(discussionTag.minNameLength);
    LOAD_CONFIG_VALUE(discussionTag.maxNameLength);
    LOAD_CONFIG_VALUE(discussionTag.maxUiBlobSize);

    LOAD_CONFIG_VALUE(discussionCategory.minNameLength);
    LOAD_CONFIG_VALUE(discussionCategory.maxNameLength);
    LOAD_CONFIG_VALUE(discussionCategory.maxDescriptionLength);

    LOAD_CONFIG_VALUE(service.numberOfIOServiceThreads);
    LOAD_CONFIG_VALUE(service.numberOfReadBuffers);
    LOAD_CONFIG_VALUE(service.numberOfWriteBuffers);
    LOAD_CONFIG_VALUE(service.listenIPAddress);
    LOAD_CONFIG_VALUE(service.listenPort);
    LOAD_CONFIG_VALUE(service.connectionTimeoutSeconds);
    LOAD_CONFIG_VALUE(service.trustIpFromXForwardedFor);
    LOAD_CONFIG_VALUE(service.disableCommands);
    LOAD_CONFIG_VALUE(service.disableCommandsForAnonymousUsers);
    LOAD_CONFIG_VALUE(service.responsePrefix);

    LOAD_CONFIG_VALUE(logging.settingsFile);

    LOAD_CONFIG_VALUE(persistence.inputFolder);
    LOAD_CONFIG_VALUE(persistence.outputFolder);
    LOAD_CONFIG_VALUE(persistence.messagesFile);
    LOAD_CONFIG_VALUE(persistence.validateChecksum);
    LOAD_CONFIG_VALUE(persistence.createNewOutputFileEverySeconds);

    setGlobalConfig(config);
}
