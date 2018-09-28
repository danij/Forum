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

#include "Configuration.h"
#include "ContextProviders.h"

#include <boost/property_tree/json_parser.hpp>

using namespace Forum::Configuration;

static std::shared_ptr<const Config> currentConfig = std::make_shared<const Config>();

std::shared_ptr<const Config> Forum::Configuration::getGlobalConfig()
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

static void loadPluginConfig(boost::property_tree::ptree& source, Config& config)
{
    for (const auto& [_, entry] : source.get_child("plugins"))
    {
        (void)_;
        config.plugins.push_back(
            {
                entry.get<std::string>("libraryPath"),
                entry.get_child("configuration")
            });
    }
}

void Forum::Configuration::loadGlobalConfigFromStream(std::istream& stream)
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
    LOAD_CONFIG_VALUE(user.visitorOnlineForSeconds);

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
    LOAD_CONFIG_VALUE(service.connectionPoolSize);
    LOAD_CONFIG_VALUE(service.listenIPAddress);
    LOAD_CONFIG_VALUE(service.listenPort);
    LOAD_CONFIG_VALUE(service.authListenIPAddress);
    LOAD_CONFIG_VALUE(service.authListenPort);
    LOAD_CONFIG_VALUE(service.connectionTimeoutSeconds);
    LOAD_CONFIG_VALUE(service.trustIpFromXForwardedFor);
    LOAD_CONFIG_VALUE(service.disableCommands);
    LOAD_CONFIG_VALUE(service.disableCommandsForAnonymousUsers);
    LOAD_CONFIG_VALUE(service.disableThrottling);
    LOAD_CONFIG_VALUE(service.responsePrefix);
    LOAD_CONFIG_VALUE(service.expectedOriginReferer);

    LOAD_CONFIG_VALUE(logging.settingsFile);

    LOAD_CONFIG_VALUE(persistence.inputFolder);
    LOAD_CONFIG_VALUE(persistence.outputFolder);
    LOAD_CONFIG_VALUE(persistence.messagesFile);
    LOAD_CONFIG_VALUE(persistence.validateChecksum);
    LOAD_CONFIG_VALUE(persistence.createNewOutputFileEverySeconds);
    LOAD_CONFIG_VALUE(persistence.persistIPAddresses);

    LOAD_CONFIG_VALUE(defaultPrivileges.threadMessage.view);
    LOAD_CONFIG_VALUE(defaultPrivileges.threadMessage.viewUnapproved);
    LOAD_CONFIG_VALUE(defaultPrivileges.threadMessage.viewRequiredPrivileges);
    LOAD_CONFIG_VALUE(defaultPrivileges.threadMessage.viewAssignedPrivileges);
    LOAD_CONFIG_VALUE(defaultPrivileges.threadMessage.viewCreatorUser);
    LOAD_CONFIG_VALUE(defaultPrivileges.threadMessage.viewIpAddress);
    LOAD_CONFIG_VALUE(defaultPrivileges.threadMessage.viewVotes);
    LOAD_CONFIG_VALUE(defaultPrivileges.threadMessage.upVote);
    LOAD_CONFIG_VALUE(defaultPrivileges.threadMessage.downVote);
    LOAD_CONFIG_VALUE(defaultPrivileges.threadMessage.resetVote);
    LOAD_CONFIG_VALUE(defaultPrivileges.threadMessage.addComment);
    LOAD_CONFIG_VALUE(defaultPrivileges.threadMessage.setCommentToSolved);
    LOAD_CONFIG_VALUE(defaultPrivileges.threadMessage.getMessageComments);
    LOAD_CONFIG_VALUE(defaultPrivileges.threadMessage.changeContent);
    LOAD_CONFIG_VALUE(defaultPrivileges.threadMessage.changeApproval);
    LOAD_CONFIG_VALUE(defaultPrivileges.threadMessage.deleteThreadMessage);
    LOAD_CONFIG_VALUE(defaultPrivileges.threadMessage.move);
    LOAD_CONFIG_VALUE(defaultPrivileges.threadMessage.adjustPrivilege);

    LOAD_CONFIG_VALUE(defaultPrivileges.thread.view);
    LOAD_CONFIG_VALUE(defaultPrivileges.thread.viewUnapproved);
    LOAD_CONFIG_VALUE(defaultPrivileges.thread.viewRequiredPrivileges);
    LOAD_CONFIG_VALUE(defaultPrivileges.thread.viewAssignedPrivileges);
    LOAD_CONFIG_VALUE(defaultPrivileges.thread.getSubscribedUsers);
    LOAD_CONFIG_VALUE(defaultPrivileges.thread.subscribe);
    LOAD_CONFIG_VALUE(defaultPrivileges.thread.unsubscribe);
    LOAD_CONFIG_VALUE(defaultPrivileges.thread.addMessage);
    LOAD_CONFIG_VALUE(defaultPrivileges.thread.autoApproveMessage);
    LOAD_CONFIG_VALUE(defaultPrivileges.thread.changeName);
    LOAD_CONFIG_VALUE(defaultPrivileges.thread.changePinDisplayOrder);
    LOAD_CONFIG_VALUE(defaultPrivileges.thread.changeApproval);
    LOAD_CONFIG_VALUE(defaultPrivileges.thread.addTag);
    LOAD_CONFIG_VALUE(defaultPrivileges.thread.removeTag);
    LOAD_CONFIG_VALUE(defaultPrivileges.thread.deleteThread);
    LOAD_CONFIG_VALUE(defaultPrivileges.thread.merge);
    LOAD_CONFIG_VALUE(defaultPrivileges.thread.adjustPrivilege);

    LOAD_CONFIG_VALUE(defaultPrivileges.tag.view);
    LOAD_CONFIG_VALUE(defaultPrivileges.tag.viewRequiredPrivileges);
    LOAD_CONFIG_VALUE(defaultPrivileges.tag.viewAssignedPrivileges);
    LOAD_CONFIG_VALUE(defaultPrivileges.tag.getDiscussionThreads);
    LOAD_CONFIG_VALUE(defaultPrivileges.tag.changeName);
    LOAD_CONFIG_VALUE(defaultPrivileges.tag.changeUiblob);
    LOAD_CONFIG_VALUE(defaultPrivileges.tag.deleteTag);
    LOAD_CONFIG_VALUE(defaultPrivileges.tag.merge);
    LOAD_CONFIG_VALUE(defaultPrivileges.tag.adjustPrivilege);

    LOAD_CONFIG_VALUE(defaultPrivileges.category.view);
    LOAD_CONFIG_VALUE(defaultPrivileges.category.viewRequiredPrivileges);
    LOAD_CONFIG_VALUE(defaultPrivileges.category.viewAssignedPrivileges);
    LOAD_CONFIG_VALUE(defaultPrivileges.category.getDiscussionThreads);
    LOAD_CONFIG_VALUE(defaultPrivileges.category.changeName);
    LOAD_CONFIG_VALUE(defaultPrivileges.category.changeDescription);
    LOAD_CONFIG_VALUE(defaultPrivileges.category.changeParent);
    LOAD_CONFIG_VALUE(defaultPrivileges.category.changeDisplayorder);
    LOAD_CONFIG_VALUE(defaultPrivileges.category.addTag);
    LOAD_CONFIG_VALUE(defaultPrivileges.category.removeTag);
    LOAD_CONFIG_VALUE(defaultPrivileges.category.deleteCategory);
    LOAD_CONFIG_VALUE(defaultPrivileges.category.adjustPrivilege);

    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.addUser);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.getEntitiesCount);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.getVersion);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.getAllUsers);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.getUserInfo);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.getDiscussionThreadsOfUser);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.getDiscussionThreadMessagesOfUser);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.getSubscribedDiscussionThreadsOfUser);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.getAllDiscussionCategories);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.getDiscussionCategoriesFromRoot);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.getAllDiscussionTags);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.getAllDiscussionThreads);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.getAllMessageComments);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.getMessageCommentsOfUser);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.addDiscussionCategory);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.addDiscussionTag);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.addDiscussionThread);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.autoApproveDiscussionThread);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.changeOwnUserName);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.changeOwnUserInfo);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.changeAnyUserName);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.changeAnyUserInfo);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.deleteAnyUser);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.viewForumWideRequiredPrivileges);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.viewForumWideAssignedPrivileges);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.viewUserAssignedPrivileges);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.adjustForumWidePrivilege);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.changeOwnUserTitle);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.changeAnyUserTitle);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.changeOwnUserSignature);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.changeAnyUserSignature);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.changeOwnUserLogo);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.changeAnyUserLogo);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.deleteOwnUserLogo);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.deleteAnyUserLogo);
    LOAD_CONFIG_VALUE(defaultPrivileges.forumWide.noThrottling);
    
    LOAD_CONFIG_VALUE(defaultPrivilegeGrants.thread.create.value);
    LOAD_CONFIG_VALUE(defaultPrivilegeGrants.thread.create.duration);
    LOAD_CONFIG_VALUE(defaultPrivilegeGrants.threadMessage.create.value);
    LOAD_CONFIG_VALUE(defaultPrivilegeGrants.threadMessage.create.duration);

    loadPluginConfig(tree, config);

    setGlobalConfig(config);
}
