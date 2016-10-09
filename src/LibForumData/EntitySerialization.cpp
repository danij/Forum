#include "EntitySerialization.h"

using namespace Forum::Entities;
using namespace Json;

JsonWriter& Json::operator<<(JsonWriter& writer, const Forum::Entities::User& user)
{
    writer
        << objStart
            << propertySafeName("id", user.id())
            << propertySafeName("name", user.name())
            << propertySafeName("created", user.created())
        << objEnd;
    return writer;
}
