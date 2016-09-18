#include "CommandHandlers.h"
#include "JsonWriter.h"
#include "Version.h"

using namespace Forum;

void Commands::version(const std::vector<std::string>& parameters, std::ostream& output)
{
    Json::JsonWriter writer(output);
    writer
        << Json::objStart
            << Json::propertySafeName("version", VERSION)
        << Json::objEnd;
}
