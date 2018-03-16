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

#include "EntityDiscussionTagCollection.h"
#include "ContextProviders.h"

using namespace Forum::Entities;

bool DiscussionTagCollection::add(DiscussionTagPtr tag)
{
    if ( ! std::get<1>(byId_.insert(tag))) return false;
    byName_.insert(tag);

    if ( ! Context::isBatchInsertInProgress())
    {
        byThreadCount_.insert(tag);
        byMessageCount_.insert(tag);
    }

    return true;
}

bool DiscussionTagCollection::remove(DiscussionTagPtr tag)
{
    {
        const auto itById = byId_.find(tag->id());
        if (itById == byId_.end()) return false;

        byId_.erase(itById);
    }
    {
        const auto itByName = byName_.find(tag->name());
        if (itByName != byName_.end()) byName_.erase(itByName);
    }
    if ( ! Context::isBatchInsertInProgress())
    {
        eraseFromNonUniqueCollection(byThreadCount_, tag, tag->threadCount());
        eraseFromNonUniqueCollection(byMessageCount_, tag, tag->messageCount());
    }

    return true;
}

void DiscussionTagCollection::stopBatchInsert()
{
    if ( ! Context::isBatchInsertInProgress()) return;

    byThreadCount_.clear();
    byMessageCount_.clear();

    auto byIdIndex = byId_;
    byThreadCount_.insert(byId_.begin(), byId_.end());
    byMessageCount_.insert(byId_.begin(), byId_.end());
}

void DiscussionTagCollection::prepareUpdateName(DiscussionTagPtr tag)
{
    byNameUpdateIt_ = byName_.find(tag->name());
}

void DiscussionTagCollection::updateName(DiscussionTagPtr tag)
{
    if (byNameUpdateIt_ != byName_.end())
    {
        replaceItemInContainer(byName_, byNameUpdateIt_, tag);
    }
}

void DiscussionTagCollection::prepareUpdateThreadCount(DiscussionTagPtr tag)
{
    if (Context::isBatchInsertInProgress()) return;

    byThreadCountUpdateIt_ = findInNonUniqueCollection(byThreadCount_, tag, tag->threadCount());
}

void DiscussionTagCollection::updateThreadCount(DiscussionTagPtr tag)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byThreadCountUpdateIt_ != byThreadCount_.end())
    {
        replaceItemInContainer(byThreadCount_, byThreadCountUpdateIt_, tag);
    }
}

void DiscussionTagCollection::prepareUpdateMessageCount(DiscussionTagPtr tag)
{
    if (Context::isBatchInsertInProgress()) return;

    byMessageCountUpdateIt_ = findInNonUniqueCollection(byMessageCount_, tag, tag->messageCount());
}

void DiscussionTagCollection::updateMessageCount(DiscussionTagPtr tag)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byMessageCountUpdateIt_ != byMessageCount_.end())
    {
        replaceItemInContainer(byMessageCount_, byMessageCountUpdateIt_, tag);
    }
}
