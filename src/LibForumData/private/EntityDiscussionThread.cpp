#include "EntityDiscussionThread.h"
#include "EntityCollection.h"

#include "Configuration.h"
#include "ContextProviders.h"

using namespace Forum;
using namespace Forum::Entities;

DiscussionThreadMessage::VoteScoreType DiscussionThread::voteScore() const
{
    if (messages_.size())
    {
        return (*messagesById().begin())->voteScore();
    }
    return 0;
}

void DiscussionThread::addVisitorSinceLastEdit(const IdType& userId)
{
    if (static_cast<int_fast32_t>(visitorsSinceLastEdit_.size()) >=
        Configuration::getGlobalConfig()->discussionThread.maxUsersInVisitedSinceLastChange)
    {
        visitorsSinceLastEdit_.clear();
    }
    visitorsSinceLastEdit_.insert(userId.value());
}

bool DiscussionThread::hasVisitedSinceLastEdit(const IdType& userId) const
{
    return visitorsSinceLastEdit_.find(userId.value()) != visitorsSinceLastEdit_.end();
}

void DiscussionThread::resetVisitorsSinceLastEdit()
{
    visitorsSinceLastEdit_.clear();
}

bool DiscussionThread::addTag(std::weak_ptr<DiscussionTag> tag)
{
    latestVisibleChange() = Context::getCurrentTime();
    return std::get<1>(tags_.insert(std::move(tag)));
}

bool DiscussionThread::removeTag(const std::weak_ptr<DiscussionTag>& tag)
{
    latestVisibleChange() = Context::getCurrentTime();
    return tags_.erase(tag) > 0;
}

bool DiscussionThread::addCategory(std::weak_ptr<DiscussionCategory> category)
{
    latestVisibleChange() = Context::getCurrentTime();
    return std::get<1>(categories_.insert(std::move(category)));
}

bool DiscussionThread::removeCategory(const std::weak_ptr<DiscussionCategory>& category)
{
    latestVisibleChange() = Context::getCurrentTime();
    return categories_.erase(category) > 0;
}

Timestamp DiscussionThread::latestMessageCreated() const
{
    auto index = messagesByCreated();
    if ( ! index.size())
    {
        return 0;
    }
    return (*index.rbegin())->created();
}
