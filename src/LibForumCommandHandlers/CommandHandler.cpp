#include "CommandHandler.h"
#include "JsonWriter.h"
#include "Version.h"

using namespace Forum::Commands;

CommandHandler::CommandHandler()
{
    handlers_[SHOW_VERSION] = std::bind(&CommandHandler::version, this, std::placeholders::_1, std::placeholders::_2);
}

void CommandHandler::handle(Command command, const std::vector<std::string>& parameters, std::ostream& output)
{
    if (command >= 0 && command < LAST_COMMAND)
    {
        handlers_[command](parameters, output);
    }
}

void CommandHandler::version(const std::vector<std::string>& parameters, std::ostream& output)
{
    Json::JsonWriter writer(output);
    writer
        << Json::objStart
            << Json::propertySafeName("version", VERSION)
        << Json::objEnd;
}
