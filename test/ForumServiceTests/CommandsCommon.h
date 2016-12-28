#pragma once

#include "CommandHandler.h"
#include "ContextProviders.h"

#include <boost/property_tree/ptree.hpp>

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
                                                 Commands::Command command, Context::SortOrder sortOrder,
                                                 const std::vector<std::string>& parameters);
        boost::property_tree::ptree handlerToObj(Commands::CommandHandlerRef handler,
                                                 Commands::Command command);
        boost::property_tree::ptree handlerToObj(Commands::CommandHandlerRef handler,
                                                 Commands::Command command, Context::SortOrder sortOrder);

        std::string createUserAndGetId(Commands::CommandHandlerRef handler, const std::string& name);
        std::string createDiscussionThreadAndGetId(Commands::CommandHandlerRef handler, const std::string& name);
        std::string createDiscussionMessageAndGetId(Commands::CommandHandlerRef handler,
                                                    const std::string& threadId, const std::string& content);
        std::string createDiscussionTagAndGetId(Commands::CommandHandlerRef handler, const std::string& name);

        template<typename T, typename It>
        void fillPropertyFromCollection(const boost::property_tree::ptree& collection, const char* name,
                                               It iterator, const T& defaultValue)
        {
            for (auto& pair : collection)
            {
                *iterator++ = pair.second.get(name, defaultValue);
            }
        };
    }
}
