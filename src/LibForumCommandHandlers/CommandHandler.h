#pragma once

#include <iosfwd>
#include <functional>
#include <vector>

#include <boost/core/noncopyable.hpp>

namespace Forum
{
    namespace Commands
    {
        enum Command
        {
            SHOW_VERSION = 0,
            LAST_COMMAND
        };

        class CommandHandler : private boost::noncopyable
        {
        public:
            CommandHandler();
            void handle(Command command, const std::vector<std::string>& parameters, std::ostream& output);

        private:
            void version(const std::vector<std::string>& parameters, std::ostream& output);

            std::function<void(const std::vector<std::string>&, std::ostream&)> handlers_[int(LAST_COMMAND)];
        };

        typedef std::shared_ptr<CommandHandler> CommandHandlerRef;
    }
}
