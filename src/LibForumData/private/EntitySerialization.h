#pragma once

#include "AuthorizationGrantedPrivilegeStore.h"
#include "JsonWriter.h"

#include <boost/optional.hpp>

namespace Json
{
    JsonWriter& operator<<(JsonWriter& writer, const Forum::Entities::EntitiesCount& thread);

    JsonWriter& operator<<(JsonWriter& writer, const Forum::Entities::UuidString& id);
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

            boost::optional<bool> allowDisplayDiscussionThreadMessage = boost::none;
            boost::optional<bool> allowDisplayDiscussionThreadMessageUser = boost::none;
            boost::optional<bool> allowDisplayDiscussionThreadMessageVotes = boost::none;
            boost::optional<bool> allowDisplayDiscussionThreadMessageIpAddress = boost::none;

            bool hideLatestMessage = false;
            bool hidePrivileges = false;
        };

        extern thread_local SerializationSettings serializationSettings;

        Json::JsonWriter& serialize(Json::JsonWriter& writer, const DiscussionThreadMessage& thread,
                                    const Authorization::SerializationRestriction& restriction);

        Json::JsonWriter& serialize(Json::JsonWriter& writer, const DiscussionThread& thread,
                                    const Authorization::SerializationRestriction& restriction);

        Json::JsonWriter& serialize(Json::JsonWriter& writer, const DiscussionTag& tag,
                                    const Authorization::SerializationRestriction& restriction);

        Json::JsonWriter& serialize(Json::JsonWriter& writer, const DiscussionCategory& category,
                                    const Authorization::SerializationRestriction& restriction);

        /**
         * The restriction parameter is not yet used by these two functions, just keeping a uniform interface
         */
        Json::JsonWriter& serialize(Json::JsonWriter& writer, const MessageComment& messageComment,
                                    const Authorization::SerializationRestriction& restriction);

        Json::JsonWriter& serialize(Json::JsonWriter& writer, const User& user,
                                    const Authorization::SerializationRestriction& restriction);

        template<typename Entity, typename PrivilegeArray>
        static void writePrivileges(Json::JsonWriter& writer, const Entity& entity, const PrivilegeArray& privilegeArray,
                                    const Authorization::SerializationRestriction& restriction)
        {
            writer.newPropertyWithSafeName("privileges");
            writer.startArray();
            for (auto& tuple : privilegeArray)
            {
                if (restriction.isAllowed(entity, std::get<0>(tuple)))
                {
                    writer.writeSafeString(std::get<1>(tuple));
                }
            }
            writer.endArray();
        }
    }
}
