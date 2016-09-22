#include <string>
#include <sstream>

#include <boost/property_tree/json_parser.hpp>

#include "CommandsCommon.h"
#include "MemoryRepository.h"

using namespace Forum::Commands;
using namespace Forum::Repository;

std::shared_ptr<CommandHandler> Forum::Helpers::createCommandHandler()
{
    auto memoryRepository = std::make_shared<MemoryRepository>();
    return std::make_shared<CommandHandler>(memoryRepository, memoryRepository);
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
