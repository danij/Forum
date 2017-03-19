#include "Configuration.h"
#include "EntitySerialization.h"
#include "StateHelpers.h"
#include "OutputHelpers.h"

#include <type_traits>

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Json;

thread_local SerializationSettings Forum::Entities::serializationSettings = {};

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

static const char* hexChars = "0123456789abcdef";

JsonWriter& Json::operator<<(JsonWriter& writer, const UuidString& id)
{
    char buffer[boost::uuids::uuid::static_size() * 2 + 4];

    auto data = id.value().data;

    for (size_t source = 0, destination = 0; source < boost::uuids::uuid::static_size(); ++source)
    {
        auto value = data[source];

        buffer[destination++] = hexChars[(value / 16) & 0xF];
        buffer[destination++] = hexChars[(value % 16) & 0xF];

        if (8 == destination || 13 == destination || 18 == destination || 23 == destination)
        {
            buffer[destination++] = '-';
        }
    }

    return writer.writeSafeString(buffer, std::extent<decltype(buffer)>::value);
}

JsonWriter& Json::operator<<(JsonWriter& writer, const User& user)
{
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

template<typename Collection, std::size_t NameSize>
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

JsonWriter& Json::operator<<(JsonWriter& writer, const DiscussionThreadMessage& message)
{
    writer
        << objStart
            << propertySafeName("id", message.id())
            << propertySafeName("created", message.created())
            << propertySafeName("commentsCount", message.messageCommentCount())
            << propertySafeName("solvedCommentsCount", message.solvedCommentsCount());

    auto content = message.content();
    writer.newPropertyWithSafeName("content").writeEscapedString(content.data(), content.size());

    if ( ! serializationSettings.hideDiscussionThreadCreatedBy)
    {
        writer << propertySafeName("createdBy", message.createdBy());
    }
    if ( ! serializationSettings.hideDiscussionThreadMessageParentThread)
    {
        message.executeActionWithParentThreadIfAvailable([&writer](auto& parentThread)
        {
            writer << propertySafeName("parentThread", parentThread);
        });
    }

    if (message.lastUpdated())
    {
        writer.newPropertyWithSafeName("lastUpdated");
        writer.startObject();
        message.executeActionWithLastUpdatedByIfAvailable([&](auto& by)
        {
            writer << propertySafeName("userId", by.id())
                   << propertySafeName("userName", by.name());
        });
        writer << propertySafeName("at", message.lastUpdated())
               << propertySafeName("reason", message.lastUpdatedReason());
        writeVisitDetails(writer, message.lastUpdatedDetails());

        writer.endObject();
    }

    writeVisitDetails(writer, message.creationDetails());
    
    writeVotes(writer, "upVotes", message.upVotes());
    writeVotes(writer, "downVotes", message.downVotes());

    writer << objEnd;
    return writer;
}

static void writeLatestMessage(JsonWriter& writer, const DiscussionThreadMessage& latestMessage)
{
    writer.newPropertyWithSafeName("latestMessage");
    writer << objStart
        << propertySafeName("created", latestMessage.created())
        << propertySafeName("createdBy", latestMessage.createdBy())
        << objEnd;
}

template<typename IndexIdType>
static void writeLatestMessage(JsonWriter& writer, const DiscussionThreadCollectionBase<IndexIdType>& threads)
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
        writeLatestMessage(writer, **messageIndex.rbegin());
    }
}

JsonWriter& Json::operator<<(JsonWriter& writer, const MessageComment& comment)
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
            writer << propertySafeName("message", message);
        });
    }
    if ( ! serializationSettings.hideMessageCommentUser)
    {
        writer << propertySafeName("createdBy", comment.createdBy());
    }

    writer << objEnd;
    return writer;
}

JsonWriter& Json::operator<<(JsonWriter& writer, const DiscussionThread& thread)
{
    writer
        << objStart
            << propertySafeName("id", thread.id())
            << propertySafeName("name", thread.name())
            << propertySafeName("created", thread.created())
            << propertySafeName("latestVisibleChangeAt", thread.latestVisibleChange())
            << propertySafeName("visitorsSinceLastChange", thread.nrOfVisitorsSinceLastEdit());
    if ( ! serializationSettings.hideDiscussionThreadCreatedBy)
    {
        writer << propertySafeName("createdBy", thread.createdBy());
    }

    const auto& messagesIndex = thread.messagesByCreated();
    auto messageCount = messagesIndex.size();

    writer << propertySafeName("messageCount", messageCount);

    if (messageCount)
    {
        writeLatestMessage(writer, **messagesIndex.rbegin());
    }
    if ( ! serializationSettings.hideDiscussionThreadMessages)
    {
        auto pageSize = getGlobalConfig()->discussionThreadMessage.maxMessagesPerPage;
        auto& displayContext = Context::getDisplayContext();

        writeEntitiesWithPagination(messagesIndex, "messages", writer, displayContext.pageNumber, pageSize, true, 
                                    [](const auto& m) { return m; });
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
                writer << *tag;
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
                writer << *category;
            }
        }
        writer << arrayEnd;
    }
    writer  << propertySafeName("lastUpdated", thread.lastUpdated())
            << propertySafeName("visited", thread.visited().load())
            << propertySafeName("voteScore", thread.voteScore())

        << objEnd;
    return writer;
}

JsonWriter& Json::operator<<(JsonWriter& writer, const DiscussionTag& tag)
{
    writer << objStart
        << propertySafeName("id", tag.id())
        << propertySafeName("name", tag.name())
        << propertySafeName("created", tag.created())
        << propertySafeName("threadCount", tag.threadsById().size())
        << propertySafeName("messageCount", tag.messageCount());

    if ( ! serializationSettings.hideLatestMessage)
    {
        writeLatestMessage(writer, tag);
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
                writer << *category;
            }
        }
        writer << arrayEnd;
    }

    writer << objEnd;
    return writer;
}

JsonWriter& Json::operator<<(JsonWriter& writer, const DiscussionCategory& category)
{
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
            writeLatestMessage(writer, *latestMessage);
        }
    }
    if ( ! serializationSettings.hideDiscussionCategoryTags)
    {
        BoolTemporaryChanger _(serializationSettings.hideDiscussionCategoriesOfTags, true);
        writer << propertySafeName("tags", enumerate(category.tags().begin(), category.tags().end()));
    }
    if (serializationSettings.showDiscussionCategoryChildren)
    {
        //only show 1 level of category children
        BoolTemporaryChanger _(serializationSettings.showDiscussionCategoryChildren, false);
        BoolTemporaryChanger __(serializationSettings.hideDiscussionCategoryParent, true);
        writer << propertySafeName("children", enumerate(category.children().begin(), category.children().end()));
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
            writer << propertySafeName("parent", parent);
        });
    }

    writer
        << objEnd;
    return writer;
}
