#include "EntitySerialization.h"

using namespace Forum::Entities;
using namespace Json;

thread_local SerializationSettings Forum::Entities::serializationSettings = {};

JsonWriter& Json::operator<<(JsonWriter& writer, const EntitiesCount& value)
{
    writer
        << objStart
            << propertySafeName("users", value.nrOfUsers)
            << propertySafeName("discussionThreads", value.nrOfDiscussionThreads)
            << propertySafeName("discussionMessages", value.nrOfDiscussionMessages)
        << objEnd;
    return writer;
}

JsonWriter& Json::operator<<(JsonWriter& writer, const IdType& id)
{
    return writer.writeSafeString(id);
}

JsonWriter& Json::operator<<(JsonWriter& writer, const User& user)
{
    writer
        << objStart
            << propertySafeName("id", user.id())
            << propertySafeName("name", user.name())
            << propertySafeName("created", user.created())
            << propertySafeName("lastSeen", user.lastSeen())
            << propertySafeName("threadCount", user.threadsById().size())
            << propertySafeName("messageCount", user.messagesById().size())
        << objEnd;
    return writer;
}

JsonWriter& Json::operator<<(JsonWriter& writer, const DiscussionMessage& message)
{
    writer
        << objStart
            << propertySafeName("id", message.id())
            << propertySafeName("content", message.content())
            << propertySafeName("created", message.created())
            << propertySafeName("createdBy", message.createdBy())
        << objEnd;
    return writer;
}

JsonWriter& Json::operator<<(JsonWriter& writer, const DiscussionThread& thread)
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

    const auto& messagesIndex = thread.messagesByCreated();
    if ( ! serializationSettings.hideDiscussionThreadMessages)
    {
        writer << propertySafeName("messages", enumerate(messagesIndex.begin(), messagesIndex.end()));
    }

    writer  << propertySafeName("lastUpdated", thread.lastUpdated())
            << propertySafeName("visited", thread.visited().load())
        << objEnd;
    return writer;
}
