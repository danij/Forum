#pragma once

#include "Entities.h"
#include "JsonWriter.h"

#include <boost/optional.hpp>

namespace Json
{
    JsonWriter& operator<<(JsonWriter& writer, const Forum::Entities::EntitiesCount& thread);

    JsonWriter& operator<<(JsonWriter& writer, const Forum::Entities::UuidString& id);
    JsonWriter& operator<<(JsonWriter& writer, const Forum::Entities::User& user);
    JsonWriter& operator<<(JsonWriter& writer, const Forum::Entities::DiscussionThreadMessage& thread);
    JsonWriter& operator<<(JsonWriter& writer, const Forum::Entities::MessageComment& messageComment);
    JsonWriter& operator<<(JsonWriter& writer, const Forum::Entities::DiscussionThread& thread);
    JsonWriter& operator<<(JsonWriter& writer, const Forum::Entities::DiscussionTag& tag);
    JsonWriter& operator<<(JsonWriter& writer, const Forum::Entities::DiscussionCategory& category);
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

            bool hideMessageCommentMessage = false;
            bool hideMessageCommentUser = false;

            bool hideVisitedThreadSinceLastChange = false;
            bool visitedThreadSinceLastChange = false;

            bool hideDiscussionCategoryTags = false;
            bool hideDiscussionCategoryParent = false;
            bool showDiscussionCategoryChildren = false;
            bool hideDiscussionCategoriesOfTags = false;
            boost::optional<int> displayDiscussionCategoryParentRecursionDepth = boost::none;

            bool hideLatestMessage = false;
        };

        extern thread_local SerializationSettings serializationSettings;
    }
}