#pragma once

#include <boost/property_tree/ptree.hpp>

#include "CommandHandler.h"

namespace Forum
{
    namespace Helpers
    {
        Commands::CommandHandlerRef createCommandHandler();

        std::string handlerToString(Commands::CommandHandlerRef handler, Commands::Command command,
                                    const std::vector<std::string>& parameters);
        std::string handlerToString(Commands::CommandHandlerRef handler, Commands::Command command);

        boost::property_tree::ptree handlerToObj(Commands::CommandHandlerRef handler,
                                                 Commands::Command command,
                                                 const std::vector<std::string>& parameters);
        boost::property_tree::ptree handlerToObj(Commands::CommandHandlerRef handler,
                                                 Commands::Command command);

        std::string createUserAndGetId(Commands::CommandHandlerRef handler, const std::string& name);
        std::string createDiscussionThreadAndGetId(Commands::CommandHandlerRef handler, const std::string& name);
        std::string createDiscussionMessageAndGetId(Commands::CommandHandlerRef handler,
                                                    const std::string& threadId, const std::string& content);

        template<typename T, typename It>
        inline void fillPropertyFromCollection(const boost::property_tree::ptree& collection, const char* name,
                                               It iterator, const T& defaultValue)
        {
            for (auto& pair : collection)
            {
                *iterator++ = pair.second.get(name, defaultValue);
            }
        };
    }
}
