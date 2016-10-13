#include "EntitySerialization.h"

using namespace Forum::Entities;
using namespace Json;

JsonWriter& Json::operator<<(JsonWriter& writer, const Forum::Entities::IdType& id)
{
    return writer.writeSafeString(id);
}

JsonWriter& Json::operator<<(JsonWriter& writer, const Forum::Entities::User& user)
{
    writer
        << objStart
            << propertySafeName("id", user.id())
            << propertySafeName("name", user.name())
            << propertySafeName("created", user.created())
            << propertySafeName("lastSeen", user.lastSeen())
        << objEnd;
    return writer;
}
