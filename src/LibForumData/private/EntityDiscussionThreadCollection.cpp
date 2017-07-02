#include "EntityDiscussionThreadCollection.h"

using namespace Forum::Entities;

bool DiscussionThreadCollectionBase::add(DiscussionThreadPtr thread)
{
    if (onPrepareCountChange_) onPrepareCountChange_();

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
    if (onPrepareCountChange_) onPrepareCountChange_();

    eraseFromNonUniqueCollection(byName_, thread, thread->name());
    eraseFromNonUniqueCollection(byCreated_, thread, thread->created());
    eraseFromNonUniqueCollection(byLastUpdated_, thread, thread->lastUpdated());
    eraseFromNonUniqueCollection(byLatestMessageCreated_, thread, thread->latestMessageCreated());
    eraseFromNonUniqueCollection(byMessageCount_, thread, thread->messageCount());
    eraseFromNonUniqueCollection(byPinDisplayOrder_, thread, thread->pinDisplayOrder());

    if (onCountChange_) onCountChange_();
    return true;
}

void DiscussionThreadCollectionBase::prepareUpdateName(DiscussionThreadPtr thread)
{
    byNameUpdateIt_ = findInNonUniqueCollection(byName_, thread, thread->name());
}

void DiscussionThreadCollectionBase::updateName(DiscussionThreadPtr thread)
{
    if (byNameUpdateIt_ != byName_.end())
    {
        byName_.replace(byNameUpdateIt_, thread);
    }
}

void DiscussionThreadCollectionBase::prepareUpdateLastUpdated(DiscussionThreadPtr thread)
{
    byLastUpdatedUpdateIt_ = findInNonUniqueCollection(byLastUpdated_, thread, thread->lastUpdated());
}

void DiscussionThreadCollectionBase::updateLastUpdated(DiscussionThreadPtr thread)
{
    if (byLastUpdatedUpdateIt_ != byLastUpdated_.end())
    {
        byLastUpdated_.replace(byLastUpdatedUpdateIt_, thread);
    }
}

void DiscussionThreadCollectionBase::prepareUpdateLatestMessageCreated(DiscussionThreadPtr thread)
{
    byLatestMessageCreatedUpdateIt_ = findInNonUniqueCollection(byLatestMessageCreated_, thread, thread->latestMessageCreated());
}

void DiscussionThreadCollectionBase::updateLatestMessageCreated(DiscussionThreadPtr thread)
{
    if (byLatestMessageCreatedUpdateIt_ != byLatestMessageCreated_.end())
    {
        byLatestMessageCreated_.replace(byLatestMessageCreatedUpdateIt_, thread);
    }
}

void DiscussionThreadCollectionBase::prepareUpdateMessageCount(DiscussionThreadPtr thread)
{
    byMessageCountUpdateIt_ = findInNonUniqueCollection(byMessageCount_, thread, thread->messageCount());
}

void DiscussionThreadCollectionBase::updateMessageCount(DiscussionThreadPtr thread)
{
    if (byMessageCountUpdateIt_ != byMessageCount_.end())
    {
        byMessageCount_.replace(byMessageCountUpdateIt_, thread);
    }
}

void DiscussionThreadCollectionBase::prepareUpdatePinDisplayOrder(DiscussionThreadPtr thread)
{
    byPinDisplayOrderUpdateIt_ = findInNonUniqueCollection(byPinDisplayOrder_, thread, thread->pinDisplayOrder());
}

void DiscussionThreadCollectionBase::updatePinDisplayOrder(DiscussionThreadPtr thread)
{
    if (byPinDisplayOrderUpdateIt_ != byPinDisplayOrder_.end())
    {
        byPinDisplayOrder_.replace(byPinDisplayOrderUpdateIt_, thread);
    }
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

void DiscussionThreadCollectionWithReferenceCountAndMessageCount::prepareUpdateLatestMessageCreated(DiscussionThreadPtr thread)
{
    byLatestMessageCreatedUpdateIt_ = findInNonUniqueCollection(byLatestMessageCreated_, thread, thread->latestMessageCreated());
}

void DiscussionThreadCollectionWithReferenceCountAndMessageCount::updateLatestMessageCreated(DiscussionThreadPtr thread)
{
    if (byLatestMessageCreatedUpdateIt_ != byLatestMessageCreated_.end())
    {
        byLatestMessageCreated_.replace(byLatestMessageCreatedUpdateIt_, thread);
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
