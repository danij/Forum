#include <string>
#include <sstream>

#include <boost/property_tree/json_parser.hpp>

#include "CommandsCommon.h"
#include "MemoryRepository.h"
#include "MetricsRepository.h"

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

boost::property_tree::ptree Forum::Helpers::handlerToObj(CommandHandlerRef handler, Command command)
{
    return handlerToObj(handler, command, {});
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
