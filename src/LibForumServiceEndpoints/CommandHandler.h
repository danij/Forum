#pragma once

#include "Repository.h"

#include <boost/core/noncopyable.hpp>
#include <boost/utility/string_view.hpp>

#include <iosfwd>
#include <functional>
#include <vector>

namespace Forum
{
    namespace Commands
    {
        enum Command
        {
            SHOW_VERSION = 0,
            COUNT_ENTITIES,

            //users related
            ADD_USER,
            GET_USERS_BY_NAME,
            GET_USERS_BY_CREATED,
            GET_USERS_BY_LAST_SEEN,
            GET_USERS_BY_THREAD_COUNT,
            GET_USERS_BY_MESSAGE_COUNT,
            GET_USER_BY_ID,
            GET_USER_BY_NAME,
            CHANGE_USER_NAME,
            CHANGE_USER_INFO,
            DELETE_USER,

            //discussion thread related
            ADD_DISCUSSION_THREAD,
            GET_DISCUSSION_THREADS_BY_NAME,
            GET_DISCUSSION_THREADS_BY_CREATED,
            GET_DISCUSSION_THREADS_BY_LAST_UPDATED,
            GET_DISCUSSION_THREADS_BY_MESSAGE_COUNT,
            GET_DISCUSSION_THREAD_BY_ID,
            CHANGE_DISCUSSION_THREAD_NAME,
            DELETE_DISCUSSION_THREAD,
            MERGE_DISCUSSION_THREADS,

            //discussion thread message related
            ADD_DISCUSSION_THREAD_MESSAGE,
            DELETE_DISCUSSION_THREAD_MESSAGE,
            CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT,
            MOVE_DISCUSSION_THREAD_MESSAGE,

            UP_VOTE_DISCUSSION_THREAD_MESSAGE,
            DOWN_VOTE_DISCUSSION_THREAD_MESSAGE,
            RESET_VOTE_DISCUSSION_THREAD_MESSAGE,

            //mixed user-discussion thread
            GET_DISCUSSION_THREADS_OF_USER_BY_NAME,
            GET_DISCUSSION_THREADS_OF_USER_BY_CREATED,
            GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED,
            GET_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT,

            GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED,            

            //message comments related
            ADD_COMMENT_TO_DISCUSSION_THREAD_MESSAGE,
            GET_MESSAGE_COMMENTS,
            GET_MESSAGE_COMMENTS_OF_DISCUSSION_THREAD_MESSAGE,
            GET_MESSAGE_COMMENTS_OF_USER,
            SET_MESSAGE_COMMENT_SOLVED,

            //discussion tag related
            ADD_DISCUSSION_TAG,
            GET_DISCUSSION_TAGS_BY_NAME,
            GET_DISCUSSION_TAGS_BY_MESSAGE_COUNT,
            CHANGE_DISCUSSION_TAG_NAME,
            CHANGE_DISCUSSION_TAG_UI_BLOB,
            DELETE_DISCUSSION_TAG,
            GET_DISCUSSION_THREADS_WITH_TAG_BY_NAME,
            GET_DISCUSSION_THREADS_WITH_TAG_BY_CREATED,
            GET_DISCUSSION_THREADS_WITH_TAG_BY_LAST_UPDATED,
            GET_DISCUSSION_THREADS_WITH_TAG_BY_MESSAGE_COUNT,
            ADD_DISCUSSION_TAG_TO_THREAD,
            REMOVE_DISCUSSION_TAG_FROM_THREAD,
            MERGE_DISCUSSION_TAG_INTO_OTHER_TAG,

            ADD_DISCUSSION_CATEGORY,
            GET_DISCUSSION_CATEGORY_BY_ID,
            GET_DISCUSSION_CATEGORIES_BY_NAME,
            GET_DISCUSSION_CATEGORIES_BY_MESSAGE_COUNT,
            GET_DISCUSSION_CATEGORIES_FROM_ROOT,
            CHANGE_DISCUSSION_CATEGORY_NAME,
            CHANGE_DISCUSSION_CATEGORY_DESCRIPTION,
            CHANGE_DISCUSSION_CATEGORY_PARENT,
            CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER,
            DELETE_DISCUSSION_CATEGORY,
            ADD_DISCUSSION_TAG_TO_CATEGORY,
            REMOVE_DISCUSSION_TAG_FROM_CATEGORY,
            GET_DISCUSSION_THREADS_OF_CATEGORY_BY_NAME,
            GET_DISCUSSION_THREADS_OF_CATEGORY_BY_CREATED,
            GET_DISCUSSION_THREADS_OF_CATEGORY_BY_LAST_UPDATED,
            GET_DISCUSSION_THREADS_OF_CATEGORY_BY_MESSAGE_COUNT,

            LAST_COMMAND
        };

        class CommandHandler final : private boost::noncopyable
        {
        public:
            CommandHandler(Repository::ObservableRepositoryRef observerRepository,
                           Repository::UserRepositoryRef userRepository,
                           Repository::DiscussionThreadRepositoryRef discussionThreadRepository,
                           Repository::DiscussionThreadMessageRepositoryRef discussionThreadMessageRepository,
                           Repository::DiscussionTagRepositoryRef discussionTagRepository,
                           Repository::DiscussionCategoryRepositoryRef discussionCategoryRepository,
                           Repository::StatisticsRepositoryRef statisticsRepository,
                           Repository::MetricsRepositoryRef metricsRepository);
            ~CommandHandler();

            struct Result
            {
                Repository::StatusCode statusCode;
                boost::string_view output;
            };

            Result handle(Command command, const std::vector<std::string>& parameters);

            Repository::ReadEvents& readEvents();
            Repository::WriteEvents& writeEvents();

        private:

            struct CommandHandlerImpl;
            //std::unique_ptr wants the complete type
            CommandHandlerImpl* impl_;
        };

        typedef std::shared_ptr<CommandHandler> CommandHandlerRef;
    }
}
