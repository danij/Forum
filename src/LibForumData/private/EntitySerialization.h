/*
Fast Forum Backend
Copyright (C) 2016-2017 Daniel Jurcau

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
        struct SerializationSettings final
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
            bool keepDiscussionCategoryDetails = false;
            boost::optional<int> displayDiscussionCategoryParentRecursionDepth = boost::none;

            boost::optional<bool> allowDisplayDiscussionThreadMessage = boost::none;
            boost::optional<bool> allowDisplayDiscussionThreadMessageUser = boost::none;
            boost::optional<bool> allowDisplayDiscussionThreadMessageVotes = boost::none;
            boost::optional<bool> allowDisplayDiscussionThreadMessageIpAddress = boost::none;

            bool hideLatestMessage = false;
            bool hidePrivileges = false;
            bool onlySendCategoryParentId = false;
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

        template<typename Entity, typename PrivilegeArray, typename PrivilegeStringArray>
        static void writePrivileges(Json::JsonWriter& writer, const Entity& entity,
                                    const PrivilegeArray& privilegeArray,
                                    const PrivilegeStringArray& privilegeStrings,
                                    const Authorization::SerializationRestriction& restriction)
        {
            writer.newPropertyWithSafeName("privileges");
            writer.startArray();
            for (auto& value : privilegeArray)
            {
                if (restriction.isAllowed(entity, value))
                {
                    writer.writeSafeString(privilegeStrings[static_cast<int>(value)]);
                }
            }
            writer.endArray();
        }
    }
}
