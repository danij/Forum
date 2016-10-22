#pragma once

#include <boost/property_tree/ptree.hpp>

#include "CommandHandler.h"

namespace Forum
{
    namespace Helpers
    {
        Forum::Commands::CommandHandlerRef createCommandHandler();

        std::string handlerToString(Forum::Commands::CommandHandlerRef handler, Forum::Commands::Command command,
                                    const std::vector<std::string>& parameters);
        std::string handlerToString(Forum::Commands::CommandHandlerRef handler, Forum::Commands::Command command);

        boost::property_tree::ptree handlerToObj(Forum::Commands::CommandHandlerRef handler,
                                                 Forum::Commands::Command command,
                                                 const std::vector<std::string>& parameters);
        boost::property_tree::ptree handlerToObj(Forum::Commands::CommandHandlerRef handler,
                                                 Forum::Commands::Command command);

        Forum::Entities::IdType createUserAndGetId(Forum::Commands::CommandHandlerRef handler, const std::string& name);

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
