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

#include "EntityDiscussionThreadMessageCollection.h"
#include "ContextProviders.h"

using namespace Forum::Entities;

bool DiscussionThreadMessageCollection::add(DiscussionThreadMessagePtr message)
{
    onPrepareCountChange_();

    if ( ! std::get<1>(byId_.insert(message))) return false;

    if ( ! Context::isBatchInsertInProgress())
    {
        byCreated_.insert(message);
    }

    onCountChange_();
    return true;
}

bool DiscussionThreadMessageCollection::add(DiscussionThreadMessageCollection& collection)
{
    onPrepareCountChange_();

    auto result = false;

    for (const DiscussionThreadMessagePtr message : collection.byId())
    {
        if ( ! std::get<1>(byId_.insert(message))) continue;

        if ( ! Context::isBatchInsertInProgress())
        {
            byCreated_.insert(message);
        }
        result = true;
    }

    onCountChange_();
    return result;
}

bool DiscussionThreadMessageCollection::remove(DiscussionThreadMessagePtr message)
{
    onPrepareCountChange_();
    {
        const auto itById = byId_.find(message->id());
        if (itById == byId_.end()) return false;

        byId_.erase(itById);
    }
    eraseFromNonUniqueCollection(byCreated_, message, message->created());

    onCountChange_();
    return true;
}

void DiscussionThreadMessageCollection::clear()
{
    byId_.clear();
    byCreated_.clear();
}

void DiscussionThreadMessageCollection::stopBatchInsert()
{
    if ( ! Context::isBatchInsertInProgress()) return;

    byCreated_.clear();
    byCreated_.insert(byId_.begin(), byId_.end());
}

///
//Low Memory
///
bool DiscussionThreadMessageCollectionLowMemory::add(DiscussionThreadMessagePtr message)
{
    onPrepareCountChange_();

    if (Context::isBatchInsertInProgress())
    {
        if ( ! byIdDuringBatchInsert_)
        {
            byIdDuringBatchInsert_ = std::make_unique<decltype(byIdDuringBatchInsert_)::element_type>();
        }
        if ( ! std::get<1>(byIdDuringBatchInsert_->insert({ message->id(), message }))) return false;
    }
    else
    {
        if ( ! std::get<1>(byId_.insert(message))) return false;
        byCreated_.insert(message);
    }

    onCountChange_();
    return true;
}

bool DiscussionThreadMessageCollectionLowMemory::add(DiscussionThreadMessageCollectionLowMemory& collection)
{
    onPrepareCountChange_();

    auto result = false;

    if (Context::isBatchInsertInProgress())
    {
        if ( ! byIdDuringBatchInsert_)
        {
            byIdDuringBatchInsert_ = std::make_unique<decltype(byIdDuringBatchInsert_)::element_type>();
        }
        auto countBefore = byIdDuringBatchInsert_->size();

        if (collection.byIdDuringBatchInsert_)
        {
            for (auto& [key, value] : *collection.byIdDuringBatchInsert_)
            {
                (*byIdDuringBatchInsert_)[key] = value;
            }
        }

        auto countAfter = byIdDuringBatchInsert_->size();
        result = countAfter > countBefore;
    }
    else
    {
        for (const DiscussionThreadMessagePtr message : collection.byId())
        {
            if ( ! std::get<1>(byId_.insert(message))) continue;
            byCreated_.insert(message);
            result = true;
        }
    }

    onCountChange_();
    return result;
}

bool DiscussionThreadMessageCollectionLowMemory::remove(DiscussionThreadMessagePtr message)
{
    onPrepareCountChange_();
    if (byIdDuringBatchInsert_)
    {
        const auto itById = byIdDuringBatchInsert_->find(message->id());
        if (itById == byIdDuringBatchInsert_->end()) return false;

        byIdDuringBatchInsert_->erase(itById);
    }
    else 
    {
        const auto itById = byId_.find(message->id());
        if (itById == byId_.end()) return false;

        byId_.erase(itById);
    }
    eraseFromNonUniqueCollection(byCreated_, message, message->created());

    onCountChange_();
    return true;
}

void DiscussionThreadMessageCollectionLowMemory::clear()
{
    byIdDuringBatchInsert_.reset();
    byId_.clear();
    byCreated_.clear();
}

void DiscussionThreadMessageCollectionLowMemory::stopBatchInsert()
{
    if ( ! Context::isBatchInsertInProgress()) return;

    byId_.clear();
    if (byIdDuringBatchInsert_)
    {
        std::vector<DiscussionThreadMessagePtr> byId{ byIdDuringBatchInsert_->size() };
        std::vector<DiscussionThreadMessagePtr>::size_type byIdIndex = 0;
        for (auto& [key, value] : *byIdDuringBatchInsert_)
        {
            byId[byIdIndex++] = value;
        }
        byId_.insertAlreadyUnique(byId.begin(), byId.end());
        byIdDuringBatchInsert_.reset();
    }

    byCreated_.clear();
    byCreated_.insert(byId_.begin(), byId_.end());
}
