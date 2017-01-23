#pragma once

#include "Entities.h"
#include "JsonWriter.h"

namespace Json
{
    JsonWriter& operator<<(JsonWriter& writer, const Forum::Entities::EntitiesCount& thread);

    JsonWriter& operator<<(JsonWriter& writer, const Forum::Entities::IdType& id);
    JsonWriter& operator<<(JsonWriter& writer, const Forum::Entities::User& user);
    JsonWriter& operator<<(JsonWriter& writer, const Forum::Entities::DiscussionThreadMessage& thread);
    JsonWriter& operator<<(JsonWriter& writer, const Forum::Entities::DiscussionThread& thread);
}

namespace Forum
{
    namespace Entities
    {
        struct SerializationSettings
        {
            bool hideDiscussionThreadCreatedBy = false;
            bool hideDiscussionThreadMessages = false;
            bool hideDiscussionThreadMessageCreatedBy = false;
            bool hideDiscussionThreadMessageParentThread = false;

            bool hideVisitedThreadSinceLastChange = false;
            bool visitedThreadSinceLastChange = false;
        };

        extern thread_local SerializationSettings serializationSettings;
    }
}