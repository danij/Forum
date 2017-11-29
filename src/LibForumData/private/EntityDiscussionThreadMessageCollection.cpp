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

#include "EntityDiscussionThreadMessageCollection.h"
#include "ContextProviders.h"

#include <algorithm>
#include <vector>

using namespace Forum::Entities;

bool DiscussionThreadMessageCollection::add(DiscussionThreadMessagePtr message)
{
    if (onPrepareCountChange_) onPrepareCountChange_();

    if ( ! std::get<1>(byId_.insert(message))) return false;

    if ( ! Context::isBatchInsertInProgress())
    {
        byCreated_.insert(message);
    }

    if (onCountChange_) onCountChange_();
    return true;
}

bool DiscussionThreadMessageCollection::remove(DiscussionThreadMessagePtr message)
{
    if (onPrepareCountChange_) onPrepareCountChange_();
    {
        auto itById = byId_.find(message->id());
        if (itById == byId_.end()) return false;

        byId_.erase(itById);
    }
    eraseFromFlatMultisetCollection(byCreated_, message);

    if (onCountChange_) onCountChange_();
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

    //sort the values so that insertion into byCreated_ works faster
    std::vector<DiscussionThreadMessagePtr> temp(byId_.begin(), byId_.end());

    std::sort(temp.begin(), temp.end(), [](DiscussionThreadMessagePtr first, DiscussionThreadMessagePtr second)
    {
        return first->created() < second->created();
    });

    byCreated_.insert(boost::container::ordered_range, temp.begin(), temp.end());
}
