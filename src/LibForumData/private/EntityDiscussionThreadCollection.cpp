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
        auto itByName = byName_.find(thread->name());
        if (itByName != byName_.end()) byName_.erase(itByName);
    }
    eraseFromNonUniqueCollection(byCreated_, thread, thread->created());
    eraseFromNonUniqueCollection(byLastUpdated_, thread, thread->lastUpdated());
    eraseFromNonUniqueCollection(byLatestMessageCreated_, thread, thread->latestMessageCreated());
    eraseFromNonUniqueCollection(byMessageCount_, thread, thread->messageCount());
    eraseFromNonUniqueCollection(byPinDisplayOrder_, thread, thread->pinDisplayOrder());

    if (onCountChange_) onCountChange_();
    return true;
}

void DiscussionThreadCollectionBase::updateName(DiscussionThreadPtr thread)
{
    auto it = byName_.find(thread->name());
    if (it != byName_.end())
    {
        byName_.replace(it, thread);
    }
}

void DiscussionThreadCollectionBase::updateLastUpdated(DiscussionThreadPtr thread)
{
    replaceInNonUniqueCollection(byLastUpdated_, thread, thread->lastUpdated());
}

void DiscussionThreadCollectionBase::updateLatestMessageCreated(DiscussionThreadPtr thread)
{
    replaceInNonUniqueCollection(byLatestMessageCreated_, thread, thread->latestMessageCreated());
}

void DiscussionThreadCollectionBase::updateMessageCount(DiscussionThreadPtr thread)
{
    replaceInNonUniqueCollection(byMessageCount_, thread, thread->messageCount());
}

void DiscussionThreadCollectionBase::updatePinDisplayOrder(DiscussionThreadPtr thread)
{
    replaceInNonUniqueCollection(byPinDisplayOrder_, thread, thread->pinDisplayOrder());
}

bool DiscussionThreadCollectionWithHashedId::add(DiscussionThreadPtr thread)
{
    if ( ! std::get<1>(byId_.insert(thread))) return false;
    
    return DiscussionThreadCollectionBase::add(thread);
}

bool DiscussionThreadCollectionWithHashedId::remove(DiscussionThreadPtr thread)
{
    {
        auto itById = byId_.find(thread->id());
        if (itById == byId_.end()) return false;
        
        byId_.erase(itById);
    }
    return DiscussionThreadCollectionBase::remove(thread);
}

bool DiscussionThreadCollectionWithHashedId::contains(DiscussionThreadPtr thread) const
{
    return byId_.find(thread->id()) != byId_.end();
}

bool DiscussionThreadCollectionWithOrderedId::add(DiscussionThreadPtr thread)
{
    if ( ! std::get<1>(byId_.insert(thread))) return false;
    
    return DiscussionThreadCollectionBase::add(thread);
}

bool DiscussionThreadCollectionWithOrderedId::remove(DiscussionThreadPtr thread)
{
    {
        auto itById = byId_.find(thread->id());
        if (itById == byId_.end()) return false;
        
        byId_.erase(itById);
    }
    return DiscussionThreadCollectionBase::remove(thread);
}

bool DiscussionThreadCollectionWithOrderedId::contains(DiscussionThreadPtr thread) const
{
    return byId_.find(thread->id()) != byId_.end();
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
        auto itById = byId_.find(thread->id());
        if (itById == byId_.end()) return false;

        byId_.erase(itById);
    }
    eraseFromNonUniqueCollection(byLatestMessageCreated_, thread, thread->latestMessageCreated());
    referenceCount_.erase(thread);
    messageCount_ -= thread->messageCount();

    return true;
}

void DiscussionThreadCollectionWithReferenceCountAndMessageCount::clear()
{
    byId_.clear();
    byLatestMessageCreated_.clear();
    referenceCount_.clear();
    messageCount_ = 0;
}

void DiscussionThreadCollectionWithReferenceCountAndMessageCount::updateLatestMessageCreated(DiscussionThreadPtr thread)
{
    replaceInNonUniqueCollection(byLatestMessageCreated_, thread, thread->latestMessageCreated());
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
