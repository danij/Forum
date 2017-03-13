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

#include <boost/property_tree/json_parser.hpp>

#include <sstream>
#include <string>

using namespace Forum::Commands;
using namespace Forum::Repository;
using namespace Forum::Helpers;

std::shared_ptr<CommandHandler> Forum::Helpers::createCommandHandler()
{
    auto store = std::make_shared<MemoryStore>(std::make_shared<Entities::EntityCollection>());
    auto userRepository = std::make_shared<MemoryRepositoryUser>(store);
    auto discussionThreadRepository = std::make_shared<MemoryRepositoryDiscussionThread>(store);
    auto discussionThreadMessageRepository = std::make_shared<MemoryRepositoryDiscussionThreadMessage>(store);
    auto discussionTagRepository = std::make_shared<MemoryRepositoryDiscussionTag>(store);
    auto discussionCategoryRepository = std::make_shared<MemoryRepositoryDiscussionCategory>(store);
    auto statisticsRepository = std::make_shared<MemoryRepositoryStatistics>(store);
    auto metricsRepository = std::make_shared<MetricsRepository>();

    ObservableRepositoryRef observableRepository = userRepository;

    return std::make_shared<CommandHandler>(observableRepository, userRepository, discussionThreadRepository, 
        discussionThreadMessageRepository, discussionTagRepository, discussionCategoryRepository, 
        statisticsRepository, metricsRepository);
}

TreeType Forum::Helpers::handlerToObj(CommandHandlerRef& handler, Command command,
                                      const std::vector<std::string>& parameters)
{
    return std::get<0>(handlerToObjAndStatus(handler, command, parameters));
}

TreeType Forum::Helpers::handlerToObj(CommandHandlerRef& handler, Command command, DisplaySettings displaySettings,
                                      const std::vector<std::string>& parameters)
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
                                                          const std::vector<std::string>& parameters)
{
    auto output = handler->handle(command, parameters);

    boost::property_tree::ptree result;
    if (output.output.size())
    {
        std::istringstream stream(output.output);
        boost::property_tree::read_json(stream, result);
    }

    return std::make_tuple(result, output.statusCode);
}

TreeStatusTupleType Forum::Helpers::handlerToObjAndStatus(CommandHandlerRef& handler, Command command,
                                                          DisplaySettings displaySettings,
                                                          const std::vector<std::string>& parameters)
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


std::string Forum::Helpers::createUserAndGetId(CommandHandlerRef& handler, const std::string& name)
{
    auto result = handlerToObj(handler, Command::ADD_USER, { name });
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
