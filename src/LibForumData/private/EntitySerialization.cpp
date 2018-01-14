/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

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

#include "Configuration.h"
#include "EntitySerialization.h"
#include "StateHelpers.h"
#include "OutputHelpers.h"

#include <type_traits>

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Authorization;
using namespace Json;

thread_local SerializationSettings Entities::serializationSettings = {};

JsonWriter& Json::operator<<(JsonWriter& writer, const EntitiesCount& value)
{
    writer
        << objStart
            << propertySafeName("users", value.nrOfUsers)
            << propertySafeName("discussionThreads", value.nrOfDiscussionThreads)
            << propertySafeName("discussionMessages", value.nrOfDiscussionMessages)
            << propertySafeName("discussionTags", value.nrOfDiscussionTags)
            << propertySafeName("discussionCategories", value.nrOfDiscussionCategories)
        << objEnd;
    return writer;
}

JsonWriter& Json::operator<<(JsonWriter& writer, const UuidString& id)
{
    char buffer[UuidString::StringRepresentationSize];

    id.toString(buffer);

    return writer.writeSafeString(buffer, std::extent<decltype(buffer)>::value);
}

JsonWriter& Entities::serialize(JsonWriter& writer, const User& user, const SerializationRestriction& restriction)
{
    (void)restriction;
    writer
        << objStart
            << propertySafeName("id", user.id())
            << propertySafeName("name", user.name())
            << propertySafeName("info", user.info())
            << propertySafeName("title", user.title())
            << propertySafeName("signature", user.signature())
            << propertySafeName("hasLogo", user.hasLogo())
            << propertySafeName("created", user.created())
            << propertySafeName("lastSeen", user.lastSeen())
            << propertySafeName("threadCount", user.threads().byId().size())
            << propertySafeName("messageCount", user.threadMessages().byId().size())
            << propertySafeName("subscribedThreadCount", user.subscribedThreads().byId().size())
            << propertySafeName("receivedUpVotes", user.receivedUpVotes())
            << propertySafeName("receivedDownVotes", user.receivedDownVotes())
        << objEnd;
    return writer;
}

template<typename Collection, size_t NameSize>
static JsonWriter& writeVotes(JsonWriter& writer, const char(&name)[NameSize], const Collection& votes)
{
    writer.newPropertyWithSafeName(name);
    writer.startArray();
    for (const auto& pair : votes)
    {
        if (auto user = pair.first)
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

JsonWriter& writeVisitDetails(JsonWriter& writer, const VisitDetails& visitDetails)
{
    //does not currently start a new object
    char buffer[IpAddress::MaxIPv6CharacterCount + 1];
    writer.newPropertyWithSafeName("ip");

    auto addressLength = visitDetails.ip.toString(buffer, std::extent<decltype(buffer)>::value);

    writer.writeSafeString(buffer, addressLength);
    return writer;
}

JsonWriter& Entities::serialize(JsonWriter& writer, const DiscussionThreadMessage& message,
                                const SerializationRestriction& restriction)
{
    auto allowView = serializationSettings.allowDisplayDiscussionThreadMessage
        ? *serializationSettings.allowDisplayDiscussionThreadMessage
        : restriction.isAllowed(message, DiscussionThreadMessagePrivilege::VIEW);

    if ( ! allowView) return writer.null();

    auto allowViewUser = serializationSettings.allowDisplayDiscussionThreadMessageUser
        ? *serializationSettings.allowDisplayDiscussionThreadMessageUser
        : restriction.isAllowed(message, DiscussionThreadMessagePrivilege::VIEW_CREATOR_USER);

    auto allowViewVotes = serializationSettings.allowDisplayDiscussionThreadMessageVotes
        ? *serializationSettings.allowDisplayDiscussionThreadMessageVotes
        : restriction.isAllowed(message, DiscussionThreadMessagePrivilege::VIEW_VOTES);

    auto allowViewIpAddress = serializationSettings.allowDisplayDiscussionThreadMessageIpAddress
        ? *serializationSettings.allowDisplayDiscussionThreadMessageIpAddress
        : restriction.isAllowed(message, DiscussionThreadMessagePrivilege::VIEW_IP_ADDRESS);

    auto allowViewCommentCount = serializationSettings.allowDisplayDiscussionThreadMessageComments
        ? *serializationSettings.allowDisplayDiscussionThreadMessageComments
        : restriction.isAllowed(message, DiscussionThreadMessagePrivilege::GET_MESSAGE_COMMENTS);

    writer
        << objStart
            << propertySafeName("id", message.id())
            << propertySafeName("created", message.created());

    if (allowViewCommentCount)
    {
        writer << propertySafeName("commentsCount", message.comments().count())
               << propertySafeName("solvedCommentsCount", message.solvedCommentsCount());
    }

    auto content = message.content();
    writer.newPropertyWithSafeName("content").writeEscapedString(content.data(), content.size());

    if (allowViewUser && ( ! serializationSettings.hideDiscussionThreadMessageCreatedBy))
    {
        BoolTemporaryChanger _(serializationSettings.hidePrivileges, true);

        writer.newPropertyWithSafeName("createdBy");
        serialize(writer, message.createdBy(), restriction);
    }
    if ( ! serializationSettings.hideDiscussionThreadMessageParentThread)
    {
        DiscussionThreadConstPtr parentThread = message.parentThread();
        assert(parentThread);

        BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadMessages, true);
        BoolTemporaryChanger __(serializationSettings.hidePrivileges, true);

        writer.newPropertyWithSafeName("parentThread");
        serialize(writer, *parentThread, restriction);
    }

    if (message.lastUpdated())
    {
        writer.newPropertyWithSafeName("lastUpdated");
        writer.startObject();

        UserConstPtr by = message.lastUpdatedBy();
        if (by && allowViewUser)
        {
            writer << propertySafeName("userId", by->id())
                    << propertySafeName("userName", by->name());
        }
        writer << propertySafeName("at", message.lastUpdated())
               << propertySafeName("reason", message.lastUpdatedReason());
        if (allowViewIpAddress)
        {
            writeVisitDetails(writer, message.lastUpdatedDetails());
        }

        writer.endObject();
    }

    if (allowViewIpAddress)
    {
        writeVisitDetails(writer, message.creationDetails());
    }

    if (allowViewVotes)
    {
        writeVotes(writer, "upVotes", message.upVotes());
        writeVotes(writer, "downVotes", message.downVotes());
    }

    if ( ! serializationSettings.hidePrivileges)
    {
        writePrivileges(writer, message, DiscussionThreadMessagePrivilegesToSerialize,
                        DiscussionThreadMessagePrivilegeStrings, restriction);
    }

    writer << objEnd;
    return writer;
}

static void writeLatestMessage(JsonWriter& writer, const DiscussionThreadMessage& latestMessage,
                               const SerializationRestriction& restriction)
{
    auto parentThread = latestMessage.parentThread();
    assert(parentThread);

    writer.newPropertyWithSafeName("latestMessage");
    writer << objStart
        << propertySafeName("id", latestMessage.id())
        << propertySafeName("created", latestMessage.created())
        << propertySafeName("threadId", parentThread->id())
        << propertySafeName("threadName", parentThread->name());

    auto content = latestMessage.content();
    writer.newPropertyWithSafeName("content").writeEscapedString(content.data(), content.size());

    {
        writer.newPropertyWithSafeName("createdBy");
        BoolTemporaryChanger _(serializationSettings.hidePrivileges, true);
        serialize(writer, latestMessage.createdBy(), restriction);
    }
    writer << objEnd;
}

template<typename ThreadCollection>
static void writeLatestMessage(JsonWriter& writer, const ThreadCollection& threads,
                               const SerializationRestriction& restriction)
{
    auto index = threads.byLatestMessageCreated();
    if ( ! index.size())
    {
        return;
    }
    auto thread = *(index.rbegin());
    auto messageIndex = thread->messages().byCreated();
    if (messageIndex.size())
    {
        writeLatestMessage(writer, **messageIndex.rbegin(), restriction);
    }
}

JsonWriter& Entities::serialize(JsonWriter& writer, const MessageComment& comment,
                                const SerializationRestriction& restriction)
{
    writer
        << objStart
        << propertySafeName("id", comment.id())
        << propertySafeName("created", comment.created())
        << propertySafeName("solved", comment.solved());

    auto content = comment.content();
    writer.newPropertyWithSafeName("content").writeEscapedString(content.data(), content.size());

    writeVisitDetails(writer, comment.creationDetails());

    if ( ! serializationSettings.hideMessageCommentMessage)
    {
        BoolTemporaryChanger _(serializationSettings.hidePrivileges, true);

        writer.newPropertyWithSafeName("message");
        serialize(writer, comment.parentMessage(), restriction);
    }
    if ( ! serializationSettings.hideMessageCommentUser)
    {
        BoolTemporaryChanger _(serializationSettings.hidePrivileges, true);

        writer.newPropertyWithSafeName("createdBy");
        serialize(writer, comment.createdBy(), restriction);
    }

    writer << objEnd;
    return writer;
}

//Overload to reduce the number of calls to the authorization engine for checking rights to each message
template<typename Collection, size_t PropertyNameSize>
void writeDiscussionThreadMessages(const Collection& collection, int_fast32_t pageNumber, int_fast32_t pageSize,
                                   bool ascending, const char(&propertyName)[PropertyNameSize], JsonWriter& writer,
                                   const SerializationRestriction& restriction)
{
    auto totalCount = static_cast<int_fast32_t>(collection.size());

    writer << propertySafeName("totalCount", totalCount)
           << propertySafeName("pageSize", pageSize)
           << propertySafeName("page", pageNumber);

    static thread_local std::vector<DiscussionThreadMessagePrivilegeCheck> privilegeChecks(100);

    privilegeChecks.clear();

    auto firstElementIndex = std::max(static_cast<decltype(totalCount)>(0),
                                      static_cast<decltype(totalCount)>(pageNumber * pageSize));
    if (ascending)
    {
        for (auto it = collection.nth(firstElementIndex), n = collection.nth(firstElementIndex + pageSize); it != n; ++it)
        {
            if (*it)
            {
                privilegeChecks.emplace_back(DiscussionThreadMessagePrivilegeCheck(restriction.userId(), **it));
            }
        }
    }
    else
    {
        auto itStart = collection.nth(std::max(totalCount - firstElementIndex,
            static_cast<decltype(totalCount)>(0)));
        auto itEnd = collection.nth(std::max(totalCount - firstElementIndex - pageSize,
            static_cast<decltype(totalCount)>(0)));

        if (itStart != collection.begin())
        {
            for (auto it = itStart; it != itEnd;)
            {
                --it;
                if (*it)
                {
                    privilegeChecks.emplace_back(DiscussionThreadMessagePrivilegeCheck(restriction.userId(), **it));
                }
            }
        }
    }

    restriction.privilegeStore().computeDiscussionThreadMessageVisibilityAllowed(privilegeChecks.data(),
                                                                                 privilegeChecks.size(),
                                                                                 restriction.now());

    writer.newPropertyWithSafeName(propertyName, PropertyNameSize - 1);
    writer.startArray();
    for (auto& item : privilegeChecks)
    {
        if ( ! item.allowedToShowMessage) continue;
        if ( ! item.message) continue;

        OptionalRevertToNoneChanger<bool> _(serializationSettings.allowDisplayDiscussionThreadMessage,
                                            item.allowedToShowMessage);
        OptionalRevertToNoneChanger<bool> __(serializationSettings.allowDisplayDiscussionThreadMessageUser,
                                             item.allowedToShowUser);
        OptionalRevertToNoneChanger<bool> ___(serializationSettings.allowDisplayDiscussionThreadMessageVotes,
                                              item.allowedToShowVotes);
        OptionalRevertToNoneChanger<bool> ____(serializationSettings.allowDisplayDiscussionThreadMessageIpAddress,
                                               item.allowedToShowIpAddress);
        OptionalRevertToNoneChanger<bool> _____(serializationSettings.allowDisplayDiscussionThreadMessageComments,
                                                item.allowedToViewComments);

        serialize(writer, *item.message, restriction);
    }
    writer.endArray();
}


JsonWriter& Entities::serialize(JsonWriter& writer, const DiscussionThread& thread,
                                const SerializationRestriction& restriction)
{
    if ( ! restriction.isAllowed(thread)) return writer.null();

    writer
        << objStart
            << propertySafeName("id", thread.id())
            << propertySafeName("name", thread.name())
            << propertySafeName("created", thread.created())
            << propertySafeName("latestVisibleChangeAt", thread.latestVisibleChange())
            << propertySafeName("pinned", thread.pinDisplayOrder() > 0)
            << propertySafeName("pinDisplayOrder", thread.pinDisplayOrder())
            << propertySafeName("subscribedUsersCount", thread.subscribedUsersCount());

    IdTypeRef currentUserId = Context::getCurrentUserId();
    writer << propertySafeName("subscribedToThread",
        currentUserId && (thread.subscribedUsers().find(currentUserId) != thread.subscribedUsers().end()));

    if ( ! serializationSettings.hideDiscussionThreadCreatedBy)
    {
        BoolTemporaryChanger _(serializationSettings.hidePrivileges, true);

        writer.newPropertyWithSafeName("createdBy");
        serialize(writer, thread.createdBy(), restriction);
    }

    const auto& messagesIndex = thread.messages().byCreated();
    auto messageCount = messagesIndex.size();

    writer << propertySafeName("messageCount", messageCount);

    if (messageCount && ! serializationSettings.hideLatestMessage)
    {
        BoolTemporaryChanger _(serializationSettings.hidePrivileges, true);

        writeLatestMessage(writer, **messagesIndex.rbegin(), restriction);
    }
    if ( ! serializationSettings.hideDiscussionThreadMessages)
    {
        auto pageSize = getGlobalConfig()->discussionThreadMessage.maxMessagesPerPage;
        auto& displayContext = Context::getDisplayContext();

        writeDiscussionThreadMessages(messagesIndex, displayContext.pageNumber, pageSize, true, "messages", writer,
                                      restriction);
    }
    if ( ! serializationSettings.hideVisitedThreadSinceLastChange)
    {
        writer << propertySafeName("visitedSinceLastChange", serializationSettings.visitedThreadSinceLastChange);
    }

    {
        BoolTemporaryChanger _(serializationSettings.hideDiscussionCategoriesOfTags, true);
        BoolTemporaryChanger __(serializationSettings.hideLatestMessage, true);
        BoolTemporaryChanger ___(serializationSettings.hidePrivileges, true);

        writer.newPropertyWithSafeName("tags");
        writer << arrayStart;
        for (auto tag : thread.tags())
        {
            serialize(writer, *tag, restriction);
        }
        writer << arrayEnd;
    }
    {
        BoolTemporaryChanger _(serializationSettings.hideDiscussionCategoryParent, true);
        BoolTemporaryChanger __(serializationSettings.hideDiscussionCategoryTags, true);
        BoolTemporaryChanger ___(serializationSettings.hideLatestMessage, true);
        BoolTemporaryChanger ____(serializationSettings.hidePrivileges, true);

        writer.newPropertyWithSafeName("categories");
        writer << arrayStart;
        for (auto category : thread.categories())
        {
            serialize(writer, *category, restriction);
        }
        writer << arrayEnd;
    }
    writer << propertySafeName("lastUpdated", thread.lastUpdated())
           << propertySafeName("visited", thread.visited().load())
           << propertySafeName("voteScore", thread.voteScore());

    if ( ! serializationSettings.hidePrivileges)
    {
        writePrivileges(writer, thread, DiscussionThreadPrivilegesToSerialize,
                        DiscussionThreadPrivilegeStrings,restriction);
    }

    writer << objEnd;
    return writer;
}

JsonWriter& Entities::serialize(JsonWriter& writer, const DiscussionTag& tag,
                                const SerializationRestriction& restriction)
{
    if ( ! restriction.isAllowed(tag)) return writer.null();

    writer << objStart
        << propertySafeName("id", tag.id())
        << propertySafeName("name", tag.name())
        << propertySafeName("created", tag.created())
        << propertySafeName("threadCount", tag.threads().count())
        << propertySafeName("messageCount", tag.messageCount());

    if ( ! serializationSettings.hideLatestMessage)
    {
        BoolTemporaryChanger _(serializationSettings.hidePrivileges, true);

        writeLatestMessage(writer, tag.threads(), restriction);
    }
    if ( ! serializationSettings.hideDiscussionCategoriesOfTags)
    {
        BoolTemporaryChanger _(serializationSettings.hideDiscussionCategoryTags, true);
        BoolTemporaryChanger __(serializationSettings.hideDiscussionCategoryParent, true);
        BoolTemporaryChanger ___(serializationSettings.hidePrivileges, true);

        writer.newPropertyWithSafeName("categories");
        writer << arrayStart;
        for (auto category : tag.categories())
        {
            serialize(writer, *category, restriction);
        }
        writer << arrayEnd;
    }

    if ( ! serializationSettings.hidePrivileges)
    {
        writePrivileges(writer, tag, DiscussionTagPrivilegesToSerialize,
                        DiscussionTagPrivilegeStrings, restriction);
    }

    writer << objEnd;
    return writer;
}

JsonWriter& Entities::serialize(JsonWriter& writer, const DiscussionCategory& category,
                                const SerializationRestriction& restriction)
{
    if ( ! restriction.isAllowed(category)) return writer.null();

    writer << objStart
        << propertySafeName("id", category.id())
        << propertySafeName("name", category.name())
        << propertySafeName("description", category.description())
        << propertySafeName("displayOrder", category.displayOrder())
        << propertySafeName("created", category.created())
        << propertySafeName("threadCount", category.threads().count())
        << propertySafeName("messageCount", category.messageCount())
        << propertySafeName("threadTotalCount", category.threadTotalCount())
        << propertySafeName("messageTotalCount", category.messageTotalCount());

    if ( ! serializationSettings.hideLatestMessage)
    {
        auto latestMessage = category.latestMessage();
        if (latestMessage)
        {
            BoolTemporaryChanger _(serializationSettings.hidePrivileges, true);

            writeLatestMessage(writer, *latestMessage, restriction);
        }
    }
    if ( ! serializationSettings.hideDiscussionCategoryTags)
    {
        BoolTemporaryChanger _(serializationSettings.hideDiscussionCategoriesOfTags, true);
        BoolTemporaryChanger __(serializationSettings.hidePrivileges, true);
        BoolTemporaryChanger ___(serializationSettings.hideLatestMessage, true);

        writeArraySafeName(writer, "tags", category.tags().begin(), category.tags().end(), restriction);
    }
    if (serializationSettings.showDiscussionCategoryChildren)
    {
        //only show 1 level of category children
        auto hideDetails = ! serializationSettings.keepDiscussionCategoryDetails;

        BoolTemporaryChanger _(serializationSettings.showDiscussionCategoryChildren, false);
        BoolTemporaryChanger __(serializationSettings.hideDiscussionCategoryParent, true);
        BoolTemporaryChanger ___(serializationSettings.hideDiscussionCategoryTags, hideDetails);
        BoolTemporaryChanger ____(serializationSettings.hideLatestMessage, hideDetails);
        BoolTemporaryChanger _____(serializationSettings.hidePrivileges, hideDetails);

        writeArraySafeName(writer, "children", category.children().begin(), category.children().end(), restriction);
    }

    OptionalRevertToNoneChanger<decltype(serializationSettings.displayDiscussionCategoryParentRecursionDepth)::value_type>
            recursionDepthChanger(serializationSettings.displayDiscussionCategoryParentRecursionDepth, 0);

    constexpr int maxDisplayDepth = 10;
    auto& depth = *serializationSettings.displayDiscussionCategoryParentRecursionDepth;

    static_assert((maxDisplayDepth * 2) < JsonWriter::MaxStateDepth,
                  "JsonWriter cannot hold a large enough state to allow recursing to the maxDisplayPath");

    if (( ! serializationSettings.hideDiscussionCategoryParent) && (depth < maxDisplayDepth))
    {
        if (DiscussionCategoryConstPtr parent = category.parent())
        {
            if (serializationSettings.onlySendCategoryParentId)
            {
                writer.newPropertyWithSafeName("parentId") << parent->id();
            }
            else
            {
                BoolTemporaryChanger _(serializationSettings.showDiscussionCategoryChildren, false);
                BoolTemporaryChanger __(serializationSettings.hidePrivileges, true);

                depth += 1;
                writer.newPropertyWithSafeName("parent");
                serialize(writer, *parent, restriction);
            }
        }
    }

    if ( ! serializationSettings.hidePrivileges)
    {
        writePrivileges(writer, category, DiscussionCategoryPrivilegesToSerialize,
                        DiscussionCategoryPrivilegeStrings, restriction);
    }

    writer
        << objEnd;
    return writer;
}
