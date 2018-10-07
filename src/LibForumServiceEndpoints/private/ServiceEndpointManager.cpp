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

#include "ServiceEndpointManager.h"
#include "ServiceEndpoints.h"

#include <tuple>

using namespace Forum;
using namespace Forum::Commands;
using namespace Http;

struct ServiceEndpointManager::ServiceEndpointManagerImpl
{
    explicit ServiceEndpointManagerImpl(CommandHandler& handler)
        : commandHandler(handler), metricsEndpoint(handler), statisticsEndpoint(handler),
          usersEndpoint(handler), threadsEndpoint(handler), threadMessagesEndpoint(handler),
          tagsEndpoint(handler), categoriesEndpoint(handler), attachmentsEndpoint(handler),
          authorizationEndpoint(handler)
    {
    }

    CommandHandler& commandHandler;
    MetricsEndpoint metricsEndpoint;
    StatisticsEndpoint statisticsEndpoint;
    UsersEndpoint usersEndpoint;
    DiscussionThreadsEndpoint threadsEndpoint;
    DiscussionThreadMessagesEndpoint threadMessagesEndpoint;
    DiscussionTagsEndpoint tagsEndpoint;
    DiscussionCategoriesEndpoint categoriesEndpoint;
    AttachmentsEndpoint attachmentsEndpoint;
    AuthorizationEndpoint authorizationEndpoint;
};

ServiceEndpointManager::ServiceEndpointManager(CommandHandler& handler)
{
    impl_ = new ServiceEndpointManagerImpl(handler);
}

ServiceEndpointManager::~ServiceEndpointManager()
{
    if (impl_)
    {
        delete impl_;
    }
}

#define ENDPOINT_DELEGATE(method) [this](auto& state) { this->impl_->method(state); }

void ServiceEndpointManager::registerRoutes(HttpRouter& router)
{
    std::tuple<StringView, HttpVerb, HttpRouter::HandlerFn> routes[] =
    {
        { "metrics/version",        HttpVerb::GET, ENDPOINT_DELEGATE(metricsEndpoint.getVersion) },
        { "statistics/entitycount", HttpVerb::GET, ENDPOINT_DELEGATE(statisticsEndpoint.getEntitiesCount) },

        { "users",                   HttpVerb::GET,    ENDPOINT_DELEGATE(usersEndpoint.getAll) },
        { "users/current",           HttpVerb::GET,    ENDPOINT_DELEGATE(usersEndpoint.getCurrent) },
        { "users/online",            HttpVerb::GET,    ENDPOINT_DELEGATE(usersEndpoint.getOnline) },
        { "users/id",                HttpVerb::GET,    ENDPOINT_DELEGATE(usersEndpoint.getUserById) },
        { "users/name",              HttpVerb::GET,    ENDPOINT_DELEGATE(usersEndpoint.getUserByName) },
        { "users/multiple/ids",      HttpVerb::GET,    ENDPOINT_DELEGATE(usersEndpoint.getMultipleUsersById) },
        { "users/multiple/names",    HttpVerb::GET,    ENDPOINT_DELEGATE(usersEndpoint.getMultipleUsersByName) },
        { "users/search",            HttpVerb::GET,    ENDPOINT_DELEGATE(usersEndpoint.searchUsersByName) },
        { "users",                   HttpVerb::POST,   ENDPOINT_DELEGATE(usersEndpoint.add) },
        { "users",                   HttpVerb::DELETE, ENDPOINT_DELEGATE(usersEndpoint.remove) },
        { "users/name",              HttpVerb::PUT,    ENDPOINT_DELEGATE(usersEndpoint.changeName) },
        { "users/info",              HttpVerb::PUT,    ENDPOINT_DELEGATE(usersEndpoint.changeInfo) },
        { "users/title",             HttpVerb::PUT,    ENDPOINT_DELEGATE(usersEndpoint.changeTitle) },
        { "users/signature",         HttpVerb::PUT,    ENDPOINT_DELEGATE(usersEndpoint.changeSignature) },
        { "users/attachment_quota",  HttpVerb::PUT,    ENDPOINT_DELEGATE(usersEndpoint.changeAttachmentQuota) },
        { "users/logo",              HttpVerb::GET,    ENDPOINT_DELEGATE(usersEndpoint.getUserLogo) },
        { "users/logo",              HttpVerb::PUT,    ENDPOINT_DELEGATE(usersEndpoint.changeLogo) },
        { "users/logo",              HttpVerb::DELETE, ENDPOINT_DELEGATE(usersEndpoint.deleteLogo) },
        { "users/votehistory",       HttpVerb::GET,    ENDPOINT_DELEGATE(usersEndpoint.getUserVoteHistory) },
        { "users/quotedhistory",     HttpVerb::GET,    ENDPOINT_DELEGATE(usersEndpoint.getUserQuotedHistory) },
        { "users/subscribed/thread", HttpVerb::GET,    ENDPOINT_DELEGATE(usersEndpoint.getUsersSubscribedToThread) },

        { "threads",                 HttpVerb::GET,    ENDPOINT_DELEGATE(threadsEndpoint.getAll) },
        { "threads/id",              HttpVerb::GET,    ENDPOINT_DELEGATE(threadsEndpoint.getThreadById) },
        { "threads/multiple",        HttpVerb::GET,    ENDPOINT_DELEGATE(threadsEndpoint.getMultipleThreadsById) },
        { "threads/user",            HttpVerb::GET,    ENDPOINT_DELEGATE(threadsEndpoint.getThreadsOfUser) },
        { "threads/subscribed/user", HttpVerb::GET,    ENDPOINT_DELEGATE(threadsEndpoint.getSubscribedThreadsOfUser) },
        { "threads/tag",             HttpVerb::GET,    ENDPOINT_DELEGATE(threadsEndpoint.getThreadsWithTag) },
        { "threads/category",        HttpVerb::GET,    ENDPOINT_DELEGATE(threadsEndpoint.getThreadsOfCategory) },
        { "threads/search",          HttpVerb::GET,    ENDPOINT_DELEGATE(threadsEndpoint.searchThreadsByName) },
        { "threads",                 HttpVerb::POST,   ENDPOINT_DELEGATE(threadsEndpoint.add) },
        { "threads",                 HttpVerb::DELETE, ENDPOINT_DELEGATE(threadsEndpoint.remove) },
        { "threads/name",            HttpVerb::PUT,    ENDPOINT_DELEGATE(threadsEndpoint.changeName) },
        { "threads/pindisplayorder", HttpVerb::PUT,    ENDPOINT_DELEGATE(threadsEndpoint.changePinDisplayOrder) },
        { "threads/approval",        HttpVerb::PUT,    ENDPOINT_DELEGATE(threadsEndpoint.changeApproval) },
        { "threads/merge",           HttpVerb::POST,   ENDPOINT_DELEGATE(threadsEndpoint.merge) },
        { "threads/subscribe",       HttpVerb::POST,   ENDPOINT_DELEGATE(threadsEndpoint.subscribe) },
        { "threads/unsubscribe",     HttpVerb::POST,   ENDPOINT_DELEGATE(threadsEndpoint.unsubscribe) },
        { "threads/tag",             HttpVerb::POST,   ENDPOINT_DELEGATE(threadsEndpoint.addTag) },
        { "threads/tag",             HttpVerb::DELETE, ENDPOINT_DELEGATE(threadsEndpoint.removeTag) },

        { "thread_messages/multiple",       HttpVerb::GET,    ENDPOINT_DELEGATE(threadMessagesEndpoint.getMultipleThreadMessagesById) },
        { "thread_messages/user",           HttpVerb::GET,    ENDPOINT_DELEGATE(threadMessagesEndpoint.getThreadMessagesOfUser) },
        { "thread_messages/latest",         HttpVerb::GET,    ENDPOINT_DELEGATE(threadMessagesEndpoint.getLatestThreadMessages) },
        { "thread_messages/allcomments",    HttpVerb::GET,    ENDPOINT_DELEGATE(threadMessagesEndpoint.getAllComments) },
        { "thread_messages/comments",       HttpVerb::GET,    ENDPOINT_DELEGATE(threadMessagesEndpoint.getCommentsOfMessage) },
        { "thread_messages/comments/user",  HttpVerb::GET,    ENDPOINT_DELEGATE(threadMessagesEndpoint.getCommentsOfUser) },
        { "thread_messages/rank",           HttpVerb::GET,    ENDPOINT_DELEGATE(threadMessagesEndpoint.getRankOfMessage) },
        { "thread_messages",                HttpVerb::POST,   ENDPOINT_DELEGATE(threadMessagesEndpoint.add) },
        { "thread_messages",                HttpVerb::DELETE, ENDPOINT_DELEGATE(threadMessagesEndpoint.remove) },
        { "thread_messages/content",        HttpVerb::PUT,    ENDPOINT_DELEGATE(threadMessagesEndpoint.changeContent) },
        { "thread_messages/approval",       HttpVerb::PUT,    ENDPOINT_DELEGATE(threadMessagesEndpoint.changeApproval) },
        { "thread_messages/move",           HttpVerb::POST,   ENDPOINT_DELEGATE(threadMessagesEndpoint.move) },
        { "thread_messages/upvote",         HttpVerb::POST,   ENDPOINT_DELEGATE(threadMessagesEndpoint.upVote) },
        { "thread_messages/downvote",       HttpVerb::POST,   ENDPOINT_DELEGATE(threadMessagesEndpoint.downVote) },
        { "thread_messages/resetvote",      HttpVerb::POST,   ENDPOINT_DELEGATE(threadMessagesEndpoint.resetVote) },
        { "thread_messages/comment",        HttpVerb::POST,   ENDPOINT_DELEGATE(threadMessagesEndpoint.addComment) },
        { "thread_messages/comment/solved", HttpVerb::PUT,    ENDPOINT_DELEGATE(threadMessagesEndpoint.setCommentSolved) },

        { "private_messages/received",      HttpVerb::GET,    ENDPOINT_DELEGATE(usersEndpoint.getReceivedPrivateMessages) },
        { "private_messages/sent",          HttpVerb::GET,    ENDPOINT_DELEGATE(usersEndpoint.getSentPrivateMessages) },
        { "private_messages",               HttpVerb::POST,   ENDPOINT_DELEGATE(usersEndpoint.sendPrivateMessage) },
        { "private_messages",               HttpVerb::DELETE, ENDPOINT_DELEGATE(usersEndpoint.deletePrivateMessage) },
        
        { "tags",        HttpVerb::GET,    ENDPOINT_DELEGATE(tagsEndpoint.getAll) },
        { "tags",        HttpVerb::POST,   ENDPOINT_DELEGATE(tagsEndpoint.add) },
        { "tags",        HttpVerb::DELETE, ENDPOINT_DELEGATE(tagsEndpoint.remove) },
        { "tags/name",   HttpVerb::PUT,    ENDPOINT_DELEGATE(tagsEndpoint.changeName) },
        { "tags/uiblob", HttpVerb::PUT,    ENDPOINT_DELEGATE(tagsEndpoint.changeUiBlob) },
        { "tags/merge",  HttpVerb::POST,   ENDPOINT_DELEGATE(tagsEndpoint.merge) },

        { "categories",              HttpVerb::GET,    ENDPOINT_DELEGATE(categoriesEndpoint.getAll) },
        { "categories/root",         HttpVerb::GET,    ENDPOINT_DELEGATE(categoriesEndpoint.getRootCategories) },
        { "category",                HttpVerb::GET,    ENDPOINT_DELEGATE(categoriesEndpoint.getCategoryById) },
        { "categories",              HttpVerb::POST,   ENDPOINT_DELEGATE(categoriesEndpoint.add) },
        { "categories",              HttpVerb::DELETE, ENDPOINT_DELEGATE(categoriesEndpoint.remove) },
        { "categories/name",         HttpVerb::PUT,    ENDPOINT_DELEGATE(categoriesEndpoint.changeName) },
        { "categories/description",  HttpVerb::PUT,    ENDPOINT_DELEGATE(categoriesEndpoint.changeDescription) },
        { "categories/parent",       HttpVerb::PUT,    ENDPOINT_DELEGATE(categoriesEndpoint.changeParent) },
        { "categories/displayorder", HttpVerb::PUT,    ENDPOINT_DELEGATE(categoriesEndpoint.changeDisplayOrder) },
        { "categories/tag",          HttpVerb::POST,   ENDPOINT_DELEGATE(categoriesEndpoint.addTag) },
        { "categories/tag",          HttpVerb::DELETE, ENDPOINT_DELEGATE(categoriesEndpoint.removeTag) },

        { "attachments",          HttpVerb::GET,    ENDPOINT_DELEGATE(attachmentsEndpoint.getAll) },
        { "attachments/user",     HttpVerb::GET,    ENDPOINT_DELEGATE(attachmentsEndpoint.getOfUser) },
        { "attachments/try",      HttpVerb::GET,    ENDPOINT_DELEGATE(attachmentsEndpoint.get) },
        { "attachments",          HttpVerb::POST,   ENDPOINT_DELEGATE(attachmentsEndpoint.add) },
        { "attachments",          HttpVerb::DELETE, ENDPOINT_DELEGATE(attachmentsEndpoint.remove) },
        { "attachments/name",     HttpVerb::PUT,    ENDPOINT_DELEGATE(attachmentsEndpoint.changeName) },
        { "attachments/approval", HttpVerb::PUT,    ENDPOINT_DELEGATE(attachmentsEndpoint.changeApproval) },
        { "attachments/message",  HttpVerb::POST,   ENDPOINT_DELEGATE(attachmentsEndpoint.addToMessage) },
        { "attachments/message",  HttpVerb::DELETE, ENDPOINT_DELEGATE(attachmentsEndpoint.removeFromMessage) },

        { "privileges/required/thread_message",  HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getRequiredPrivilegesForThreadMessage) },
        { "privileges/assigned/thread_message",  HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getAssignedPrivilegesForThreadMessage) },
        { "privileges/required/thread",          HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getRequiredPrivilegesForThread) },
        { "privileges/assigned/thread",          HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getAssignedPrivilegesForThread) },
        { "privileges/required/tag",             HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getRequiredPrivilegesForTag) },
        { "privileges/assigned/tag",             HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getAssignedPrivilegesForTag) },
        { "privileges/required/category",        HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getRequiredPrivilegesForCategory) },
        { "privileges/assigned/category",        HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getAssignedPrivilegesForCategory) },
        { "privileges/forum_wide/current_user",  HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getForumWideCurrentUserPrivileges) },
        { "privileges/required/forum_wide",      HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getForumWideRequiredPrivileges) },
        { "privileges/defaults/forum_wide",      HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getForumWideDefaultPrivilegeLevels) },
        { "privileges/assigned/forum_wide",      HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getForumWideAssignedPrivileges) },
        { "privileges/assigned/user",            HttpVerb::GET, ENDPOINT_DELEGATE(authorizationEndpoint.getAssignedPrivilegesForUser) },

        { "privileges/thread_message/required/thread_message", HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeDiscussionThreadMessageRequiredPrivilegeForThreadMessage) },
        { "privileges/thread_message/required/thread",         HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeDiscussionThreadMessageRequiredPrivilegeForThread) },
        { "privileges/thread/required/thread",                 HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeDiscussionThreadRequiredPrivilegeForThread) },
        { "privileges/thread_message/required/tag",            HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeDiscussionThreadMessageRequiredPrivilegeForTag) },
        { "privileges/thread/required/tag",                    HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeDiscussionThreadRequiredPrivilegeForTag) },
        { "privileges/tag/required/tag",                       HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeDiscussionTagRequiredPrivilegeForTag) },
        { "privileges/category/required/category",             HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeDiscussionCategoryRequiredPrivilegeForCategory) },
        { "privileges/thread_message/required/forum_wide",     HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeDiscussionThreadMessageRequiredPrivilege) },
        { "privileges/thread/required/forum_wide",             HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeDiscussionThreadRequiredPrivilege) },
        { "privileges/tag/required/forum_wide",                HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeDiscussionTagRequiredPrivilege) },
        { "privileges/category/required/forum_wide",           HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeDiscussionCategoryRequiredPrivilege) },
        { "privileges/forum_wide/required/forum_wide",         HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeForumWideRequiredPrivilege) },
        { "privileges/forum_wide/defaults/forum_wide",         HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.changeForumWideDefaultPrivilegeLevel) },

        { "privileges/thread_message/assign",                  HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.assignDiscussionThreadMessagePrivilege) },
        { "privileges/thread/assign",                          HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.assignDiscussionThreadPrivilege) },
        { "privileges/tag/assign",                             HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.assignDiscussionTagPrivilege) },
        { "privileges/category/assign",                        HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.assignDiscussionCategoryPrivilege) },
        { "privileges/forum_wide/assign",                      HttpVerb::POST, ENDPOINT_DELEGATE(authorizationEndpoint.assignForumWidePrivilege) }
    };

    for (auto& [pathLowerCase, verb, handler] : routes)
    {
        router.addRoute(pathLowerCase, verb, std::move(handler));
    }
}

void ServiceEndpointManager::registerAuthRoutes(HttpRouter& router)
{
    std::tuple<StringView, HttpVerb, HttpRouter::HandlerFn> routes[] =
    {
        { "login", HttpVerb::POST, ENDPOINT_DELEGATE(usersEndpoint.login) }
    };

    for (auto& [pathLowerCase, verb, handler] : routes)
    {
        router.addRoute(pathLowerCase, verb, std::move(handler));
    }
}
