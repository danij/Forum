#include "CommandsCommon.h"

#include "MemoryRepository.h"
#include "MetricsRepository.h"
#include "TestHelpers.h"

#include <boost/property_tree/json_parser.hpp>

#include <sstream>
#include <string>

using namespace Forum::Commands;
using namespace Forum::Repository;

std::shared_ptr<CommandHandler> Forum::Helpers::createCommandHandler()
{
    auto memoryRepository = std::make_shared<MemoryRepository>();
    auto metricsRepository = std::make_shared<MetricsRepository>();
    return std::make_shared<CommandHandler>(memoryRepository, memoryRepository, metricsRepository);
}

std::string Forum::Helpers::handlerToString(CommandHandlerRef handler, Command command,
                                            const std::vector<std::string>& parameters)
{
    std::stringstream stream;
    handler->handle(command, parameters, stream);
    return stream.str();
}

std::string Forum::Helpers::handlerToString(CommandHandlerRef handler, Command command)
{
    return handlerToString(handler, command, {});
}

boost::property_tree::ptree Forum::Helpers::handlerToObj(CommandHandlerRef handler, Command command,
                                                         const std::vector<std::string>& parameters)
{
    std::stringstream stream;
    handler->handle(command, parameters, stream);
    boost::property_tree::ptree result;
    boost::property_tree::read_json(stream, result);
    return result;
}

boost::property_tree::ptree Forum::Helpers::handlerToObj(CommandHandlerRef handler, Command command,
                                                         Context::SortOrder sortOrder,
                                                         const std::vector<std::string>& parameters)
{
    auto oldSortOrder = Context::getDisplayContext().sortOrder;
    auto disposer = createDisposer([oldSortOrder]() { Context::getMutableDisplayContext().sortOrder = oldSortOrder; });
    Context::getMutableDisplayContext().sortOrder = sortOrder;

    return handlerToObj(handler, command, parameters);
}

boost::property_tree::ptree Forum::Helpers::handlerToObj(CommandHandlerRef handler, Command command)
{
    return handlerToObj(handler, command, {});
}

boost::property_tree::ptree Forum::Helpers::handlerToObj(CommandHandlerRef handler, Command command, 
                                                         Context::SortOrder sortOrder)
{
    return handlerToObj(handler, command, sortOrder, {});
}

std::string Forum::Helpers::createUserAndGetId(CommandHandlerRef handler, const std::string& name)
{
    auto result = handlerToObj(handler, Command::ADD_USER, { name });
    return result.get<std::string>("id");
}

std::string Forum::Helpers::createDiscussionThreadAndGetId(CommandHandlerRef handler, const std::string& name)
{
    auto result = handlerToObj(handler, Command::ADD_DISCUSSION_THREAD, { name });
    return result.get<std::string>("id");
}

std::string Forum::Helpers::createDiscussionMessageAndGetId(CommandHandlerRef handler, const std::string& threadId,
                                                            const std::string& content)
{
    auto result = handlerToObj(handler, Command::ADD_DISCUSSION_THREAD_MESSAGE, { threadId, content });
    return result.get<std::string>("id");
}

std::string Forum::Helpers::createDiscussionTagAndGetId(CommandHandlerRef handler, const std::string& name)
{
    auto result = handlerToObj(handler, Command::ADD_DISCUSSION_TAG, { name });
    return result.get<std::string>("id");
}
