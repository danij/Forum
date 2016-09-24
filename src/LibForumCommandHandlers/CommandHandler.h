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
            LAST_COMMAND
        };

        class CommandHandler : private boost::noncopyable
        {
        public:
            CommandHandler(Forum::Repository::ReadRepositoryConstRef readRepository,
                           Forum::Repository::WriteRepositoryRef writeRepository);
            void handle(Command command, const std::vector<std::string>& parameters, std::ostream& output);

        private:
            void version(const std::vector<std::string>& parameters, std::ostream& output);
            void countUsers(const std::vector<std::string>& parameters, std::ostream& output);
            void addNewUser(const std::vector<std::string>& parameters, std::ostream& output);
            void getUsers(const std::vector<std::string>& parameters, std::ostream& output);

            std::function<void(const std::vector<std::string>&, std::ostream&)> handlers_[int(LAST_COMMAND)];
            Forum::Repository::ReadRepositoryConstRef readRepository_;
            Forum::Repository::WriteRepositoryRef writeRepository_;
        };

        typedef std::shared_ptr<CommandHandler> CommandHandlerRef;
    }
}
