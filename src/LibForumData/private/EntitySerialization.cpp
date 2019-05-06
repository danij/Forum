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

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Authorization;
using namespace Json;

thread_local SerializationSettings Entities::serializationSettings = {};

JsonWriter& Json::operator<<(JsonWriter& writer, const EntitiesCount& value)
{
    writer.startObject();
    JSON_WRITE_FIRST_PROP(writer, "users", value.nrOfUsers);
    JSON_WRITE_PROP(writer, "discussionThreads", value.nrOfDiscussionThreads);
    JSON_WRITE_PROP(writer, "discussionMessages", value.nrOfDiscussionMessages);
    JSON_WRITE_PROP(writer, "discussionTags", value.nrOfDiscussionTags);
    JSON_WRITE_PROP(writer, "discussionCategories", value.nrOfDiscussionCategories);
    JSON_WRITE_PROP(writer, "visitors", value.nrOfVisitors);
    writer.endObject();
    return writer;
}

JsonWriter& Json::operator<<(JsonWriter& writer, const UuidString& id)
{
    char buffer[UuidString::StringRepresentationSizeCompact];

    id.toStringCompact(buffer);

    return writer.writeSafeString(buffer, std::size(buffer));
}

JsonWriter& Entities::serialize(JsonWriter& writer, const User& user, const SerializationRestriction& restriction)
{
    writer.startObject();
    JSON_WRITE_FIRST_PROP(writer, "id", user.id());
    JSON_WRITE_PROP(writer, "name", user.name());

    const auto sameUser = Context::getCurrentUserId() == user.id();

    if (sameUser || restriction.isAllowed(ForumWidePrivilege::GET_USER_INFO))
    {
        JSON_WRITE_PROP(writer, "info", user.info());
    }

    if (sameUser || restriction.isAllowed(ForumWidePrivilege::GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER))
    {
        JSON_WRITE_PROP(writer, "subscribedThreadCount", user.subscribedThreads().count());
    }

    if (sameUser || restriction.isAllowed(ForumWidePrivilege::GET_ATTACHMENTS_OF_USER))
    {
        JSON_WRITE_PROP(writer, "attachmentCount", user.attachments().count());
        JSON_WRITE_PROP(writer, "attachmentTotalSize", user.attachments().totalSize());
    }

    if (user.attachmentQuota() && (sameUser || restriction.isAllowed(ForumWidePrivilege::CHANGE_USER_ATTACHMENT_QUOTA)))
    {
        JSON_WRITE_PROP(writer, "attachmentQuota", *user.attachmentQuota());
    }

    if ((0 == user.lastSeen()) || user.showInOnlineUsers())
    {
        JSON_WRITE_PROP(writer, "lastSeen", user.lastSeen());
    }

    JSON_WRITE_PROP(writer, "title", user.title());
    JSON_WRITE_PROP(writer, "signature", user.signature());
    JSON_WRITE_PROP(writer, "hasLogo", user.hasLogo());
    JSON_WRITE_PROP(writer, "created", user.created());
    JSON_WRITE_PROP(writer, "threadCount", user.threads().count());
    JSON_WRITE_PROP(writer, "messageCount", user.threadMessages().byId().size());
    JSON_WRITE_PROP(writer, "receivedUpVotes", user.receivedUpVotes());
    JSON_WRITE_PROP(writer, "receivedDownVotes", user.receivedDownVotes());

    writer.endObject();
    return writer;
}

JsonWriter& writeVisitDetails(JsonWriter& writer, const VisitDetails& visitDetails)
{
    //does not currently start a new object
    char buffer[IpAddress::MaxIPv6CharacterCount + 1];
    writer.newPropertyRaw(JSON_RAW_PROP_COMMA("ip"));

    const auto addressLength = visitDetails.ip.toString(buffer, std::size(buffer));

    writer.writeSafeString(buffer, addressLength);
    return writer;
}

JsonWriter& Entities::serialize(JsonWriter& writer, const DiscussionThreadMessage& message,
                                const SerializationRestriction& restriction)
{
    const auto allowView = serializationSettings.allowDisplayDiscussionThreadMessage
            ? (*serializationSettings.allowDisplayDiscussionThreadMessage
                && restriction.checkMessageAllowViewApproval(message)
                && restriction.checkThreadAllowViewApproval(*message.parentThread()))
            : restriction.isAllowedToViewMessage(message);

    if ( ! allowView) return writer.null();

    const auto allowViewUser = serializationSettings.allowDisplayDiscussionThreadMessageUser
            ? *serializationSettings.allowDisplayDiscussionThreadMessageUser
            : restriction.isAllowed(message, DiscussionThreadMessagePrivilege::VIEW_CREATOR_USER);

    const auto allowViewVotes = serializationSettings.allowDisplayDiscussionThreadMessageVotes
            ? *serializationSettings.allowDisplayDiscussionThreadMessageVotes
            : restriction.isAllowed(message, DiscussionThreadMessagePrivilege::VIEW_VOTES);

    const auto allowViewIpAddress = serializationSettings.allowDisplayDiscussionThreadMessageIpAddress
            ? *serializationSettings.allowDisplayDiscussionThreadMessageIpAddress
            : restriction.isAllowed(message, DiscussionThreadMessagePrivilege::VIEW_IP_ADDRESS);

    const auto allowViewCommentCount = serializationSettings.allowDisplayDiscussionThreadMessageComments
            ? *serializationSettings.allowDisplayDiscussionThreadMessageComments
            : restriction.isAllowed(message, DiscussionThreadMessagePrivilege::GET_MESSAGE_COMMENTS);

    writer.startObject();
    JSON_WRITE_FIRST_PROP(writer, "id", message.id());
    JSON_WRITE_PROP(writer, "created", message.created());
    JSON_WRITE_PROP(writer, "approved", message.approved());

    if (allowViewCommentCount)
    {
        JSON_WRITE_PROP(writer, "commentsCount", message.comments().count());
        JSON_WRITE_PROP(writer, "solvedCommentsCount", message.solvedCommentsCount());
    }

    auto content = message.content();
    writer.newPropertyRaw(JSON_RAW_PROP_COMMA("content")).writeEscapedString(content.data(), content.size());

    if (allowViewUser && ( ! serializationSettings.hideDiscussionThreadMessageCreatedBy))
    {
        BoolTemporaryChanger _(serializationSettings.hidePrivileges, true);

        writer.newPropertyRaw(JSON_RAW_PROP_COMMA("createdBy"));
        serialize(writer, message.createdBy(), restriction);
    }
    if ( ! serializationSettings.hideDiscussionThreadMessageParentThread)
    {
        DiscussionThreadConstPtr parentThread = message.parentThread();
        assert(parentThread);

        BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadMessages, true);
        BoolTemporaryChanger __(serializationSettings.hidePrivileges, true);

        writer.newPropertyRaw(JSON_RAW_PROP_COMMA("parentThread"));
        serialize(writer, *parentThread, restriction);
    }

    if (message.lastUpdated())
    {
        writer.newPropertyRaw(JSON_RAW_PROP_COMMA("lastUpdated"));
        writer.startObject();

        UserConstPtr by = message.lastUpdatedBy();
        if (by && allowViewUser)
        {
            JSON_WRITE_FIRST_PROP(writer, "userId", by->id());
            JSON_WRITE_PROP(writer, "userName", by->name());
            JSON_WRITE_PROP(writer, "at", message.lastUpdated());
        }
        else
        {
            JSON_WRITE_FIRST_PROP(writer, "at", message.lastUpdated());            
        }
        JSON_WRITE_PROP(writer, "reason", message.lastUpdatedReason());
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

    const auto& upVotes = message.upVotes();
    const auto& downVotes = message.downVotes();

    if (allowViewVotes)
    {
        JSON_WRITE_PROP(writer, "nrOfUpVotes", upVotes.size());
        JSON_WRITE_PROP(writer, "nrOfDownVotes", downVotes.size());
    }
    auto voteStatus = 0;
    {
        auto currentUserTemp = const_cast<UserPtr>(serializationSettings.currentUser);
        if (currentUserTemp && (downVotes.find(currentUserTemp) != downVotes.end()))
        {
            voteStatus = -1;
        }
        else if (currentUserTemp && (upVotes.find(currentUserTemp) != upVotes.end()))
        {
            voteStatus = 1;
        }
    }
    JSON_WRITE_PROP(writer, "voteStatus", voteStatus);

    if (( ! message.attachments().empty()) && restriction.isAllowedToViewMessageAttachments(message))
    {
        writer.newPropertyRaw(JSON_RAW_PROP_COMMA("attachments"));
        writer.startArray();

        const auto allowViewAllAttachments = restriction.isAllowedToViewAnyAttachment();
        for (const auto attachmentPtr: message.attachments())
        {
            const Attachment& attachment = *attachmentPtr;

            if (allowViewAllAttachments || restriction.isAllowedToViewAttachment(attachment, message))
            {
                serialize(writer, attachment, restriction);
            }
        }

        writer.endArray();
    }

    if ( ! serializationSettings.hidePrivileges)
    {
        writePrivileges(writer, message, DiscussionThreadMessagePrivilegesToSerialize,
                        DiscussionThreadMessagePrivilegeStrings, restriction);
    }

    writer << objEnd;
    return writer;
}

JsonWriter& Entities::serialize(JsonWriter& writer, const PrivateMessage& message,
                                const SerializationRestriction& restriction)
{
    writer.startObject();
    JSON_WRITE_FIRST_PROP(writer, "id", message.id());
    JSON_WRITE_PROP(writer, "created", message.created());
    JSON_WRITE_PROP(writer, "content", message.content());

    if (serializationSettings.allowDisplayPrivateMessageIpAddress)
    {
        writeVisitDetails(writer, message.creationDetails());
    }

    if ( ! serializationSettings.hidePrivateMessageSource)
    {
        BoolTemporaryChanger _(serializationSettings.hidePrivileges, true);

        writer.newPropertyRaw(JSON_RAW_PROP_COMMA("source"));
        serialize(writer, message.source(), restriction);
    }

    if ( ! serializationSettings.hidePrivateMessageDestination)
    {
        BoolTemporaryChanger _(serializationSettings.hidePrivileges, true);

        writer.newPropertyRaw(JSON_RAW_PROP_COMMA("destination"));
        serialize(writer, message.destination(), restriction);
    }

    writer << objEnd;
    return writer;
}

static void writeLatestMessage(JsonWriter& writer, const DiscussionThreadMessage& latestMessage,
                               const SerializationRestriction& restriction)
{
    writer.newPropertyRaw(JSON_RAW_PROP_COMMA("latestMessage"));
    
    if ( ! restriction.isAllowedToViewMessage(latestMessage))
    {
        writer.null();
        return;
    }

    auto parentThread = latestMessage.parentThread();
    assert(parentThread);
    
    writer.startObject();
    JSON_WRITE_FIRST_PROP(writer, "id", latestMessage.id());
    JSON_WRITE_PROP(writer, "created", latestMessage.created());
    JSON_WRITE_PROP(writer, "approved", latestMessage.approved());
    JSON_WRITE_PROP(writer, "threadId", parentThread->id());
    JSON_WRITE_PROP(writer, "threadName", parentThread->name());

    auto content = latestMessage.content();
    writer.newPropertyRaw(JSON_RAW_PROP_COMMA("content")).writeEscapedString(content.data(), content.size());

    if (restriction.isAllowed(latestMessage, DiscussionThreadMessagePrivilege::VIEW_CREATOR_USER))
    {
        writer.newPropertyRaw(JSON_RAW_PROP_COMMA("createdBy"));
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
    writer.startObject();
    JSON_WRITE_FIRST_PROP(writer, "id", comment.id());
    JSON_WRITE_PROP(writer, "created", comment.created());
    JSON_WRITE_PROP(writer, "solved", comment.solved());

    auto content = comment.content();
    writer.newPropertyRaw(JSON_RAW_PROP_COMMA("content")).writeEscapedString(content.data(), content.size());

    writeVisitDetails(writer, comment.creationDetails());

    if ( ! serializationSettings.hideMessageCommentMessage)
    {
        writer.newPropertyRaw(JSON_RAW_PROP_COMMA("message"));
        serialize(writer, comment.parentMessage(), restriction);
    }
    if ( ! serializationSettings.hideMessageCommentUser)
    {
        BoolTemporaryChanger _(serializationSettings.hidePrivileges, true);

        writer.newPropertyRaw(JSON_RAW_PROP_COMMA("createdBy"));
        serialize(writer, comment.createdBy(), restriction);
    }

    writer << objEnd;
    return writer;
}

//Overload to reduce the number of calls to the authorization engine for checking rights to each message
template<typename Collection, size_t PropertyNameSize>
void writeDiscussionThreadMessages(const Collection& collection, const int_fast32_t pageNumber, const int_fast32_t pageSize,
                                   const bool ascending, const char(&propertyName)[PropertyNameSize], JsonWriter& writer,
                                   const SerializationRestriction& restriction)
{
    auto totalCount = static_cast<int_fast32_t>(collection.size());

    writer.newPropertyWithSafeName("totalCount") << totalCount;
    JSON_WRITE_PROP(writer, "pageSize", pageSize);
    JSON_WRITE_PROP(writer, "page", pageNumber);

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
    if ( ! restriction.isAllowedToViewThread(thread)) return writer.null();

    writer.startObject();
    JSON_WRITE_FIRST_PROP(writer, "id", thread.id());
    JSON_WRITE_PROP(writer, "name", thread.name());
    JSON_WRITE_PROP(writer, "created", thread.created());
    JSON_WRITE_PROP(writer, "approved", thread.approved());
    JSON_WRITE_PROP(writer, "latestVisibleChangeAt", thread.latestVisibleChange());
    JSON_WRITE_PROP(writer, "pinned", (thread.pinDisplayOrder() > 0));
    JSON_WRITE_PROP(writer, "pinDisplayOrder", thread.pinDisplayOrder());
    JSON_WRITE_PROP(writer, "subscribedUsersCount", thread.subscribedUsersCount());

    auto currentUser = serializationSettings.currentUser;

    if (currentUser)
    {
        writer.newPropertyRaw(JSON_RAW_PROP_COMMA("subscribedToThread")) <<
            (thread.subscribedUsers().find(const_cast<UserPtr>(currentUser)) != thread.subscribedUsers().end());
        
        const auto latestVisitedPage = currentUser->latestPageVisited(thread.id());
        if (latestVisitedPage > 0)
        {
            JSON_WRITE_PROP(writer, "latestVisitedPage", latestVisitedPage);
        }
    }

    if ( ! serializationSettings.hideDiscussionThreadCreatedBy)
    {
        BoolTemporaryChanger _(serializationSettings.hidePrivileges, true);

        writer.newPropertyRaw(JSON_RAW_PROP_COMMA("createdBy"));
        serialize(writer, thread.createdBy(), restriction);
    }

    const auto& messagesIndex = thread.messages().byCreated();
    const auto messageCount = messagesIndex.size();

    JSON_WRITE_PROP(writer, "messageCount", messageCount);

    if (messageCount && ! serializationSettings.hideLatestMessage)
    {
        BoolTemporaryChanger _(serializationSettings.hidePrivileges, true);

        writeLatestMessage(writer, **messagesIndex.rbegin(), restriction);
    }
    if ( ! serializationSettings.hideDiscussionThreadMessages)
    {
        const auto pageSize = getGlobalConfig()->discussionThreadMessage.maxMessagesPerPage;
        auto& displayContext = Context::getDisplayContext();

        writeDiscussionThreadMessages(messagesIndex, displayContext.pageNumber, pageSize, true, "messages", writer,
                                      restriction);
    }
    if ( ! serializationSettings.hideVisitedThreadSinceLastChange)
    {
        JSON_WRITE_PROP(writer, "visitedSinceLastChange", serializationSettings.visitedThreadSinceLastChange);
    }

    {
        BoolTemporaryChanger _(serializationSettings.hideDiscussionCategoriesOfTags, true);
        BoolTemporaryChanger __(serializationSettings.hideLatestMessage, true);
        BoolTemporaryChanger ___(serializationSettings.hidePrivileges, true);

        writer.newPropertyRaw(JSON_RAW_PROP_COMMA("tags"));
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

        writer.newPropertyRaw(JSON_RAW_PROP_COMMA("categories"));
        writer << arrayStart;
        for (auto category : thread.categories())
        {
            serialize(writer, *category, restriction);
        }
        writer << arrayEnd;
    }
    JSON_WRITE_PROP(writer, "lastUpdated", thread.lastUpdated());
    JSON_WRITE_PROP(writer, "visited", thread.visited().load());
    JSON_WRITE_PROP(writer, "voteScore", thread.voteScore());

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

    writer.startObject();
    JSON_WRITE_FIRST_PROP(writer, "id", tag.id());
    JSON_WRITE_PROP(writer, "name", tag.name());
    JSON_WRITE_PROP(writer, "created", tag.created());
    JSON_WRITE_PROP(writer, "threadCount", tag.threads().count());
    JSON_WRITE_PROP(writer, "messageCount", tag.messageCount());

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

        writer.newPropertyRaw(JSON_RAW_PROP_COMMA("categories"));
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

    writer.startObject();
    JSON_WRITE_FIRST_PROP(writer, "id", category.id());
    JSON_WRITE_PROP(writer, "name", category.name());
    JSON_WRITE_PROP(writer, "description", category.description());
    JSON_WRITE_PROP(writer, "displayOrder", category.displayOrder());
    JSON_WRITE_PROP(writer, "created", category.created());
    JSON_WRITE_PROP(writer, "threadCount", category.threads().count());
    JSON_WRITE_PROP(writer, "messageCount", category.messageCount());
    JSON_WRITE_PROP(writer, "threadTotalCount", category.threadTotalCount());
    JSON_WRITE_PROP(writer, "messageTotalCount", category.messageTotalCount());

    if ( ! serializationSettings.hideLatestMessage)
    {
        if (const auto latestMessage = category.latestMessage())
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
    if (serializationSettings.showDiscussionCategoryChildrenRecursionLeft > 0)
    {
        const auto hideDetails = serializationSettings.showDiscussionCategoryChildrenRecursionLeft <= 1;

        IntTemporaryChanger _(serializationSettings.showDiscussionCategoryChildrenRecursionLeft, 
                              serializationSettings.showDiscussionCategoryChildrenRecursionLeft - 1);
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
        if (const DiscussionCategoryConstPtr parent = category.parent())
        {
            if (serializationSettings.onlySendCategoryParentId)
            {
                JSON_WRITE_PROP(writer, "parentId", parent->id());
            }
            else
            {
                IntTemporaryChanger _(serializationSettings.showDiscussionCategoryChildrenRecursionLeft, 0);
                BoolTemporaryChanger __(serializationSettings.hidePrivileges, true);

                depth += 1;
                writer.newPropertyRaw(JSON_RAW_PROP_COMMA("parent"));
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

JsonWriter& Entities::serialize(JsonWriter& writer, const Attachment& attachment,
                                const SerializationRestriction& restriction)
{
    writer.startObject();
    JSON_WRITE_FIRST_PROP(writer, "id", attachment.id());
    JSON_WRITE_PROP(writer, "created", attachment.created());
    JSON_WRITE_PROP(writer, "name", attachment.name());
    JSON_WRITE_PROP(writer, "size", attachment.size());
    JSON_WRITE_PROP(writer, "approved", attachment.approved());
    JSON_WRITE_PROP(writer, "nrOfMessagesAttached", attachment.messages().size());
    JSON_WRITE_PROP(writer, "nrOfGetRequests", attachment.nrOfGetRequests());

    if (serializationSettings.allowDisplayAttachmentIpAddress)
    {
        writeVisitDetails(writer, attachment.creationDetails());        
    }

    if ( ! serializationSettings.hideAttachmentCreatedBy)
    {
        BoolTemporaryChanger _(serializationSettings.hidePrivileges, true);

        writer.newPropertyRaw(JSON_RAW_PROP_COMMA("createdBy"));
        serialize(writer, attachment.createdBy(), restriction);
    }

    writer.endObject();
    return writer;
}
