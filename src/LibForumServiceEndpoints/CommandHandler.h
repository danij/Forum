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

#include "Repository.h"

#include <functional>
#include <vector>

#include <boost/core/noncopyable.hpp>

namespace Forum::Commands
{
    enum Command
    {
        ADD_USER = 0,
        CHANGE_USER_NAME,
        CHANGE_USER_INFO,
        CHANGE_USER_TITLE,
        CHANGE_USER_SIGNATURE,
        CHANGE_USER_ATTACHMENT_QUOTA,
        CHANGE_USER_LOGO,
        DELETE_USER_LOGO,
        DELETE_USER,
        SEND_PRIVATE_MESSAGE,
        DELETE_PRIVATE_MESSAGE,

        ADD_DISCUSSION_THREAD,
        CHANGE_DISCUSSION_THREAD_NAME,
        CHANGE_DISCUSSION_THREAD_PIN_DISPLAY_ORDER,
        CHANGE_DISCUSSION_THREAD_APPROVAL,
        DELETE_DISCUSSION_THREAD,
        MERGE_DISCUSSION_THREADS,

        ADD_DISCUSSION_THREAD_MESSAGE,
        DELETE_DISCUSSION_THREAD_MESSAGE,
        CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT,
        CHANGE_DISCUSSION_THREAD_MESSAGE_APPROVAL,
        MOVE_DISCUSSION_THREAD_MESSAGE,

        UP_VOTE_DISCUSSION_THREAD_MESSAGE,
        DOWN_VOTE_DISCUSSION_THREAD_MESSAGE,
        RESET_VOTE_DISCUSSION_THREAD_MESSAGE,

        SUBSCRIBE_TO_THREAD,
        UNSUBSCRIBE_FROM_THREAD,

        ADD_COMMENT_TO_DISCUSSION_THREAD_MESSAGE,
        SET_MESSAGE_COMMENT_SOLVED,

        ADD_DISCUSSION_TAG,
        CHANGE_DISCUSSION_TAG_NAME,
        CHANGE_DISCUSSION_TAG_UI_BLOB,
        DELETE_DISCUSSION_TAG,
        ADD_DISCUSSION_TAG_TO_THREAD,
        REMOVE_DISCUSSION_TAG_FROM_THREAD,
        MERGE_DISCUSSION_TAG_INTO_OTHER_TAG,

        ADD_DISCUSSION_CATEGORY,
        CHANGE_DISCUSSION_CATEGORY_NAME,
        CHANGE_DISCUSSION_CATEGORY_DESCRIPTION,
        CHANGE_DISCUSSION_CATEGORY_PARENT,
        CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER,
        DELETE_DISCUSSION_CATEGORY,
        ADD_DISCUSSION_TAG_TO_CATEGORY,
        REMOVE_DISCUSSION_TAG_FROM_CATEGORY,

        ADD_ATTACHMENT,
        DELETE_ATTACHMENT,
        CHANGE_ATTACHMENT_NAME,
        CHANGE_ATTACHMENT_APPROVAL,
        ADD_ATTACHMENT_TO_DISCUSSION_THREAD_MESSAGE,
        REMOVE_ATTACHMENT_FROM_DISCUSSION_THREAD_MESSAGE,

        CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_THREAD_MESSAGE,

        CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_THREAD,
        CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE_FOR_THREAD,

        CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_TAG,
        CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE_FOR_TAG,
        CHANGE_DISCUSSION_TAG_REQUIRED_PRIVILEGE_FOR_TAG,

        CHANGE_DISCUSSION_CATEGORY_REQUIRED_PRIVILEGE_FOR_CATEGORY,

        CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE,
        CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE,
        CHANGE_DISCUSSION_TAG_REQUIRED_PRIVILEGE,
        CHANGE_DISCUSSION_CATEGORY_REQUIRED_PRIVILEGE,
        CHANGE_FORUM_WIDE_REQUIRED_PRIVILEGE,
        CHANGE_FORUM_WIDE_DEFAULT_PRIVILEGE_LEVEL,

        ASSIGN_DISCUSSION_THREAD_MESSAGE_PRIVILEGE,
        ASSIGN_DISCUSSION_THREAD_PRIVILEGE,
        ASSIGN_DISCUSSION_TAG_PRIVILEGE,
        ASSIGN_DISCUSSION_CATEGORY_PRIVILEGE,
        ASSIGN_FORUM_WIDE_PRIVILEGE,

        LAST_COMMAND
    };

    enum View
    {
        SHOW_VERSION = 0,
        COUNT_ENTITIES,

        GET_CURRENT_USER,
        GET_USERS_BY_NAME,
        GET_USERS_BY_CREATED,
        GET_USERS_BY_LAST_SEEN,
        GET_USERS_BY_THREAD_COUNT,
        GET_USERS_BY_MESSAGE_COUNT,
        GET_USERS_ONLINE,
        GET_USER_BY_ID,
        GET_USER_BY_NAME,
        GET_MULTIPLE_USERS_BY_ID,
        GET_MULTIPLE_USERS_BY_NAME,
        SEARCH_USERS_BY_NAME,
        GET_USER_LOGO,
        GET_USER_VOTE_HISTORY,
        GET_USER_QUOTED_HISTORY,
        GET_USER_RECEIVED_PRIVATE_MESSAGES,
        GET_USER_SENT_PRIVATE_MESSAGES,

        GET_DISCUSSION_THREADS_BY_NAME,
        GET_DISCUSSION_THREADS_BY_CREATED,
        GET_DISCUSSION_THREADS_BY_LAST_UPDATED,
        GET_DISCUSSION_THREADS_BY_LATEST_MESSAGE_CREATED,
        GET_DISCUSSION_THREADS_BY_MESSAGE_COUNT,
        GET_DISCUSSION_THREAD_BY_ID,
        GET_MULTIPLE_DISCUSSION_THREADS_BY_ID,
        SEARCH_DISCUSSION_THREADS_BY_NAME,

        GET_DISCUSSION_THREADS_OF_USER_BY_NAME,
        GET_DISCUSSION_THREADS_OF_USER_BY_CREATED,
        GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED,
        GET_DISCUSSION_THREADS_OF_USER_BY_LATEST_MESSAGE_CREATED,
        GET_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT,
        GET_USERS_SUBSCRIBED_TO_DISCUSSION_THREAD,

        GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_NAME,
        GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_CREATED,
        GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED,
        GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_LATEST_MESSAGE_CREATED,
        GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT,

        GET_MULTIPLE_DISCUSSION_THREAD_MESSAGES_BY_ID,
        GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED,
        GET_LATEST_DISCUSSION_THREAD_MESSAGES,
        GET_DISCUSSION_THREAD_MESSAGE_RANK,

        GET_MESSAGE_COMMENTS,
        GET_MESSAGE_COMMENTS_OF_DISCUSSION_THREAD_MESSAGE,
        GET_MESSAGE_COMMENTS_OF_USER,

        GET_DISCUSSION_TAGS_BY_NAME,
        GET_DISCUSSION_TAGS_BY_THREAD_COUNT,
        GET_DISCUSSION_TAGS_BY_MESSAGE_COUNT,

        GET_DISCUSSION_THREADS_WITH_TAG_BY_NAME,
        GET_DISCUSSION_THREADS_WITH_TAG_BY_CREATED,
        GET_DISCUSSION_THREADS_WITH_TAG_BY_LAST_UPDATED,
        GET_DISCUSSION_THREADS_WITH_TAG_BY_LATEST_MESSAGE_CREATED,
        GET_DISCUSSION_THREADS_WITH_TAG_BY_MESSAGE_COUNT,

        GET_DISCUSSION_CATEGORY_BY_ID,
        GET_DISCUSSION_CATEGORIES_BY_NAME,
        GET_DISCUSSION_CATEGORIES_BY_MESSAGE_COUNT,
        GET_DISCUSSION_CATEGORIES_FROM_ROOT,

        GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME,
        GET_DISCUSSION_THREADS_OF_CATEGORY_BY_CREATED,
        GET_DISCUSSION_THREADS_OF_CATEGORY_BY_LAST_UPDATED,
        GET_DISCUSSION_THREADS_OF_CATEGORY_BY_LATEST_MESSAGE_CREATED,
        GET_DISCUSSION_THREADS_OF_CATEGORY_BY_MESSAGE_COUNT,

        GET_ATTACHMENTS_BY_CREATED,
        GET_ATTACHMENTS_BY_NAME,
        GET_ATTACHMENTS_BY_SIZE,
        GET_ATTACHMENTS_BY_APPROVAL,
        GET_ATTACHMENTS_OF_USER_BY_CREATED,
        GET_ATTACHMENTS_OF_USER_BY_NAME,
        GET_ATTACHMENTS_OF_USER_BY_SIZE,
        GET_ATTACHMENTS_OF_USER_BY_APPROVAL,
        CAN_GET_ATTACHMENT,
        GET_ATTACHMENT,
        CAN_ADD_ATTACHMENT,

        GET_REQUIRED_PRIVILEGES_FOR_THREAD_MESSAGE,
        GET_ASSIGNED_PRIVILEGES_FOR_THREAD_MESSAGE,

        GET_REQUIRED_PRIVILEGES_FOR_THREAD,
        GET_ASSIGNED_PRIVILEGES_FOR_THREAD,

        GET_REQUIRED_PRIVILEGES_FOR_TAG,
        GET_ASSIGNED_PRIVILEGES_FOR_TAG,

        GET_REQUIRED_PRIVILEGES_FOR_CATEGORY,
        GET_ASSIGNED_PRIVILEGES_FOR_CATEGORY,

        GET_FORUM_WIDE_CURRENT_USER_PRIVILEGES,
        GET_FORUM_WIDE_REQUIRED_PRIVILEGES,
        GET_FORUM_WIDE_DEFAULT_PRIVILEGE_LEVELS,
        GET_FORUM_WIDE_ASSIGNED_PRIVILEGES,

        GET_ASSIGNED_PRIVILEGES_FOR_USER,

        LAST_VIEW
    };

    class CommandHandler final : boost::noncopyable
    {
    public:
        CommandHandler(Repository::ObservableRepositoryRef observerRepository,
                       Repository::UserRepositoryRef userRepository,
                       Repository::DiscussionThreadRepositoryRef discussionThreadRepository,
                       Repository::DiscussionThreadMessageRepositoryRef discussionThreadMessageRepository,
                       Repository::DiscussionTagRepositoryRef discussionTagRepository,
                       Repository::DiscussionCategoryRepositoryRef discussionCategoryRepository,
                       Repository::AttachmentRepositoryRef attachmentRepository,
                       Repository::AuthorizationRepositoryRef authorizationRepository,
                       Repository::StatisticsRepositoryRef statisticsRepository,
                       Repository::MetricsRepositoryRef metricsRepository);
        ~CommandHandler();

        struct Result
        {
            Repository::StatusCode statusCode;
            StringView output;
        };

        Result handle(Command command, const std::vector<StringView>& parameters);
        Result handle(View view, const std::vector<StringView>& parameters);

        Repository::ReadEvents& readEvents();
        Repository::WriteEvents& writeEvents();

    private:

        struct CommandHandlerImpl;
        //std::unique_ptr wants the complete type
        CommandHandlerImpl* impl_;
    };

    typedef std::shared_ptr<CommandHandler> CommandHandlerRef;
}
