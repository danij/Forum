#include "EntitySerialization.h"

using namespace Forum::Entities;
using namespace Json;

thread_local SerializationSettings Forum::Entities::serializationSettings = {};

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
            << propertySafeName("threadCount", user.threadsById().size())
        << objEnd;
    return writer;
}

JsonWriter& Json::operator<<(JsonWriter& writer, const Forum::Entities::DiscussionThread& thread)
{
    writer
        << objStart
            << propertySafeName("id", thread.id())
            << propertySafeName("name", thread.name())
            << propertySafeName("created", thread.created());
    if ( ! serializationSettings.hideDiscussionThreadCreatedBy)
    {
        writer << propertySafeName("createdBy", thread.createdBy());
    }
    writer  << propertySafeName("lastUpdated", thread.lastUpdated())
        << objEnd;
    return writer;
}
