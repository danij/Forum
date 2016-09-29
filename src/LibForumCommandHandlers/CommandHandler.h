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
            COUNT_USERS,
            ADD_USER,
            GET_USERS,
            GET_USER_BY_NAME,
            CHANGE_USER_NAME,
            LAST_COMMAND
        };

        class CommandHandler : private boost::noncopyable
        {
        public:
            CommandHandler(Forum::Repository::ReadRepositoryConstRef readRepository,
                           Forum::Repository::WriteRepositoryRef writeRepository,
                           Forum::Repository::MetricsRepositoryRef metricsRepository);
            void handle(Command command, const std::vector<std::string>& parameters, std::ostream& output);

        private:
            void version(const std::vector<std::string>& parameters, std::ostream& output);

            void countUsers(const std::vector<std::string>& parameters, std::ostream& output);
            void addNewUser(const std::vector<std::string>& parameters, std::ostream& output);
            void getUsers(const std::vector<std::string>& parameters, std::ostream& output);
            void getUserByName(const std::vector<std::string>& parameters, std::ostream& output);
            void changeUserName(const std::vector<std::string>& parameters, std::ostream& output);

            bool checkNumberOfParameters(const std::vector<std::string>& parameters, std::ostream& output, size_t number);

            std::function<void(const std::vector<std::string>&, std::ostream&)> handlers_[int(LAST_COMMAND)];
            Forum::Repository::ReadRepositoryConstRef readRepository_;
            Forum::Repository::WriteRepositoryRef writeRepository_;
            Forum::Repository::MetricsRepositoryRef metricsRepository_;
        };

        typedef std::shared_ptr<CommandHandler> CommandHandlerRef;
    }
}
