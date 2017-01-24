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

template<typename Collection>
static JsonWriter& writeVotes(JsonWriter& writer, const char* name, const Collection& votes)
{
    writer.newPropertyWithSafeName(name);
    writer.startArray();
    for (auto& pair : votes)
    {
        if (auto user = pair.first.lock())
        {
            writer << objStart
                << propertySafeName("userId", user->id())
                << propertySafeName("userName", user->name())
                << propertySafeName("at", pair.second)
                << objEnd;
        }
    }
    writer.endArray();
    return writer;
}

JsonWriter& Json::operator<<(JsonWriter& writer, const DiscussionThreadMessage& message)
{
    writer
        << objStart
            << propertySafeName("id", message.id())
            << propertySafeName("content", message.content())
            << propertySafeName("created", message.created());

    if ( ! serializationSettings.hideDiscussionThreadCreatedBy)
    {
        writer << propertySafeName("createdBy", message.createdBy());
    }
    if ( ! serializationSettings.hideDiscussionThreadMessageParentThread)
    {
        writer << propertySafeName("parentThread", message.parentThread());
    }

    if (message.lastUpdated())
    {
        writer.newPropertyWithSafeName("lastUpdated");
        writer.startObject();
        auto& details = message.lastUpdatedDetails();
        if (auto ptr = details.by.lock())
        {
            writer << propertySafeName("userId", ptr->id())
                   << propertySafeName("userName", ptr->name());
        }
        writer << propertySafeName("at", message.lastUpdated());
        writer << propertySafeName("ip", details.ip);
        writer << propertySafeName("userAgent", details.userAgent);

        writer.endObject();
    }

    writer << propertySafeName("ip", message.creationDetails().ip)
           << propertySafeName("userAgent", message.creationDetails().userAgent);
    
    writeVotes(writer, "upVotes", message.upVotes());
    writeVotes(writer, "downVotes", message.downVotes());

    writer << objEnd;
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
    auto messageCount = messagesIndex.size();

    writer  << propertySafeName("messageCount", messageCount);

    if (messageCount)
    {
        auto& latestMessage = **messagesIndex.rbegin();
        writer.newPropertyWithSafeName("latestMessage");
        writer << objStart
                    << propertySafeName("created", latestMessage.created())
                    << propertySafeName("createdBy", latestMessage.createdBy())
               << objEnd;
    }
    if ( ! serializationSettings.hideDiscussionThreadMessages)
    {
        writer << propertySafeName("messages", enumerate(messagesIndex.begin(), messagesIndex.end()));
    }
    if ( ! serializationSettings.hideVisitedThreadSinceLastChange)
    {
        writer << propertySafeName("visitedSinceLastChange", serializationSettings.visitedThreadSinceLastChange);
    }

    writer  << propertySafeName("lastUpdated", thread.lastUpdated())
            << propertySafeName("visited", thread.visited().load())
            << propertySafeName("voteScore", thread.voteScore())
    
        << objEnd;
    return writer;
}
