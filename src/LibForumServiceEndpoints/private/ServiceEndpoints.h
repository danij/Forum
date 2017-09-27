#pragma once

#include "CommandHandler.h"
#include "HttpRouter.h"

#include <vector>

namespace Forum
{
    namespace Commands
    {
        //Endpoints can be called from multiple threads

        class AbstractEndpoint
        {
        public:
            explicit AbstractEndpoint(CommandHandler& handler);

        protected:
            typedef CommandHandler::Result (*ExecuteFn)(const Http::RequestState&, CommandHandler&,
                                                        std::vector<StringView>&);

            void handle(Http::RequestState& requestState, ExecuteFn executeCommand);

            CommandHandler& commandHandler_;
        };

        class MetricsEndpoint : private AbstractEndpoint
        {
        public:
            explicit MetricsEndpoint(CommandHandler& handler);

            void getVersion(Http::RequestState& requestState);
        };

        class StatisticsEndpoint : private AbstractEndpoint
        {
        public:
            explicit StatisticsEndpoint(CommandHandler& handler);

            void getEntitiesCount(Http::RequestState& requestState);
        };

        class UsersEndpoint : private AbstractEndpoint
        {
        public:
            explicit UsersEndpoint(CommandHandler& handler);

            void getAll(Http::RequestState& requestState);
            void getUserById(Http::RequestState& requestState);
            void getUserByName(Http::RequestState& requestState);

            void add(Http::RequestState& requestState);
            void remove(Http::RequestState& requestState);
            void changeName(Http::RequestState& requestState);
            void changeInfo(Http::RequestState& requestState);
        };

        class DiscussionThreadsEndpoint : private AbstractEndpoint
        {
        public:
            explicit DiscussionThreadsEndpoint(CommandHandler& handler);

            void getAll(Http::RequestState& requestState);
            void getThreadById(Http::RequestState& requestState);
            void getThreadsOfUser(Http::RequestState& requestState);
            void getThreadsWithTag(Http::RequestState& requestState);
            void getThreadsOfCategory(Http::RequestState& requestState);
            void getSubscribedThreadsOfUser(Http::RequestState& requestState);

            void add(Http::RequestState& requestState);
            void remove(Http::RequestState& requestState);
            void changeName(Http::RequestState& requestState);
            void changePinDisplayOrder(Http::RequestState& requestState);
            void merge(Http::RequestState& requestState);
            void subscribe(Http::RequestState& requestState);
            void unsubscribe(Http::RequestState& requestState);
            void addTag(Http::RequestState& requestState);
            void removeTag(Http::RequestState& requestState);
        };

        class DiscussionThreadMessagesEndpoint : private AbstractEndpoint
        {
        public:
            explicit DiscussionThreadMessagesEndpoint(CommandHandler& handler);

            void getThreadMessagesOfUser(Http::RequestState& requestState);
            void getRankOfMessage(Http::RequestState& requestState);
            void getAllComments(Http::RequestState& requestState);
            void getCommentsOfMessage(Http::RequestState& requestState);
            void getCommentsOfUser(Http::RequestState& requestState);

            void add(Http::RequestState& requestState);
            void remove(Http::RequestState& requestState);
            void changeContent(Http::RequestState& requestState);
            void move(Http::RequestState& requestState);
            void upVote(Http::RequestState& requestState);
            void downVote(Http::RequestState& requestState);
            void resetVote(Http::RequestState& requestState);
            void addComment(Http::RequestState& requestState);
            void setCommentSolved(Http::RequestState& requestState);
        };

        class DiscussionTagsEndpoint : private AbstractEndpoint
        {
        public:
            explicit DiscussionTagsEndpoint(CommandHandler& handler);

            void getAll(Http::RequestState& requestState);

            void add(Http::RequestState& requestState);
            void remove(Http::RequestState& requestState);
            void changeName(Http::RequestState& requestState);
            void changeUiBlob(Http::RequestState& requestState);
            void merge(Http::RequestState& requestState);
        };

        class DiscussionCategoriesEndpoint : private AbstractEndpoint
        {
        public:
            explicit DiscussionCategoriesEndpoint(CommandHandler& handler);

            void getAll(Http::RequestState& requestState);
            void getRootCategories(Http::RequestState& requestState);
            void getCategoryById(Http::RequestState& requestState);

            void add(Http::RequestState& requestState);
            void remove(Http::RequestState& requestState);
            void changeName(Http::RequestState& requestState);
            void changeDescription(Http::RequestState& requestState);
            void changeParent(Http::RequestState& requestState);
            void changeDisplayOrder(Http::RequestState& requestState);
            void addTag(Http::RequestState& requestState);
            void removeTag(Http::RequestState& requestState);
        };

        class AuthorizationEndpoint : private AbstractEndpoint
        {
        public:
            explicit AuthorizationEndpoint(CommandHandler& handler);

            void getRequiredPrivilegesForThreadMessage(Http::RequestState& requestState);
            void getAssignedPrivilegesForThreadMessage(Http::RequestState& requestState);
            void getRequiredPrivilegesForThread(Http::RequestState& requestState);
            void getDefaultPrivilegeDurationsForThread(Http::RequestState& requestState);
            void getAssignedPrivilegesForThread(Http::RequestState& requestState);
            void getRequiredPrivilegesForTag(Http::RequestState& requestState);
            void getDefaultPrivilegeDurationsForTag(Http::RequestState& requestState);
            void getAssignedPrivilegesForTag(Http::RequestState& requestState);
            void getRequiredPrivilegesForCategory(Http::RequestState& requestState);
            void getAssignedPrivilegesForCategory(Http::RequestState& requestState);
            void getForumWideCurrentUserPrivileges(Http::RequestState& requestState);
            void getForumWideRequiredPrivileges(Http::RequestState& requestState);
            void getForumWideDefaultPrivilegeDurations(Http::RequestState& requestState);
            void getForumWideAssignedPrivileges(Http::RequestState& requestState);
            void getForumWideAssignedPrivilegesForUser(Http::RequestState& requestState);

            void changeDiscussionThreadMessageRequiredPrivilegeForThreadMessage(Http::RequestState& requestState);
            void assignDiscussionThreadMessagePrivilegeForThreadMessage(Http::RequestState& requestState);
            void changeDiscussionThreadMessageRequiredPrivilegeForThread(Http::RequestState& requestState);
            void changeDiscussionThreadRequiredPrivilegeForThread(Http::RequestState& requestState);
            void changeDiscussionThreadMessageDefaultPrivilegeDurationForThread(Http::RequestState& requestState);
            void assignDiscussionThreadMessagePrivilegeForThread(Http::RequestState& requestState);
            void assignDiscussionThreadPrivilegeForThread(Http::RequestState& requestState);
            void changeDiscussionThreadMessageRequiredPrivilegeForTag(Http::RequestState& requestState);
            void changeDiscussionThreadRequiredPrivilegeForTag(Http::RequestState& requestState);
            void changeDiscussionTagRequiredPrivilegeForTag(Http::RequestState& requestState);
            void changeDiscussionThreadMessageDefaultPrivilegeDurationForTag(Http::RequestState& requestState);
            void assignDiscussionThreadMessagePrivilegeForTag(Http::RequestState& requestState);
            void assignDiscussionThreadPrivilegeForTag(Http::RequestState& requestState);
            void assignDiscussionTagPrivilegeForTag(Http::RequestState& requestState);
            void changeDiscussionCategoryRequiredPrivilegeForCategory(Http::RequestState& requestState);
            void assignDiscussionCategoryPrivilegeForCategory(Http::RequestState& requestState);
            void changeDiscussionThreadMessageRequiredPrivilege(Http::RequestState& requestState);
            void changeDiscussionThreadRequiredPrivilege(Http::RequestState& requestState);
            void changeDiscussionTagRequiredPrivilege(Http::RequestState& requestState);
            void changeDiscussionCategoryRequiredPrivilege(Http::RequestState& requestState);
            void changeForumWideRequiredPrivilege(Http::RequestState& requestState);
            void changeDiscussionThreadMessageDefaultPrivilegeDuration(Http::RequestState& requestState);
            void changeForumWideDefaultPrivilegeDuration(Http::RequestState& requestState);
            void assignDiscussionThreadMessagePrivilege(Http::RequestState& requestState);
            void assignDiscussionThreadPrivilege(Http::RequestState& requestState);
            void assignDiscussionTagPrivilege(Http::RequestState& requestState);
            void assignDiscussionCategoryPrivilege(Http::RequestState& requestState);
            void assignForumWidePrivilege(Http::RequestState& requestState);
        };
    }
}
