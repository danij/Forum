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
            //users related
            COUNT_USERS,
            ADD_USER,
            GET_USERS_BY_NAME,
            GET_USERS_BY_CREATED,
            GET_USERS_BY_LAST_SEEN,
            GET_USER_BY_NAME,
            CHANGE_USER_NAME,
            DELETE_USER,
            //discussion thread related
            COUNT_DISCUSSION_THREADS,
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

            DECLARE_COMMAND_HANDLER(countUsers);
            DECLARE_COMMAND_HANDLER(addNewUser);
            DECLARE_COMMAND_HANDLER(getUsersByName);
            DECLARE_COMMAND_HANDLER(getUsersByCreated);
            DECLARE_COMMAND_HANDLER(getUsersByLastSeen);
            DECLARE_COMMAND_HANDLER(getUserByName);
            DECLARE_COMMAND_HANDLER(changeUserName);
            DECLARE_COMMAND_HANDLER(deleteUser);

            DECLARE_COMMAND_HANDLER(countDiscussionThreads);

            bool checkNumberOfParameters(const std::vector<std::string>& parameters, std::ostream& output, size_t number);

            std::function<void(const std::vector<std::string>&, std::ostream&)> handlers_[int(LAST_COMMAND)];
            Forum::Repository::ReadRepositoryRef readRepository_;
            Forum::Repository::WriteRepositoryRef writeRepository_;
            Forum::Repository::MetricsRepositoryRef metricsRepository_;
        };

        typedef std::shared_ptr<CommandHandler> CommandHandlerRef;
    }
}
