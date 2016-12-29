#pragma once

#include "Repository.h"

#include <boost/core/noncopyable.hpp>

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
            GET_USER_BY_ID,
            GET_USER_BY_NAME,
            CHANGE_USER_NAME,
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

            ADD_DISCUSSION_THREAD_MESSAGE,
            DELETE_DISCUSSION_THREAD_MESSAGE,

            //mixed user-discussion thread
            GET_DISCUSSION_THREADS_OF_USER_BY_NAME,
            GET_DISCUSSION_THREADS_OF_USER_BY_CREATED,
            GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED,
            GET_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT,

            GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED,

            //discussion tag related
            ADD_DISCUSSION_TAG,
            GET_DISCUSSION_TAGS_BY_NAME,
            GET_DISCUSSION_TAGS_BY_MESSAGE_COUNT,
            CHANGE_DISCUSSION_TAG_NAME,
            DELETE_DISCUSSION_TAG,
            GET_DISCUSSION_THREADS_WITH_TAG_BY_NAME,
            GET_DISCUSSION_THREADS_WITH_TAG_BY_CREATED,
            GET_DISCUSSION_THREADS_WITH_TAG_BY_LAST_UPDATED,
            GET_DISCUSSION_THREADS_WITH_TAG_BY_MESSAGE_COUNT,
            ADD_DISCUSSION_TAG_TO_THREAD,
            REMOVE_DISCUSSION_TAG_FROM_THREAD,
            MERGE_DISCUSSION_TAG_INTO_OTHER_TAG,

            LAST_COMMAND
        };

        class CommandHandler final : private boost::noncopyable
        {
        public:
            CommandHandler(Repository::ReadRepositoryRef readRepository,
                           Repository::WriteRepositoryRef writeRepository,
                           Repository::MetricsRepositoryRef metricsRepository);
            void handle(Command command, const std::vector<std::string>& parameters, std::ostream& output);

            Repository::ReadRepositoryRef getReadRepository();
            Repository::WriteRepositoryRef getWriteRepository();

        private:
#define DECLARE_COMMAND_HANDLER(name) void name(const std::vector<std::string>& parameters, std::ostream& output);

            DECLARE_COMMAND_HANDLER(version);
            DECLARE_COMMAND_HANDLER(countEntities);

            DECLARE_COMMAND_HANDLER(addNewUser);
            DECLARE_COMMAND_HANDLER(getUsersByName);
            DECLARE_COMMAND_HANDLER(getUsersByCreated);
            DECLARE_COMMAND_HANDLER(getUsersByLastSeen);
            DECLARE_COMMAND_HANDLER(getUserById);
            DECLARE_COMMAND_HANDLER(getUserByName);
            DECLARE_COMMAND_HANDLER(changeUserName);
            DECLARE_COMMAND_HANDLER(deleteUser);

            DECLARE_COMMAND_HANDLER(addNewDiscussionThread);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadsByName);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadsByCreated);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadsByLastUpdated);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadsByMessageCount);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadById);
            DECLARE_COMMAND_HANDLER(changeDiscussionThreadName);
            DECLARE_COMMAND_HANDLER(deleteDiscussionThread);

            DECLARE_COMMAND_HANDLER(addNewDiscussionThreadMessage);
            DECLARE_COMMAND_HANDLER(deleteDiscussionThreadMessage);

            DECLARE_COMMAND_HANDLER(getDiscussionThreadsOfUserByName);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadsOfUserByCreated);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadsOfUserByLastUpdated);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadsOfUserByMessageCount);

            DECLARE_COMMAND_HANDLER(getDiscussionThreadMessagesOfUserByCreated);

            std::function<void(const std::vector<std::string>&, std::ostream&)> handlers_[int(LAST_COMMAND)];
            Repository::ReadRepositoryRef readRepository_;
            Repository::WriteRepositoryRef writeRepository_;
            Repository::MetricsRepositoryRef metricsRepository_;
        };

        typedef std::shared_ptr<CommandHandler> CommandHandlerRef;
    }
}
