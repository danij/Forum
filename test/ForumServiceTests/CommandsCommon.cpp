#include "CommandsCommon.h"

#include "MemoryRepository.h"
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
    auto memoryRepository = std::make_shared<MemoryRepository>();
    auto metricsRepository = std::make_shared<MetricsRepository>();
    return std::make_shared<CommandHandler>(memoryRepository, memoryRepository, metricsRepository);
}

std::string Forum::Helpers::handlerToString(CommandHandlerRef& handler, Command command,
                                            const std::vector<std::string>& parameters)
{
    std::stringstream stream;
    handler->handle(command, parameters, stream);
    return stream.str();
}

std::string Forum::Helpers::handlerToString(CommandHandlerRef& handler, Command command)
{
    return handlerToString(handler, command, {});
}

TreeType Forum::Helpers::handlerToObj(CommandHandlerRef& handler, Command command,
                                      const std::vector<std::string>& parameters)
{
    std::stringstream stream;
    handler->handle(command, parameters, stream);

    boost::property_tree::ptree result;
    boost::property_tree::read_json(stream, result);

    return result;
}

TreeType Forum::Helpers::handlerToObj(CommandHandlerRef& handler, Command command, DisplaySettings displaySettings,
                                      const std::vector<std::string>& parameters)
{
    auto oldPageNumber = Context::getDisplayContext().pageNumber;
    auto oldSortOrder = Context::getDisplayContext().sortOrder;

    auto _ = createDisposer([oldPageNumber]() { Context::getMutableDisplayContext().pageNumber = oldPageNumber; });
    auto ___ = createDisposer([oldSortOrder]() { Context::getMutableDisplayContext().sortOrder = oldSortOrder; });

    Context::getMutableDisplayContext().pageNumber = displaySettings.pageNumber;
    Context::getMutableDisplayContext().sortOrder = displaySettings.sortOrder;

    return handlerToObj(handler, command, parameters);
}

TreeType Forum::Helpers::handlerToObj(CommandHandlerRef& handler, Command command)
{
    return handlerToObj(handler, command, DisplaySettings{});
}

TreeType Forum::Helpers::handlerToObj(CommandHandlerRef& handler, Command command, DisplaySettings displaySettings)
{
    return handlerToObj(handler, command, displaySettings, {});
}


TreeStatusTupleType Forum::Helpers::handlerToObjAndStatus(CommandHandlerRef& handler, Command command,
                                                          const std::vector<std::string>& parameters)
{
    std::stringstream stream;
    auto statusCode = handler->handle(command, parameters, stream);

    boost::property_tree::ptree result;
    boost::property_tree::read_json(stream, result);

    return std::make_tuple(result, statusCode);
}

TreeStatusTupleType Forum::Helpers::handlerToObjAndStatus(CommandHandlerRef& handler, Command command,
                                                          DisplaySettings displaySettings,
                                                          const std::vector<std::string>& parameters)
{
    auto oldPageNumber = Context::getDisplayContext().pageNumber;
    auto oldSortOrder = Context::getDisplayContext().sortOrder;

    auto _ = createDisposer([oldPageNumber]() { Context::getMutableDisplayContext().pageNumber = oldPageNumber; });
    auto ___ = createDisposer([oldSortOrder]() { Context::getMutableDisplayContext().sortOrder = oldSortOrder; });

    Context::getMutableDisplayContext().pageNumber = displaySettings.pageNumber;
    Context::getMutableDisplayContext().sortOrder = displaySettings.sortOrder;

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
