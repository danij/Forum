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

#include "CommandHandler.h"
#include "HttpRouter.h"

#include <vector>
#include <string>

namespace Forum
{
    namespace Commands
    {
        //Endpoints can be called from multiple threads

        class AbstractEndpoint
        {
        public:
            DECLARE_ABSTRACT_MANDATORY(AbstractEndpoint)

            explicit AbstractEndpoint(CommandHandler& handler);

        protected:
            typedef CommandHandler::Result (*ExecuteFn)(const Http::RequestState&, CommandHandler&,
                                                        std::vector<StringView>&);

            void handle(Http::RequestState& requestState, ExecuteFn executeCommand);
            void handleInternal(Http::RequestState& requestState, StringView contentType, ExecuteFn executeCommand,
                                bool writePrefix);
            void handleBinary(Http::RequestState& requestState, StringView contentType, ExecuteFn executeCommand);

            CommandHandler& commandHandler_;
            std::string prefix_;
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
            void getOnline(Http::RequestState& requestState);
            void getUserById(Http::RequestState& requestState);
            void getUserByName(Http::RequestState& requestState);
            void searchUsersByName(Http::RequestState& requestState);
            void getUserLogo(Http::RequestState& requestState);
            void getUserVoteHistory(Http::RequestState& requestState);
            void getUsersSubscribedToThread(Http::RequestState& requestState);

            void add(Http::RequestState& requestState);
            void remove(Http::RequestState& requestState);
            void changeName(Http::RequestState& requestState);
            void changeInfo(Http::RequestState& requestState);
            void changeTitle(Http::RequestState& requestState);
            void changeSignature(Http::RequestState& requestState);
            void changeLogo(Http::RequestState& requestState);
            void deleteLogo(Http::RequestState& requestState);
        };

        class DiscussionThreadsEndpoint : private AbstractEndpoint
        {
        public:
            explicit DiscussionThreadsEndpoint(CommandHandler& handler);

            void getAll(Http::RequestState& requestState);
            void getThreadById(Http::RequestState& requestState);
            void getMultipleThreadsById(Http::RequestState& requestState);
            void getThreadsOfUser(Http::RequestState& requestState);
            void getThreadsWithTag(Http::RequestState& requestState);
            void getThreadsOfCategory(Http::RequestState& requestState);
            void searchThreadsByName(Http::RequestState& requestState);
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

            void getMultipleThreadMessagesById(Http::RequestState& requestState);
            void getThreadMessagesOfUser(Http::RequestState& requestState);
            void getLatestThreadMessages(Http::RequestState& requestState);
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
            void getAssignedPrivilegesForThread(Http::RequestState& requestState);
            void getRequiredPrivilegesForTag(Http::RequestState& requestState);
            void getAssignedPrivilegesForTag(Http::RequestState& requestState);
            void getRequiredPrivilegesForCategory(Http::RequestState& requestState);
            void getAssignedPrivilegesForCategory(Http::RequestState& requestState);
            void getForumWideCurrentUserPrivileges(Http::RequestState& requestState);
            void getForumWideRequiredPrivileges(Http::RequestState& requestState);
            void getForumWideDefaultPrivilegeLevels(Http::RequestState& requestState);
            void getForumWideAssignedPrivileges(Http::RequestState& requestState);

            void getAssignedPrivilegesForUser(Http::RequestState& requestState);

            void changeDiscussionThreadMessageRequiredPrivilegeForThreadMessage(Http::RequestState& requestState);
            void changeDiscussionThreadMessageRequiredPrivilegeForThread(Http::RequestState& requestState);
            void changeDiscussionThreadRequiredPrivilegeForThread(Http::RequestState& requestState);
            void changeDiscussionThreadMessageRequiredPrivilegeForTag(Http::RequestState& requestState);
            void changeDiscussionThreadRequiredPrivilegeForTag(Http::RequestState& requestState);
            void changeDiscussionTagRequiredPrivilegeForTag(Http::RequestState& requestState);
            void changeDiscussionCategoryRequiredPrivilegeForCategory(Http::RequestState& requestState);
            void changeDiscussionThreadMessageRequiredPrivilege(Http::RequestState& requestState);
            void changeDiscussionThreadRequiredPrivilege(Http::RequestState& requestState);
            void changeDiscussionTagRequiredPrivilege(Http::RequestState& requestState);
            void changeDiscussionCategoryRequiredPrivilege(Http::RequestState& requestState);
            void changeForumWideRequiredPrivilege(Http::RequestState& requestState);
            void changeForumWideDefaultPrivilegeLevel(Http::RequestState& requestState);

            void assignDiscussionThreadMessagePrivilege(Http::RequestState& requestState);
            void assignDiscussionThreadPrivilege(Http::RequestState& requestState);
            void assignDiscussionTagPrivilege(Http::RequestState& requestState);
            void assignDiscussionCategoryPrivilege(Http::RequestState& requestState);
            void assignForumWidePrivilege(Http::RequestState& requestState);
        };
    }
}
