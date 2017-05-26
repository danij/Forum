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
            << propertySafeName("created", user.created())
            << propertySafeName("lastSeen", user.lastSeen())
            << propertySafeName("threadCount", user.threadsById().size())
            << propertySafeName("messageCount", user.messagesById().size())
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

JsonWriter& writeVisitDetails(JsonWriter& writer, const VisitDetails& visitDetails)
{
    //does not currently start a new object
    char buffer[IpAddress::MaxIPv6CharacterCount + 1];
    writer.newPropertyWithSafeName("ip");

    auto addressLength = visitDetails.ip.toString(buffer, std::extent<decltype(buffer)>::value);

    writer.writeSafeString(buffer, addressLength);
    return writer;
}

template<typename Entity, typename PrivilegeArray>
static void writePermissions(JsonWriter& writer, const Entity& entity, const PrivilegeArray& privilegeArray, 
                             const SerializationRestriction& restriction)
{
    writer.newPropertyWithSafeName("permissions");
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

JsonWriter& Entities::serialize(JsonWriter& writer, const DiscussionThreadMessage& message, 
                                const SerializationRestriction& restriction)
{
    auto allowViewOverride = serializationSettings.allowDisplayDiscussionThreadMessage == true;
    auto allowViewUserOverride = serializationSettings.allowDisplayDiscussionThreadMessageUser == true;
    auto allowViewVotesOverride = serializationSettings.allowDisplayDiscussionThreadMessageVotes == true;
    auto allowViewIpAddressOverride = serializationSettings.allowDisplayDiscussionThreadMessageIpAddress == true;

    if ( ! allowViewOverride)
    {
        if ( ! restriction.isAllowed(message)) return writer.null();
    }

    writer
        << objStart
            << propertySafeName("id", message.id())
            << propertySafeName("created", message.created())
            << propertySafeName("commentsCount", message.messageCommentCount())
            << propertySafeName("solvedCommentsCount", message.solvedCommentsCount());

    auto content = message.content();
    writer.newPropertyWithSafeName("content").writeEscapedString(content.data(), content.size());

    if (allowViewUserOverride && ( ! serializationSettings.hideDiscussionThreadCreatedBy))
    {
        writer.newPropertyWithSafeName("createdBy");
        serialize(writer, message.createdBy(), restriction);
    }
    if ( ! serializationSettings.hideDiscussionThreadMessageParentThread)
    {
        message.executeActionWithParentThreadIfAvailable([&writer, &restriction](auto& parentThread)
        {
            writer.newPropertyWithSafeName("parentThread");
            BoolTemporaryChanger _(serializationSettings.hideDiscussionThreadMessages, true);
            serialize(writer, parentThread, restriction);
        });
    }

    if (message.lastUpdated())
    {
        writer.newPropertyWithSafeName("lastUpdated");
        writer.startObject();
        message.executeActionWithLastUpdatedByIfAvailable([&](auto& by)
        {
            if (allowViewUserOverride)
            {
                writer << propertySafeName("userId", by.id())
                       << propertySafeName("userName", by.name());
            }
        });
        writer << propertySafeName("at", message.lastUpdated())
               << propertySafeName("reason", message.lastUpdatedReason());
        if (allowViewIpAddressOverride)
        {
            writeVisitDetails(writer, message.lastUpdatedDetails());
        }

        writer.endObject();
    }

    if (allowViewIpAddressOverride)
    {
        writeVisitDetails(writer, message.creationDetails());
    }

    if (allowViewVotesOverride)
    {
        writeVotes(writer, "upVotes", message.upVotes());
        writeVotes(writer, "downVotes", message.downVotes());
    }

    writePermissions(writer, message, DiscussionThreadMessagePrivilegesToSerialize, restriction);

    writer << objEnd;
    return writer;
}

static void writeLatestMessage(JsonWriter& writer, const DiscussionThreadMessage& latestMessage,
                               const SerializationRestriction& restriction)
{
    writer.newPropertyWithSafeName("latestMessage");
    writer << objStart
        << propertySafeName("id", latestMessage.id())
        << propertySafeName("created", latestMessage.created());
    writer.newPropertyWithSafeName("createdBy");
    serialize(writer, latestMessage.createdBy(), restriction);
    writer << objEnd;
}

template<typename IndexIdType>
static void writeLatestMessage(JsonWriter& writer, const DiscussionThreadCollectionBase<IndexIdType>& threads,
                               const SerializationRestriction& restriction)
{
    auto index = threads.threadsByLatestMessageCreated();
    if ( ! index.size())
    {
        return;
    }
    auto thread = *(index.rbegin());
    auto messageIndex = thread->messagesByCreated();
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
        comment.executeActionWithParentMessageIfAvailable([&](const DiscussionThreadMessage& message)
        {
            writer.newPropertyWithSafeName("message");
            serialize(writer, message, restriction);
        });
    }
    if ( ! serializationSettings.hideMessageCommentUser)
    {
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
                privilegeChecks.emplace_back(DiscussionThreadMessagePrivilegeCheck(restriction.user(), **it));
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
                    privilegeChecks.emplace_back(DiscussionThreadMessagePrivilegeCheck(restriction.user(), **it));
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
            << propertySafeName("visitorsSinceLastChange", thread.nrOfVisitorsSinceLastEdit())
            << propertySafeName("subscribedUsersCount", thread.subscribedUsersCount());
    if ( ! serializationSettings.hideDiscussionThreadCreatedBy)
    {
        writer.newPropertyWithSafeName("createdBy");
        serialize(writer, thread.createdBy(), restriction);
    }

    const auto& messagesIndex = thread.messagesByCreated();
    auto messageCount = messagesIndex.size();

    writer << propertySafeName("messageCount", messageCount);

    if (messageCount)
    {
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
        //need to iterate manually over tags as they are weak pointers and may be empty
        writer.newPropertyWithSafeName("tags");
        writer << arrayStart;
        for (auto tag : thread.tags())
        {
            if (tag)
            {
                serialize(writer, *tag, restriction);
            }
        }
        writer << arrayEnd;
    }
    {
        BoolTemporaryChanger _(serializationSettings.hideDiscussionCategoryParent, true);
        BoolTemporaryChanger __(serializationSettings.hideDiscussionCategoryTags, true);
        BoolTemporaryChanger ___(serializationSettings.hideLatestMessage, true);
        //need to iterate manually over categories as they are weak pointers and may be empty
        writer.newPropertyWithSafeName("categories");
        writer << arrayStart;
        for (auto category : thread.categories())
        {
            if (category)
            {
                serialize(writer, *category, restriction);
            }
        }
        writer << arrayEnd;
    }
    writer << propertySafeName("lastUpdated", thread.lastUpdated())
           << propertySafeName("visited", thread.visited().load())
           << propertySafeName("voteScore", thread.voteScore());

    writePermissions(writer, thread, DiscussionThreadPrivilegesToSerialize, restriction);

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
        << propertySafeName("threadCount", tag.threadsById().size())
        << propertySafeName("messageCount", tag.messageCount());

    if ( ! serializationSettings.hideLatestMessage)
    {
        writeLatestMessage(writer, tag, restriction);
    }
    if ( ! serializationSettings.hideDiscussionCategoriesOfTags)
    {
        BoolTemporaryChanger _(serializationSettings.hideDiscussionCategoryTags, true);
        BoolTemporaryChanger __(serializationSettings.hideDiscussionCategoryParent, true);
        //need to iterate manually over categories as they are weak pointers and may be empty
        writer.newPropertyWithSafeName("categories");
        writer << arrayStart;
        for (auto category : tag.categories())
        {
            if (category)
            {
                serialize(writer, *category, restriction);
            }
        }
        writer << arrayEnd;
    }
    
    writePermissions(writer, tag, DiscussionTagPrivilegesToSerialize, restriction);
    
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
        << propertySafeName("threadCount", category.threadsById().size())
        << propertySafeName("messageCount", category.messageCount())
        << propertySafeName("threadTotalCount", category.threadTotalCount())
        << propertySafeName("messageTotalCount", category.messageTotalCount());

    if ( ! serializationSettings.hideLatestMessage)
    {
        auto latestMessage = category.latestMessage();
        if (latestMessage)
        {
            writeLatestMessage(writer, *latestMessage, restriction);
        }
    }
    if ( ! serializationSettings.hideDiscussionCategoryTags)
    {
        BoolTemporaryChanger _(serializationSettings.hideDiscussionCategoriesOfTags, true);

        writeArraySafeName(writer, "tags", category.tags().begin(), category.tags().end(), restriction);
    }
    if (serializationSettings.showDiscussionCategoryChildren)
    {
        //only show 1 level of category children
        BoolTemporaryChanger _(serializationSettings.showDiscussionCategoryChildren, false);
        BoolTemporaryChanger __(serializationSettings.hideDiscussionCategoryParent, true);

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
        category.executeActionWithParentCategoryIfAvailable([&](auto& parent)
        {
            BoolTemporaryChanger _(serializationSettings.showDiscussionCategoryChildren, false);
            depth += 1;
            writer.newPropertyWithSafeName("parent");
            serialize(writer, parent, restriction);
        });
    }

    writePermissions(writer, category, DiscussionCategoryPrivilegesToSerialize, restriction);

    writer
        << objEnd;
    return writer;
}
