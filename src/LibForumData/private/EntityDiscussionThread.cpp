#include "EntityDiscussionThread.h"
#include "EntityCollection.h"

#include "Configuration.h"
#include "ContextProviders.h"

#include <algorithm>

using namespace Forum;
using namespace Forum::Entities;
using namespace Forum::Authorization;

DiscussionThreadMessage::VoteScoreType DiscussionThread::voteScore() const
{
    if (messages_.size())
    {
        return (*messagesByCreated().begin())->voteScore();
    }
    return 0;
}

PrivilegeValueType DiscussionThread::getDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege privilege) const
{
    auto result = DiscussionThreadMessagePrivilegeStore::getDiscussionThreadMessagePrivilege(privilege);
    for (auto& tagWeak : tags_)
    {
        if (auto tagShared = tagWeak.lock())
        {
            result = minimumPrivilegeValue(result, tagShared->getDiscussionThreadMessagePrivilege(privilege));
        }
    }
    return result;
}

PrivilegeValueType DiscussionThread::getDiscussionThreadPrivilege(DiscussionThreadPrivilege privilege) const
{
    auto result = DiscussionThreadPrivilegeStore::getDiscussionThreadPrivilege(privilege);
    for (auto& tagWeak : tags_)
    {
        if (auto tagShared = tagWeak.lock())
        {
            result = minimumPrivilegeValue(result, tagShared->getDiscussionThreadPrivilege(privilege));
        }
    }
    return result;
}

PrivilegeDefaultDurationType DiscussionThread::getDiscussionThreadMessageDefaultPrivilegeDuration(DiscussionThreadMessageDefaultPrivilegeDuration privilege) const
{
    auto result = DiscussionThreadPrivilegeStore::getDiscussionThreadMessageDefaultPrivilegeDuration(privilege);
    if (result) return result;

    for (auto& tagWeak : tags_)
    {
        if (auto tagShared = tagWeak.lock())
        {
            result = maximumPrivilegeDefaultDuration(result, tagShared->getDiscussionThreadMessageDefaultPrivilegeDuration(privilege));
        }
    }
    return result;
}

void DiscussionThread::insertMessage(DiscussionThreadMessageRef message)
{
    DiscussionThreadMessageCollectionBase<OrderedIndexForId>::insertMessage(message);
    if (message)
    {
        latestMessageCreated_ = std::max(latestMessageCreated_, message->created());
    }
}

void DiscussionThread::modifyDiscussionThreadMessage(MessageIdIteratorType iterator,
                                                     std::function<void(DiscussionThreadMessage&)>&& modifyFunction)
{
    DiscussionThreadMessageCollectionBase<OrderedIndexForId>::modifyDiscussionThreadMessage(iterator, std::move(modifyFunction));
    refreshLatestMessageCreated();
}

DiscussionThreadMessageRef DiscussionThread::deleteDiscussionThreadMessage(MessageIdIteratorType iterator)
{
    auto result = DiscussionThreadMessageCollectionBase<OrderedIndexForId>::deleteDiscussionThreadMessage(iterator);
    refreshLatestMessageCreated();
    return result;
}

void DiscussionThread::refreshLatestMessageCreated()
{
    auto& index = messages_.template get<DiscussionThreadMessageCollectionByCreated>();
    auto it = index.rbegin();
    if (it == index.rend() || nullptr == *it)
    {
        latestMessageCreated_ = 0;
        return;
    }
    latestMessageCreated_ = (*it)->created();
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
