#include "EntityDiscussionThreadCollection.h"

using namespace Forum::Entities;

bool DiscussionThreadCollectionBase::add(DiscussionThreadPtr thread)
{
    byName_.insert(thread);
    byCreated_.insert(thread);
    byLastUpdated_.insert(thread);
    byLatestMessageCreated_.insert(thread);
    byMessageCount_.insert(thread);
    byPinDisplayOrder_.insert(thread);

    if (onCountChange_) onCountChange_();
    return true;
}

bool DiscussionThreadCollectionBase::remove(DiscussionThreadPtr thread)
{
    {
        auto itByName = byName_.find(thread);
        if (itByName != byName_.end()) byName_.erase(itByName);
    }
    byCreated_.erase(thread);
    byLastUpdated_.erase(thread);
    byLatestMessageCreated_.erase(thread);
    byMessageCount_.erase(thread);
    byPinDisplayOrder_.erase(thread);

    if (onCountChange_) onCountChange_();
    return true;
}

void DiscussionThreadCollectionBase::updateName(DiscussionThreadPtr thread)
{
    auto it = byName_.find(thread);
    if (it != byName_.end())
    {
        byName_.replace(it, thread);
    }
}

void DiscussionThreadCollectionBase::updateLastUpdated(DiscussionThreadPtr thread)
{
    auto it = byLastUpdated_.find(thread);
    if (it != byLastUpdated_.end())
    {
        byLastUpdated_.replace(it, thread);
    }
}

void DiscussionThreadCollectionBase::updateLatestMessageCreated(DiscussionThreadPtr thread)
{
    auto it = byLatestMessageCreated_.find(thread);
    if (it != byLatestMessageCreated_.end())
    {
        byLatestMessageCreated_.replace(it, thread);
    }
}

void DiscussionThreadCollectionBase::updateMessageCount(DiscussionThreadPtr thread)
{
    auto it = byMessageCount_.find(thread);
    if (it != byMessageCount_.end())
    {
        byMessageCount_.replace(it, thread);
    }
}

void DiscussionThreadCollectionBase::updatePinDisplayOrder(DiscussionThreadPtr thread)
{
    auto it = byPinDisplayOrder_.find(thread);
    if (it != byPinDisplayOrder_.end())
    {
        byPinDisplayOrder_.replace(it, thread);
    }
}

bool DiscussionThreadCollectionWithHashedId::add(DiscussionThreadPtr thread)
{
    if ( ! std::get<1>(byId_.insert(thread))) return;
    
    return DiscussionThreadCollectionBase::add(thread);
}

bool DiscussionThreadCollectionWithHashedId::remove(DiscussionThreadPtr thread)
{
    {
        auto itById = byId_.find(thread);
        if (itById == byId_.end()) return false;
        
        byId_.erase(itById);
    }
    return DiscussionThreadCollectionBase::remove(thread);
}

bool DiscussionThreadCollectionWithOrderedId::add(DiscussionThreadPtr thread)
{
    if ( ! std::get<1>(byId_.insert(thread))) return false;
    
    return DiscussionThreadCollectionBase::add(thread);
}

bool DiscussionThreadCollectionWithOrderedId::remove(DiscussionThreadPtr thread)
{
    {
        auto itById = byId_.find(thread);
        if (itById == byId_.end()) return false;
        
        byId_.erase(itById);
    }
    return DiscussionThreadCollectionBase::remove(thread);
}

bool DiscussionThreadCollectionWithReferenceCountAndMessageCount::add(DiscussionThreadPtr thread)
{
    auto it = referenceCount_.find(thread);

    if (it == referenceCount_.end() && std::get<1>(byId_.insert(thread)))
    {
        byLatestMessageCreated_.insert(thread);

        referenceCount_.insert(std::make_pair(thread, 1));
        messageCount_ += thread->messageCount();
        return true;
    }
    it->second += 1;
    return false;
}

void DiscussionThreadCollectionWithReferenceCountAndMessageCount::decreaseReferenceCount(DiscussionThreadPtr thread)
{
    assert(thread);

    auto it = referenceCount_.find(thread);
    if (it == referenceCount_.end())
    {
        return;
    }
    if (it->second < 2)
    {
        referenceCount_.erase(it);
        remove(thread);
    }
}

bool DiscussionThreadCollectionWithReferenceCountAndMessageCount::remove(DiscussionThreadPtr thread)
{
    assert(thread);
    {
        auto itById = byId_.find(thread);
        if (itById == byId_.end()) return false;

        byId_.erase(itById);
    }
    byLatestMessageCreated_.erase(thread);
    referenceCount_.erase(thread);
    messageCount_ -= thread->messageCount();

    return true;
}

void DiscussionThreadCollectionWithReferenceCountAndMessageCount::updateLatestMessageCreated(DiscussionThreadPtr thread)
{
    auto it = byLatestMessageCreated_.find(thread);
    if (it != byLatestMessageCreated_.end())
    {
        byLatestMessageCreated_.replace(it, thread);
    }
}

DiscussionThreadMessagePtr DiscussionThreadCollectionWithReferenceCountAndMessageCount::latestMessage() const
{
    auto& index = byLatestMessageCreated_;
    if ( ! index.size())
    {
        return nullptr;
    }
    auto thread = *(index.rbegin());

    auto messageIndex = thread->messages().byCreated();
    if (messageIndex.size())
    {
        return *(messageIndex.rbegin());
    }
    return nullptr;
}
