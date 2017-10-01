#include "EntityDiscussionThreadCollection.h"
#include "ContextProviders.h"

using namespace Forum::Entities;

bool DiscussionThreadCollectionBase::add(DiscussionThreadPtr thread)
{
    if (onPrepareCountChange_) onPrepareCountChange_();

    if ( ! Context::isBatchInsertInProgress())
    {
        byName_.insert(thread);
        byCreated_.insert(thread);
        byLastUpdated_.insert(thread);
        byLatestMessageCreated_.insert(thread);
        byMessageCount_.insert(thread);
    }

    if (onCountChange_) onCountChange_();
    return true;
}

bool DiscussionThreadCollectionBase::remove(DiscussionThreadPtr thread)
{
    if (onPrepareCountChange_) onPrepareCountChange_();

    if ( ! Context::isBatchInsertInProgress())
    {
        eraseFromNonUniqueCollection(byName_, thread, thread->name());
        eraseFromNonUniqueCollection(byCreated_, thread, thread->created());
        eraseFromNonUniqueCollection(byLastUpdated_, thread, thread->lastUpdated());
        eraseFromNonUniqueCollection(byLatestMessageCreated_, thread, thread->latestMessageCreated());
        eraseFromNonUniqueCollection(byMessageCount_, thread, thread->messageCount());
    }

    if (onCountChange_) onCountChange_();
    return true;
}

void DiscussionThreadCollectionBase::stopBatchInsert()
{
    if ( ! Context::isBatchInsertInProgress()) return;

    byName_.clear();
    byCreated_.clear();
    byLastUpdated_.clear();
    byLatestMessageCreated_.clear();
    byMessageCount_.clear();

    iterateAllThreads([&](DiscussionThreadPtr thread)
    {
        byName_.insert(thread);
        byCreated_.insert(thread);
        byLastUpdated_.insert(thread);
        byLatestMessageCreated_.insert(thread);
        byMessageCount_.insert(thread);
    });
}

void DiscussionThreadCollectionBase::prepareUpdateName(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    byNameUpdateIt_ = findInNonUniqueCollection(byName_, thread, thread->name());
}

void DiscussionThreadCollectionBase::updateName(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byNameUpdateIt_ != byName_.end())
    {
        byName_.replace(byNameUpdateIt_, thread);
    }
}

void DiscussionThreadCollectionBase::prepareUpdateLastUpdated(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    byLastUpdatedUpdateIt_ = findInNonUniqueCollection(byLastUpdated_, thread, thread->lastUpdated());
}

void DiscussionThreadCollectionBase::updateLastUpdated(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byLastUpdatedUpdateIt_ != byLastUpdated_.end())
    {
        byLastUpdated_.replace(byLastUpdatedUpdateIt_, thread);
    }
}

void DiscussionThreadCollectionBase::prepareUpdateLatestMessageCreated(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    byLatestMessageCreatedUpdateIt_ = findInNonUniqueCollection(byLatestMessageCreated_, thread, thread->latestMessageCreated());
}

void DiscussionThreadCollectionBase::updateLatestMessageCreated(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byLatestMessageCreatedUpdateIt_ != byLatestMessageCreated_.end())
    {
        byLatestMessageCreated_.replace(byLatestMessageCreatedUpdateIt_, thread);
    }
}

void DiscussionThreadCollectionBase::prepareUpdateMessageCount(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    byMessageCountUpdateIt_ = findInNonUniqueCollection(byMessageCount_, thread, thread->messageCount());
}

void DiscussionThreadCollectionBase::updateMessageCount(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byMessageCountUpdateIt_ != byMessageCount_.end())
    {
        byMessageCount_.replace(byMessageCountUpdateIt_, thread);
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

void DiscussionThreadCollectionWithHashedId::iterateAllThreads(std::function<void(DiscussionThreadPtr)>&& callback)
{
    for (auto ptr : byId())
    {
        callback(ptr);
    }
}

bool DiscussionThreadCollectionWithHashedIdAndPinOrder::add(DiscussionThreadPtr thread)
{
    if ( ! DiscussionThreadCollectionWithHashedId::add(thread)) return false;

    if ( ! Context::isBatchInsertInProgress())
    {
        byPinDisplayOrder_.insert(thread);
    }
    return true;
}

bool DiscussionThreadCollectionWithHashedIdAndPinOrder::remove(DiscussionThreadPtr thread)
{
    if ( ! DiscussionThreadCollectionWithHashedId::remove(thread)) return false;

    if ( ! Context::isBatchInsertInProgress())
    {
        eraseFromNonUniqueCollection(byPinDisplayOrder_, thread, thread->pinDisplayOrder());
    }

    return true;
}

void DiscussionThreadCollectionWithHashedIdAndPinOrder::stopBatchInsert()
{
    if ( ! Context::isBatchInsertInProgress()) return;

    byPinDisplayOrder_.clear();

    auto byIdIndex = byId();
    byPinDisplayOrder_.insert(byIdIndex.begin(), byIdIndex.end());
}

void DiscussionThreadCollectionWithHashedIdAndPinOrder::prepareUpdatePinDisplayOrder(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    byPinDisplayOrderUpdateIt_ = findInNonUniqueCollection(byPinDisplayOrder_, thread, thread->pinDisplayOrder());
}

void DiscussionThreadCollectionWithHashedIdAndPinOrder::updatePinDisplayOrder(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byPinDisplayOrderUpdateIt_ != byPinDisplayOrder_.end())
    {
        byPinDisplayOrder_.replace(byPinDisplayOrderUpdateIt_, thread);
    }
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

void DiscussionThreadCollectionWithOrderedId::iterateAllThreads(std::function<void(DiscussionThreadPtr)>&& callback)
{
    for (auto ptr : byId())
    {
        callback(ptr);
    }
}

bool DiscussionThreadCollectionWithReferenceCountAndMessageCount::add(DiscussionThreadPtr thread)
{
    return add(thread, 1);
}

bool DiscussionThreadCollectionWithReferenceCountAndMessageCount::add(DiscussionThreadPtr thread, int_fast32_t amount)
{
    auto it = referenceCount_.find(thread);

    if (it == referenceCount_.end())
    {
        if ( ! std::get<1>(byId_.insert(thread))) return false;

        if ( ! Context::isBatchInsertInProgress())
        {
            byLatestMessageCreated_.insert(thread);
        }
        referenceCount_.insert(std::make_pair(thread, amount));
        messageCount_ += static_cast<decltype(messageCount_)>(thread->messageCount());
        return true;
    }
    it->second += amount;
    return false;
}

void DiscussionThreadCollectionWithReferenceCountAndMessageCount::add(
        const DiscussionThreadCollectionWithReferenceCountAndMessageCount& collection)
{
    for (auto pair : collection.referenceCount_)
    {
        add(pair.first, pair.second);
    }
}

void DiscussionThreadCollectionWithReferenceCountAndMessageCount::decreaseReferenceCount(DiscussionThreadPtr thread)
{
    assert(thread);

    auto it = referenceCount_.find(thread);
    if (it == referenceCount_.end())
    {
        return;
    }
    if (--(it->second) < 1)
    {
        referenceCount_.erase(it);
        remove(thread);
    }
}


void DiscussionThreadCollectionWithReferenceCountAndMessageCount::decreaseReferenceCount(
        const DiscussionThreadCollectionWithReferenceCountAndMessageCount& collection)
{
    std::vector<DiscussionThreadPtr> toRemove;

    for (auto pair : collection.referenceCount_)
    {
        auto it = referenceCount_.find(pair.first);
        if (it == referenceCount_.end()) continue;
        it->second -= pair.second;
        if (it->second < 1)
        {
            toRemove.push_back(it->first);
        }
    }

    for (auto thread : toRemove)
    {
        referenceCount_.erase(thread);
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
    if ( ! Context::isBatchInsertInProgress())
    {
        eraseFromNonUniqueCollection(byLatestMessageCreated_, thread, thread->latestMessageCreated());
    }
    referenceCount_.erase(thread);
    messageCount_ -= static_cast<decltype(messageCount_)>(thread->messageCount());

    return true;
}

void DiscussionThreadCollectionWithReferenceCountAndMessageCount::clear()
{
    byId_.clear();
    byLatestMessageCreated_.clear();
    referenceCount_.clear();
    messageCount_ = 0;
}

void DiscussionThreadCollectionWithReferenceCountAndMessageCount::stopBatchInsert()
{
    if ( ! Context::isBatchInsertInProgress()) return;

    byLatestMessageCreated_.clear();

    auto byIdIndex = byId();
    byLatestMessageCreated_.insert(byIdIndex.begin(), byIdIndex.end());
}

void DiscussionThreadCollectionWithReferenceCountAndMessageCount::prepareUpdateLatestMessageCreated(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    byLatestMessageCreatedUpdateIt_ = findInNonUniqueCollection(byLatestMessageCreated_, thread, thread->latestMessageCreated());
}

void DiscussionThreadCollectionWithReferenceCountAndMessageCount::updateLatestMessageCreated(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

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
