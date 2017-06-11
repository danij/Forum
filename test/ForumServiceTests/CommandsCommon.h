#pragma once

#include "CommandHandler.h"
#include "ContextProviders.h"

#include <tuple>
#include <boost/property_tree/ptree.hpp>

#define CREATE_FUNCTION_ALIAS(name, target) \
    template<typename... Args> \
    static auto name(Args&&... args) \
    { \
        return target(std::forward<Args>(args)...); \
    } \

namespace Forum
{
    namespace Helpers
    {
        Commands::CommandHandlerRef createCommandHandler();

        struct DisplaySettings
        {
            int pageNumber = 0;
            Context::SortOrder sortOrder = Context::SortOrder::Ascending;
            Entities::Timestamp checkNotChangedSince = 0;

            DisplaySettings() {}
            DisplaySettings(Context::SortOrder overrideSortOrder) : sortOrder(overrideSortOrder) {}
        };

        using TreeType = boost::property_tree::ptree;
        using TreeStatusTupleType = std::tuple<TreeType, Forum::Repository::StatusCode>;

        TreeType handlerToObj(Commands::CommandHandlerRef& handler, Commands::Command command,
                              const std::vector<StringView>& parameters);
        TreeType handlerToObj(Commands::CommandHandlerRef& handler, Commands::Command command,
                              DisplaySettings displaySettings, const std::vector<StringView>& parameters);
        TreeType handlerToObj(Commands::CommandHandlerRef& handler, Commands::Command command);
        TreeType handlerToObj(Commands::CommandHandlerRef& handler, Commands::Command command,
                              DisplaySettings displaySettings);

        TreeStatusTupleType handlerToObjAndStatus(Commands::CommandHandlerRef& handler, Commands::Command command,
                                                  const std::vector<StringView>& parameters);
        TreeStatusTupleType handlerToObjAndStatus(Commands::CommandHandlerRef& handler, Commands::Command command,
                                                  DisplaySettings displaySettings,
                                                  const std::vector<StringView>& parameters);
        TreeStatusTupleType handlerToObjAndStatus(Commands::CommandHandlerRef& handler, Commands::Command command);
        TreeStatusTupleType handlerToObjAndStatus(Commands::CommandHandlerRef& handler, Commands::Command command,
                                                  DisplaySettings displaySettings);

        TreeType handlerToObj(Commands::CommandHandlerRef& handler, Commands::View view,
                              const std::vector<StringView>& parameters);
        TreeType handlerToObj(Commands::CommandHandlerRef& handler, Commands::View view,
                              DisplaySettings displaySettings, const std::vector<StringView>& parameters);
        TreeType handlerToObj(Commands::CommandHandlerRef& handler, Commands::View view);
        TreeType handlerToObj(Commands::CommandHandlerRef& handler, Commands::View view,
                              DisplaySettings displaySettings);

        TreeStatusTupleType handlerToObjAndStatus(Commands::CommandHandlerRef& handler, Commands::View view,
                                                  const std::vector<StringView>& parameters);
        TreeStatusTupleType handlerToObjAndStatus(Commands::CommandHandlerRef& handler, Commands::View view,
                                                  DisplaySettings displaySettings,
                                                  const std::vector<StringView>& parameters);
        TreeStatusTupleType handlerToObjAndStatus(Commands::CommandHandlerRef& handler, Commands::View view);
        TreeStatusTupleType handlerToObjAndStatus(Commands::CommandHandlerRef& handler, Commands::View view,
                                                  DisplaySettings displaySettings);

        TreeType createUser(Commands::CommandHandlerRef& handler, const std::string& name);
        std::string createUserAndGetId(Commands::CommandHandlerRef& handler, const std::string& name);
        std::string createDiscussionThreadAndGetId(Commands::CommandHandlerRef& handler, const std::string& name);
        std::string createDiscussionMessageAndGetId(Commands::CommandHandlerRef& handler,
                                                    const std::string& threadId, const std::string& content);
        std::string createDiscussionTagAndGetId(Commands::CommandHandlerRef& handler, const std::string& name);
        std::string createDiscussionCategoryAndGetId(Commands::CommandHandlerRef& handler, const std::string& name,
                                                     const std::string& parentId = "");
        void deleteDiscussionThread(Commands::CommandHandlerRef& handler, const std::string& id);
        void deleteDiscussionThreadMessage(Commands::CommandHandlerRef& handler, const std::string& id);
        void deleteDiscussionTag(Commands::CommandHandlerRef& handler, const std::string& id);

        template<typename T, typename It>
        void fillPropertyFromCollection(const boost::property_tree::ptree& collection, const char* name,
                                               It iterator, const T& defaultValue)
        {
            for (auto& pair : collection)
            {
                *iterator++ = pair.second.get(name, defaultValue);
            }
        }

        template<typename T>
        auto deserializeEntity(const boost::property_tree::ptree& tree)
        {
            T result;
            result.populate(tree);
            return result;
        }

        template<typename T>
        auto deserializeEntities(const boost::property_tree::ptree& collection)
        {
            std::vector<T> result;
            for (auto& tree : collection)
            {
                result.push_back(deserializeEntity<T>(tree.second));
            }
            return result;
        }
    }
}
