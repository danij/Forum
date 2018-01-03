/*
Fast Forum Backend
Copyright (C) 2016-2017 Daniel Jurcau

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

#include "EntityDiscussionCategoryCollection.h"
#include "ContextProviders.h"

using namespace Forum::Entities;

bool DiscussionCategoryCollection::add(DiscussionCategoryPtr category)
{
    if ( ! std::get<1>(byId_.insert(category))) return false;
    byName_.insert(category);

    if ( ! Context::isBatchInsertInProgress())
    {
        byMessageCount_.insert(category);
        byDisplayOrderRootPriority_.insert(category);
    }

    return true;
}

bool DiscussionCategoryCollection::remove(DiscussionCategoryPtr category)
{
    {
        auto itById = byId_.find(category->id());
        if (itById == byId_.end()) return false;

        byId_.erase(itById);
    }
    {
        auto itByName = byName_.find(category->name());
        if (itByName != byName_.end()) byName_.erase(itByName);
    }
    if ( ! Context::isBatchInsertInProgress())
    {
        eraseFromNonUniqueCollection(byMessageCount_, category, category->messageCount());
        eraseFromNonUniqueCollection(byDisplayOrderRootPriority_, category, category->displayOrderWithRootPriority());
    }
    return true;
}

void DiscussionCategoryCollection::stopBatchInsert()
{
    if ( ! Context::isBatchInsertInProgress()) return;

    byMessageCount_.clear();
    byMessageCount_.insert(byId_.begin(), byId_.end());

    byDisplayOrderRootPriority_.clear();
    byDisplayOrderRootPriority_.insert(byId_.begin(), byId_.end());
}

void DiscussionCategoryCollection::prepareUpdateName(DiscussionCategoryPtr category)
{
    byNameUpdateIt_ = byName_.find(category->name());
}

void DiscussionCategoryCollection::updateName(DiscussionCategoryPtr category)
{
    if (byNameUpdateIt_ != byName_.end())
    {
        replaceItemInContainer(byName_, byNameUpdateIt_, category);
    }
}

void DiscussionCategoryCollection::prepareUpdateMessageCount(DiscussionCategoryPtr category)
{
    if (Context::isBatchInsertInProgress()) return;

    byMessageCountUpdateIt_ = findInNonUniqueCollection(byMessageCount_, category, category->messageCount());
}

void DiscussionCategoryCollection::updateMessageCount(DiscussionCategoryPtr category)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byMessageCountUpdateIt_ != byMessageCount_.end())
    {
        replaceItemInContainer(byMessageCount_, byMessageCountUpdateIt_, category);
    }
}

void DiscussionCategoryCollection::prepareUpdateDisplayOrderRootPriority(DiscussionCategoryPtr category)
{
    if (Context::isBatchInsertInProgress()) return;

    byDisplayOrderRootPriorityUpdateIt_ = findInNonUniqueCollection(byDisplayOrderRootPriority_, category,
                                                                    category->displayOrderWithRootPriority());
}

void DiscussionCategoryCollection::updateDisplayOrderRootPriority(DiscussionCategoryPtr category)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byDisplayOrderRootPriorityUpdateIt_ != byDisplayOrderRootPriority_.end())
    {
        replaceItemInContainer(byDisplayOrderRootPriority_, byDisplayOrderRootPriorityUpdateIt_, category);
    }
}
