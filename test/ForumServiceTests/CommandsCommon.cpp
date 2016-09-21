#include <string>
#include <sstream>

#include <boost/property_tree/json_parser.hpp>

#include "CommandsCommon.h"

using namespace Forum::Commands;

std::shared_ptr<CommandHandler> Forum::Helpers::createCommandHandler()
{
    return std::make_shared<CommandHandler>();
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
