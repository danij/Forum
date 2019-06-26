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

#include "EntityDiscussionThreadCollection.h"

#include <boost/iterator/transform_iterator.hpp>

using namespace Forum::Entities;

bool DiscussionThreadCollectionWithHashedId::add(DiscussionThreadPtr thread)
{
    if (byId_.find(thread->id()) != byId_.end()) return false;

    prepareCountChange();
    if ( ! std::get<1>(byId_.insert(thread)))
    {
        finishCountChange();
        return false;
    }

    if ( ! Context::isBatchInsertInProgress())
    {
        byName_.insert(thread);
        byCreated_.insert(thread);
        byLastUpdated_.insert(thread);
        byLatestMessageCreated_.insert(thread);
        byMessageCount_.insert(thread);
    }

    finishCountChange();
    return true;
}

bool DiscussionThreadCollectionWithHashedId::add(DiscussionThreadPtr* threads, const size_t threadCount)
{
    if (threadCount < 1) return false;
    assert(threads);

    const auto sizeBefore = count();

    prepareCountChange();

    const auto valuesBegin = threads;
    const auto valuesEnd = threads + threadCount;

    byId_.insert(valuesBegin, valuesEnd);

    if ( ! Context::isBatchInsertInProgress())
    {
        byName_.insert(valuesBegin, valuesEnd);
        byCreated_.insert(valuesBegin, valuesEnd);
        byLastUpdated_.insert(valuesBegin, valuesEnd);
        byLatestMessageCreated_.insert(valuesBegin, valuesEnd);
        byMessageCount_.insert(valuesBegin, valuesEnd);
    }

    finishCountChange();

    const auto sizeAfter = count();

    assert(sizeAfter >= sizeBefore);

    return sizeAfter != sizeBefore;
}

void DiscussionThreadCollectionWithHashedId::prepareCountChange()
{
    onPrepareCountChange_();
}

void DiscussionThreadCollectionWithHashedId::finishCountChange()
{
    onCountChange_();
}

bool DiscussionThreadCollectionWithHashedId::remove(DiscussionThreadPtr thread)
{
    assert(thread);

    prepareCountChange();
    {
        const auto itById = byId_.find(thread->id());
        if (itById == byId_.end())
        {
            finishCountChange();
            return false;
        }

        byId_.erase(itById);
    }

    if ( ! Context::isBatchInsertInProgress())
    {
        eraseFromNonUniqueCollection(byName_, thread, thread->name());
        eraseFromNonUniqueCollection(byCreated_, thread, thread->created());
        eraseFromNonUniqueCollection(byLastUpdated_, thread, thread->lastUpdated());
        eraseFromNonUniqueCollection(byLatestMessageCreated_, thread, thread->latestMessageCreated());
        eraseFromNonUniqueCollection(byMessageCount_, thread, thread->messageCount());
    }

    finishCountChange();
    return true;
}

bool DiscussionThreadCollectionWithHashedId::contains(DiscussionThreadPtr thread) const
{
    assert(thread);
    return findById(thread->id());
}

const DiscussionThread* DiscussionThreadCollectionWithHashedId::findById(IdTypeRef id) const
{
    return (const_cast<DiscussionThreadCollectionWithHashedId*>(this))->findById(id);
}


DiscussionThreadPtr DiscussionThreadCollectionWithHashedId::findById(IdTypeRef id)
{
    const auto it = byId_.find(id);
    if (it != byId_.end())
    {
        return *it;
    }
    return {};
}

void DiscussionThreadCollectionWithHashedId::prepareUpdateName(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    byNameUpdateIt_ = findInNonUniqueCollection(byName_, thread, thread->name());
}

void DiscussionThreadCollectionWithHashedId::updateName(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byNameUpdateIt_ != byName_.end())
    {
        replaceItemInContainer(byName_, byNameUpdateIt_, thread);
    }
}

void DiscussionThreadCollectionWithHashedId::prepareUpdateLastUpdated(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    byLastUpdatedUpdateIt_ = findInNonUniqueCollection(byLastUpdated_, thread, thread->lastUpdated());
}

void DiscussionThreadCollectionWithHashedId::updateLastUpdated(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byLastUpdatedUpdateIt_ != byLastUpdated_.end())
    {
        replaceItemInContainer(byLastUpdated_, byLastUpdatedUpdateIt_, thread);
    }
}

void DiscussionThreadCollectionWithHashedId::prepareUpdateLatestMessageCreated(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    byLatestMessageCreatedUpdateIt_ = findInNonUniqueCollection(byLatestMessageCreated_, thread, 
                                                                thread->latestMessageCreated());
}

void DiscussionThreadCollectionWithHashedId::updateLatestMessageCreated(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byLatestMessageCreatedUpdateIt_ != byLatestMessageCreated_.end())
    {
        replaceItemInContainer(byLatestMessageCreated_, byLatestMessageCreatedUpdateIt_, thread);
    }
}

void DiscussionThreadCollectionWithHashedId::prepareUpdateMessageCount(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    byMessageCountUpdateIt_ = findInNonUniqueCollection(byMessageCount_, thread, thread->messageCount());
}

void DiscussionThreadCollectionWithHashedId::updateMessageCount(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byMessageCountUpdateIt_ != byMessageCount_.end())
    {
        replaceItemInContainer(byMessageCount_, byMessageCountUpdateIt_, thread);
    }
}

void DiscussionThreadCollectionWithHashedId::stopBatchInsert()
{
    if ( ! Context::isBatchInsertInProgress()) return;

    onStopBatchInsert();
}

void DiscussionThreadCollectionWithHashedId::onStopBatchInsert()
{
    const auto valuesBegin = byId_.begin();
    const auto valuesEnd = byId_.end();

    byName_.clear();
    byCreated_.clear();
    byLastUpdated_.clear();
    byLatestMessageCreated_.clear();
    byMessageCount_.clear();
    
    byName_.insert(valuesBegin, valuesEnd);
    byCreated_.insert(valuesBegin, valuesEnd);
    byLastUpdated_.insert(valuesBegin, valuesEnd);
    byLatestMessageCreated_.insert(valuesBegin, valuesEnd);
    byMessageCount_.insert(valuesBegin, valuesEnd);
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

bool DiscussionThreadCollectionWithHashedIdAndPinOrder::add(DiscussionThreadPtr* threads, const size_t threadCount)
{
    if ( ! DiscussionThreadCollectionWithHashedId::add(threads, threadCount)) return false;

    if ( ! Context::isBatchInsertInProgress())
    {
        byPinDisplayOrder_.insert(threads, threads + threadCount);
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

void DiscussionThreadCollectionWithHashedIdAndPinOrder::onStopBatchInsert()
{
    DiscussionThreadCollectionWithHashedId::onStopBatchInsert();
    
    const auto valuesBegin = byId_.begin();
    const auto valuesEnd = byId_.end();

    byPinDisplayOrder_.clear();
    byPinDisplayOrder_.insert(valuesBegin, valuesEnd);
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
        replaceItemInContainer(byPinDisplayOrder_, byPinDisplayOrderUpdateIt_, thread);
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
    for (const auto pair : collection.referenceCount_)
    {
        add(pair.first, pair.second);
    }
}

void DiscussionThreadCollectionWithReferenceCountAndMessageCount::decreaseReferenceCount(DiscussionThreadPtr thread)
{
    assert(thread);

    const auto it = referenceCount_.find(thread);
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

    for (const auto pair : collection.referenceCount_)
    {
        auto it = referenceCount_.find(pair.first);
        if (it == referenceCount_.end()) continue;
        it->second -= pair.second;
        if (it->second < 1)
        {
            toRemove.push_back(it->first);
        }
    }

    for (const auto thread : toRemove)
    {
        referenceCount_.erase(thread);
    }
}

bool DiscussionThreadCollectionWithReferenceCountAndMessageCount::remove(DiscussionThreadPtr thread)
{
    assert(thread);
    {
        const auto itById = byId_.find(thread->id());
        if (itById == byId_.end()) return false;

        byId_.erase(itById);
    }
    referenceCount_.erase(thread);
    messageCount_ -= static_cast<decltype(messageCount_)>(thread->messageCount());

    return true;
}

void DiscussionThreadCollectionWithReferenceCountAndMessageCount::clear()
{
    byId_.clear();
    referenceCount_.clear();
    messageCount_ = 0;
}

void DiscussionThreadCollectionWithReferenceCountAndMessageCount::stopBatchInsert()
{
    if ( ! Context::isBatchInsertInProgress()) return;
}

///
//Low Memory
///
bool DiscussionThreadCollectionLowMemory::add(DiscussionThreadPtr thread)
{
    if (byId_.find(thread->id()) != byId_.end()) return false;

    prepareCountChange();
    if ( ! std::get<1>(byId_.insert(thread)))
    {
        finishCountChange();
        return false;
    }

    if ( ! Context::isBatchInsertInProgress())
    {
        byName_.insert(thread);
        byCreated_.insert(thread);
        byLastUpdated_.insert(thread);
        byLatestMessageCreated_.insert(thread);
        byMessageCount_.insert(thread);
    }

    finishCountChange();
    return true;
}

bool DiscussionThreadCollectionLowMemory::remove(DiscussionThreadPtr thread)
{
    prepareCountChange();
    {
        const auto itById = byId_.find(thread->id());
        if (itById == byId_.end())
        {
            finishCountChange();
            return false;
        }

        byId_.erase(itById);
    }

    if ( ! Context::isBatchInsertInProgress())
    {
        eraseFromNonUniqueCollection(byName_, thread, thread->name());
        eraseFromNonUniqueCollection(byCreated_, thread, thread->created());
        eraseFromNonUniqueCollection(byLastUpdated_, thread, thread->lastUpdated());
        eraseFromNonUniqueCollection(byLatestMessageCreated_, thread, thread->latestMessageCreated());
        eraseFromNonUniqueCollection(byMessageCount_, thread, thread->messageCount());
    }

    finishCountChange();
    return true;
}

bool DiscussionThreadCollectionLowMemory::contains(DiscussionThreadPtr thread) const
{
    return byId_.find(thread->id()) != byId_.end();
}

void DiscussionThreadCollectionLowMemory::prepareCountChange()
{
    onPrepareCountChange_();
}

void DiscussionThreadCollectionLowMemory::finishCountChange()
{
    onCountChange_();
}

void DiscussionThreadCollectionLowMemory::stopBatchInsert()
{
    if ( ! Context::isBatchInsertInProgress()) return;

    byName_.clear();
    byCreated_.clear();
    byLastUpdated_.clear();
    byLatestMessageCreated_.clear();
    byMessageCount_.clear();

    byName_.insert(byId_.begin(), byId_.end());
    byCreated_.insert(byId_.begin(), byId_.end());
    byLastUpdated_.insert(byId_.begin(), byId_.end());
    byLatestMessageCreated_.insert(byId_.begin(), byId_.end());
    byMessageCount_.insert(byId_.begin(), byId_.end());
}

void DiscussionThreadCollectionLowMemory::prepareUpdateName(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    byNameUpdateIt_ = findInNonUniqueCollection(byName_, thread, thread->name());
}

void DiscussionThreadCollectionLowMemory::updateName(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byNameUpdateIt_ != byName_.end())
    {
        replaceItemInContainer(byName_, byNameUpdateIt_, thread);
    }
}

void DiscussionThreadCollectionLowMemory::prepareUpdateLastUpdated(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    byLastUpdatedUpdateIt_ = findInNonUniqueCollection(byLastUpdated_, thread, thread->lastUpdated());
}

void DiscussionThreadCollectionLowMemory::updateLastUpdated(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byLastUpdatedUpdateIt_ != byLastUpdated_.end())
    {
        replaceItemInContainer(byLastUpdated_, byLastUpdatedUpdateIt_, thread);
    }
}

void DiscussionThreadCollectionLowMemory::prepareUpdateLatestMessageCreated(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    byLatestMessageCreatedUpdateIt_ = findInNonUniqueCollection(byLatestMessageCreated_, thread, thread->latestMessageCreated());
}

void DiscussionThreadCollectionLowMemory::updateLatestMessageCreated(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byLatestMessageCreatedUpdateIt_ != byLatestMessageCreated_.end())
    {
        replaceItemInContainer(byLatestMessageCreated_, byLatestMessageCreatedUpdateIt_, thread);
    }
}

void DiscussionThreadCollectionLowMemory::prepareUpdateMessageCount(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    byMessageCountUpdateIt_ = findInNonUniqueCollection(byMessageCount_, thread, thread->messageCount());
}

void DiscussionThreadCollectionLowMemory::updateMessageCount(DiscussionThreadPtr thread)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byMessageCountUpdateIt_ != byMessageCount_.end())
    {
        replaceItemInContainer(byMessageCount_, byMessageCountUpdateIt_, thread);
    }
}
