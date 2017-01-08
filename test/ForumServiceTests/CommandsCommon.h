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

        struct DisplaySettings
        {
            int pageNumber = 0;
            int pageSize = std::numeric_limits<int>::max();
            Context::SortOrder sortOrder = Context::SortOrder::Ascending;

            DisplaySettings() {}
            DisplaySettings(Context::SortOrder overrideSortOrder) : sortOrder(overrideSortOrder) {}
        };

        boost::property_tree::ptree handlerToObj(Commands::CommandHandlerRef handler,
                                                 Commands::Command command,
                                                 const std::vector<std::string>& parameters);
        boost::property_tree::ptree handlerToObj(Commands::CommandHandlerRef handler,
                                                 Commands::Command command, DisplaySettings displaySettings,
                                                 const std::vector<std::string>& parameters);
        boost::property_tree::ptree handlerToObj(Commands::CommandHandlerRef handler,
                                                 Commands::Command command);
        boost::property_tree::ptree handlerToObj(Commands::CommandHandlerRef handler,
                                                 Commands::Command command, DisplaySettings displaySettings);

        std::string createUserAndGetId(Commands::CommandHandlerRef handler, const std::string& name);
        std::string createDiscussionThreadAndGetId(Commands::CommandHandlerRef handler, const std::string& name);
        std::string createDiscussionMessageAndGetId(Commands::CommandHandlerRef handler,
                                                    const std::string& threadId, const std::string& content);
        std::string createDiscussionTagAndGetId(Commands::CommandHandlerRef handler, const std::string& name);
        std::string createDiscussionCategoryAndGetId(Commands::CommandHandlerRef handler, const std::string& name,
                                                     const std::string& parentId = "");
        void deleteDiscussionThread(Commands::CommandHandlerRef handler, const std::string& id);
        void deleteDiscussionTag(Commands::CommandHandlerRef handler, const std::string& id);

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
