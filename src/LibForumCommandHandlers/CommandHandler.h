#pragma once

#include <iosfwd>
#include <functional>
#include <vector>

#include <boost/core/noncopyable.hpp>

#include "Repository.h"

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
            GET_USERS_BY_CREATED_ASCENDING,
            GET_USERS_BY_CREATED_DESCENDING,
            GET_USERS_BY_LAST_SEEN_ASCENDING,
            GET_USERS_BY_LAST_SEEN_DESCENDING,
            GET_USER_BY_ID,
            GET_USER_BY_NAME,
            CHANGE_USER_NAME,
            DELETE_USER,

            //discussion thread related
            ADD_DISCUSSION_THREAD,
            GET_DISCUSSION_THREADS_BY_NAME,
            GET_DISCUSSION_THREADS_BY_CREATED_ASCENDING,
            GET_DISCUSSION_THREADS_BY_CREATED_DESCENDING,
            GET_DISCUSSION_THREADS_BY_LAST_UPDATED_ASCENDING,
            GET_DISCUSSION_THREADS_BY_LAST_UPDATED_DESCENDING,
            GET_DISCUSSION_THREADS_BY_MESSAGE_COUNT_ASCENDING,
            GET_DISCUSSION_THREADS_BY_MESSAGE_COUNT_DESCENDING,
            GET_DISCUSSION_THREAD_BY_ID,
            CHANGE_DISCUSSION_THREAD_NAME,
            DELETE_DISCUSSION_THREAD,

            ADD_DISCUSSION_THREAD_MESSAGE,
            DELETE_DISCUSSION_THREAD_MESSAGE,

            //mixed user-discussion thread
            GET_DISCUSSION_THREADS_OF_USER_BY_NAME,
            GET_DISCUSSION_THREADS_OF_USER_BY_CREATED_ASCENDING,
            GET_DISCUSSION_THREADS_OF_USER_BY_CREATED_DESCENDING,
            GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED_ASCENDING,
            GET_DISCUSSION_THREADS_OF_USER_BY_LAST_UPDATED_DESCENDING,
            GET_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT_ASCENDING,
            GET_DISCUSSION_THREADS_OF_USER_BY_MESSAGE_COUNT_DESCENDING,

            GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED_ASCENDING,
            GET_DISCUSSION_THREAD_MESSAGES_OF_USER_BY_CREATED_DESCENDING,

            LAST_COMMAND
        };

        class CommandHandler final : private boost::noncopyable
        {
        public:
            CommandHandler(Forum::Repository::ReadRepositoryRef readRepository,
                           Forum::Repository::WriteRepositoryRef writeRepository,
                           Forum::Repository::MetricsRepositoryRef metricsRepository);
            void handle(Command command, const std::vector<std::string>& parameters, std::ostream& output);

            Forum::Repository::ReadRepositoryRef getReadRepository();
            Forum::Repository::WriteRepositoryRef getWriteRepository();

        private:
#define DECLARE_COMMAND_HANDLER(name) void name(const std::vector<std::string>& parameters, std::ostream& output);

            DECLARE_COMMAND_HANDLER(version);
            DECLARE_COMMAND_HANDLER(countEntities);

            DECLARE_COMMAND_HANDLER(addNewUser);
            DECLARE_COMMAND_HANDLER(getUsersByName);
            DECLARE_COMMAND_HANDLER(getUsersByCreatedAscending);
            DECLARE_COMMAND_HANDLER(getUsersByCreatedDescending);
            DECLARE_COMMAND_HANDLER(getUsersByLastSeenAscending);
            DECLARE_COMMAND_HANDLER(getUsersByLastSeenDescending);
            DECLARE_COMMAND_HANDLER(getUserById);
            DECLARE_COMMAND_HANDLER(getUserByName);
            DECLARE_COMMAND_HANDLER(changeUserName);
            DECLARE_COMMAND_HANDLER(deleteUser);

            DECLARE_COMMAND_HANDLER(addNewDiscussionThread);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadsByName);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadsByCreatedAscending);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadsByCreatedDescending);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadsByLastUpdatedAscending);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadsByLastUpdatedDescending);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadsByMessageCountAscending);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadsByMessageCountDescending);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadById);
            DECLARE_COMMAND_HANDLER(changeDiscussionThreadName);
            DECLARE_COMMAND_HANDLER(deleteDiscussionThread);

            DECLARE_COMMAND_HANDLER(addNewDiscussionThreadMessage);
            DECLARE_COMMAND_HANDLER(deleteDiscussionThreadMessage);

            DECLARE_COMMAND_HANDLER(getDiscussionThreadsOfUserByName);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadsOfUserByCreatedAscending);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadsOfUserByCreatedDescending);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadsOfUserByLastUpdatedAscending);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadsOfUserByLastUpdatedDescending);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadsOfUserByMessageCountAscending);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadsOfUserByMessageCountDescending);

            DECLARE_COMMAND_HANDLER(getDiscussionThreadMessagesOfUserByCreatedAscending);
            DECLARE_COMMAND_HANDLER(getDiscussionThreadMessagesOfUserByCreatedDescending);

            bool checkNumberOfParameters(const std::vector<std::string>& parameters, std::ostream& output, size_t number);

            std::function<void(const std::vector<std::string>&, std::ostream&)> handlers_[int(LAST_COMMAND)];
            Forum::Repository::ReadRepositoryRef readRepository_;
            Forum::Repository::WriteRepositoryRef writeRepository_;
            Forum::Repository::MetricsRepositoryRef metricsRepository_;
        };

        typedef std::shared_ptr<CommandHandler> CommandHandlerRef;
    }
}
