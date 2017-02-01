#include "EntitySerialization.h"
#include "StateHelpers.h"
#include "OutputHelpers.h"

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
        writer << propertySafeName("reason", details.reason);
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

static void writeLatestMessage(JsonWriter& writer, const DiscussionThreadMessage& latestMessage)
{
    writer.newPropertyWithSafeName("latestMessage");
    writer << objStart
        << propertySafeName("created", latestMessage.created())
        << propertySafeName("createdBy", latestMessage.createdBy())
        << objEnd;
}

static void writeLatestMessage(JsonWriter& writer, const DiscussionThreadCollectionBase& threads)
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

JsonWriter& Json::operator<<(JsonWriter& writer, const DiscussionThread& thread)
{
    writer
        << objStart
            << propertySafeName("id", thread.id())
            << propertySafeName("name", thread.name())
            << propertySafeName("created", thread.created())
            << propertySafeName("visitorsSinceLastChange", thread.nrOfVisitorsSinceLastEdit());
    if ( ! serializationSettings.hideDiscussionThreadCreatedBy)
    {
        writer << propertySafeName("createdBy", thread.createdBy());
    }

    const auto& messagesIndex = thread.messagesByCreated();
    auto messageCount = messagesIndex.size();

    writer  << propertySafeName("messageCount", messageCount);

    if (messageCount)
    {
        writeLatestMessage(writer, **messagesIndex.rbegin());
    }
    if ( ! serializationSettings.hideDiscussionThreadMessages)
    {
        auto pageSize = getGlobalConfig()->discussionThreadMessage.maxMessagesPerPage;
        auto& displayContext = Context::getDisplayContext();

        writeEntitiesWithPagination(messagesIndex, "messages", writer, displayContext.pageNumber, pageSize, true, 
                                    [](auto m) { return m; });
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
    if ( ! serializationSettings.hideDiscussionCategoryParent)
    {
        if (auto parentRef = category.parentWeak().lock())
        {
            BoolTemporaryChanger _(serializationSettings.showDiscussionCategoryChildren, false);
            writer << propertySafeName("parent", *parentRef);
        }
    }

    writer
        << objEnd;
    return writer;
}
