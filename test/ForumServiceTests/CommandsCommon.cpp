#include "CommandsCommon.h"

#include "MemoryRepositoryCommon.h"
#include "MemoryRepositoryUser.h"
#include "MemoryRepositoryDiscussionThread.h"
#include "MemoryRepositoryDiscussionThreadMessage.h"
#include "MemoryRepositoryDiscussionTag.h"
#include "MemoryRepositoryDiscussionCategory.h"
#include "MemoryRepositoryStatistics.h"
#include "MetricsRepository.h"
#include "TestHelpers.h"
#include "RandomGenerator.h"

#include <boost/property_tree/json_parser.hpp>

#include <sstream>
#include <string>

using namespace Forum::Commands;
using namespace Forum::Repository;
using namespace Forum::Helpers;
using namespace Forum::Authorization;

std::shared_ptr<CommandHandler> Forum::Helpers::createCommandHandler()
{
    auto authorization = std::make_shared<AllowAllAuthorization>();

    auto store = std::make_shared<MemoryStore>(std::make_shared<Entities::EntityCollection>());

    auto userRepository = std::make_shared<MemoryRepositoryUser>(store, authorization);
    auto discussionThreadRepository = std::make_shared<MemoryRepositoryDiscussionThread>(store, authorization);
    auto discussionThreadMessageRepository = std::make_shared<MemoryRepositoryDiscussionThreadMessage>(store, authorization);
    auto discussionTagRepository = std::make_shared<MemoryRepositoryDiscussionTag>(store, authorization);
    auto discussionCategoryRepository = std::make_shared<MemoryRepositoryDiscussionCategory>(store, authorization);
    auto statisticsRepository = std::make_shared<MemoryRepositoryStatistics>(store, authorization);
    auto metricsRepository = std::make_shared<MetricsRepository>(store, authorization);

    ObservableRepositoryRef observableRepository = userRepository;

    return std::make_shared<CommandHandler>(observableRepository, userRepository, discussionThreadRepository, 
        discussionThreadMessageRepository, discussionTagRepository, discussionCategoryRepository, 
        statisticsRepository, metricsRepository);
}

TreeType Forum::Helpers::handlerToObj(CommandHandlerRef& handler, Command command,
                                      const std::vector<StringView>& parameters)
{
    return std::get<0>(handlerToObjAndStatus(handler, command, parameters));
}

TreeType Forum::Helpers::handlerToObj(CommandHandlerRef& handler, Command command, DisplaySettings displaySettings,
                                      const std::vector<StringView>& parameters)
{
    return std::get<0>(handlerToObjAndStatus(handler, command, displaySettings, parameters));
}

TreeType Forum::Helpers::handlerToObj(CommandHandlerRef& handler, Command command)
{
    return std::get<0>(handlerToObjAndStatus(handler, command));
}

TreeType Forum::Helpers::handlerToObj(CommandHandlerRef& handler, Command command, DisplaySettings displaySettings)
{
    return std::get<0>(handlerToObjAndStatus(handler, command, displaySettings));
}

TreeStatusTupleType Forum::Helpers::handlerToObjAndStatus(CommandHandlerRef& handler, Command command,
                                                          const std::vector<StringView>& parameters)
{
    auto output = handler->handle(command, parameters);

    boost::property_tree::ptree result;
    if (output.output.size())
    {
        auto str = toString(output.output);
        std::istringstream stream(str);
        boost::property_tree::read_json(stream, result);
    }

    return std::make_tuple(result, output.statusCode);
}

TreeStatusTupleType Forum::Helpers::handlerToObjAndStatus(CommandHandlerRef& handler, Command command,
                                                          DisplaySettings displaySettings,
                                                          const std::vector<StringView>& parameters)
{
    auto oldPageNumber = Context::getDisplayContext().pageNumber;
    auto oldSortOrder = Context::getDisplayContext().sortOrder;
    auto oldCheckNotChangedSince = Context::getDisplayContext().checkNotChangedSince;

    auto _ = createDisposer([oldPageNumber]() { Context::getMutableDisplayContext().pageNumber = oldPageNumber; });
    auto __ = createDisposer([oldSortOrder]() { Context::getMutableDisplayContext().sortOrder = oldSortOrder; });
    auto ___ = createDisposer([oldCheckNotChangedSince]()
    {
        Context::getMutableDisplayContext().checkNotChangedSince = oldCheckNotChangedSince;
    });

    Context::getMutableDisplayContext().pageNumber = displaySettings.pageNumber;
    Context::getMutableDisplayContext().sortOrder = displaySettings.sortOrder;
    Context::getMutableDisplayContext().checkNotChangedSince = displaySettings.checkNotChangedSince;

    return handlerToObjAndStatus(handler, command, parameters);
}

TreeStatusTupleType Forum::Helpers::handlerToObjAndStatus(CommandHandlerRef& handler, Command command)
{
    return handlerToObjAndStatus(handler, command, DisplaySettings{});
}

TreeStatusTupleType Forum::Helpers::handlerToObjAndStatus(CommandHandlerRef& handler, Command command,
                                                          DisplaySettings displaySettings)
{
    return handlerToObjAndStatus(handler, command, displaySettings, {});
}


TreeType Forum::Helpers::handlerToObj(CommandHandlerRef& handler, View view,
                                      const std::vector<StringView>& parameters)
{
    return std::get<0>(handlerToObjAndStatus(handler, view, parameters));
}

TreeType Forum::Helpers::handlerToObj(CommandHandlerRef& handler, View view, DisplaySettings displaySettings,
                                      const std::vector<StringView>& parameters)
{
    return std::get<0>(handlerToObjAndStatus(handler, view, displaySettings, parameters));
}

TreeType Forum::Helpers::handlerToObj(CommandHandlerRef& handler, View view)
{
    return std::get<0>(handlerToObjAndStatus(handler, view));
}

TreeType Forum::Helpers::handlerToObj(CommandHandlerRef& handler, View view, DisplaySettings displaySettings)
{
    return std::get<0>(handlerToObjAndStatus(handler, view, displaySettings));
}

TreeStatusTupleType Forum::Helpers::handlerToObjAndStatus(CommandHandlerRef& handler, View view,
                                                          const std::vector<StringView>& parameters)
{
    auto output = handler->handle(view, parameters);

    boost::property_tree::ptree result;
    if (output.output.size())
    {
        auto str = toString(output.output);
        std::istringstream stream(str);
        boost::property_tree::read_json(stream, result);
    }

    return std::make_tuple(result, output.statusCode);
}

TreeStatusTupleType Forum::Helpers::handlerToObjAndStatus(CommandHandlerRef& handler, View view,
                                                          DisplaySettings displaySettings,
                                                          const std::vector<StringView>& parameters)
{
    auto oldPageNumber = Context::getDisplayContext().pageNumber;
    auto oldSortOrder = Context::getDisplayContext().sortOrder;
    auto oldCheckNotChangedSince = Context::getDisplayContext().checkNotChangedSince;

    auto _ = createDisposer([oldPageNumber]() { Context::getMutableDisplayContext().pageNumber = oldPageNumber; });
    auto __ = createDisposer([oldSortOrder]() { Context::getMutableDisplayContext().sortOrder = oldSortOrder; });
    auto ___ = createDisposer([oldCheckNotChangedSince]()
    {
        Context::getMutableDisplayContext().checkNotChangedSince = oldCheckNotChangedSince;
    });

    Context::getMutableDisplayContext().pageNumber = displaySettings.pageNumber;
    Context::getMutableDisplayContext().sortOrder = displaySettings.sortOrder;
    Context::getMutableDisplayContext().checkNotChangedSince = displaySettings.checkNotChangedSince;

    return handlerToObjAndStatus(handler, view, parameters);
}

TreeStatusTupleType Forum::Helpers::handlerToObjAndStatus(CommandHandlerRef& handler, View view)
{
    return handlerToObjAndStatus(handler, view, DisplaySettings{});
}

TreeStatusTupleType Forum::Helpers::handlerToObjAndStatus(CommandHandlerRef& handler, View view,
                                                          DisplaySettings displaySettings)
{
    return handlerToObjAndStatus(handler, view, displaySettings, {});
}

TreeType Forum::Helpers::createUser(CommandHandlerRef& handler, const std::string& name)
{
    return handlerToObj(handler, Command::ADD_USER, { name, static_cast<std::string>(generateUUIDString()) });
}

std::string Forum::Helpers::createUserAndGetId(CommandHandlerRef& handler, const std::string& name)
{
    auto result = createUser(handler, name);
    return result.get<std::string>("id");
}

std::string Forum::Helpers::createDiscussionThreadAndGetId(CommandHandlerRef& handler, const std::string& name)
{
    auto result = handlerToObj(handler, Command::ADD_DISCUSSION_THREAD, { name });
    return result.get<std::string>("id");
}

std::string Forum::Helpers::createDiscussionMessageAndGetId(CommandHandlerRef& handler, const std::string& threadId,
                                                            const std::string& content)
{
    auto result = handlerToObj(handler, Command::ADD_DISCUSSION_THREAD_MESSAGE, { threadId, content });
    return result.get<std::string>("id");
}

std::string Forum::Helpers::createDiscussionTagAndGetId(CommandHandlerRef& handler, const std::string& name)
{
    auto result = handlerToObj(handler, Command::ADD_DISCUSSION_TAG, { name });
    return result.get<std::string>("id");
}

std::string Forum::Helpers::createDiscussionCategoryAndGetId(CommandHandlerRef& handler, const std::string& name,
                                                             const std::string& parentId)
{
    auto result = handlerToObj(handler, Command::ADD_DISCUSSION_CATEGORY, { name, parentId });
    return result.get<std::string>("id");
}

void Forum::Helpers::deleteDiscussionThread(CommandHandlerRef& handler, const std::string& id)
{
    handlerToObj(handler, Command::DELETE_DISCUSSION_THREAD, { id });
}

void Forum::Helpers::deleteDiscussionThreadMessage(CommandHandlerRef& handler, const std::string& id)
{
    handlerToObj(handler, Command::DELETE_DISCUSSION_THREAD_MESSAGE, { id });
}

void Forum::Helpers::deleteDiscussionTag(CommandHandlerRef& handler, const std::string& id)
{
    handlerToObj(handler, Command::DELETE_DISCUSSION_TAG, { id });
}
