#pragma once

#include "Entities.h"
#include "JsonWriter.h"

namespace Json
{
    Json::JsonWriter& operator<<(Json::JsonWriter& writer, const Forum::Entities::EntitiesCount& thread);

    Json::JsonWriter& operator<<(Json::JsonWriter& writer, const Forum::Entities::IdType& id);
    Json::JsonWriter& operator<<(Json::JsonWriter& writer, const Forum::Entities::User& user);
    Json::JsonWriter& operator<<(Json::JsonWriter& writer, const Forum::Entities::DiscussionMessage& thread);
    Json::JsonWriter& operator<<(Json::JsonWriter& writer, const Forum::Entities::DiscussionThread& thread);
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
        };

        extern thread_local SerializationSettings serializationSettings;
    }
}