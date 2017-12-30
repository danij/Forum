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

#include "EntityDiscussionThread.h"
#include "EntityCollection.h"

#include "Configuration.h"
#include "ContextProviders.h"

#include <algorithm>

using namespace Forum;
using namespace Forum::Entities;
using namespace Forum::Authorization;

DiscussionThread::ChangeNotification DiscussionThread::changeNotifications_;

DiscussionThreadMessage::VoteScoreType DiscussionThread::voteScore() const
{
    if (messages_.count())
    {
        return (*messages_.byCreated().begin())->voteScore();
    }
    return 0;
}

PrivilegeValueType DiscussionThread::getDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege privilege) const
{
    auto result = DiscussionThreadMessagePrivilegeStore::getDiscussionThreadMessagePrivilege(privilege);
    if (result) return result;

    for (auto tag : tags_)
    {
        assert(tag);
        result = maximumPrivilegeValue(result, tag->getDiscussionThreadMessagePrivilege(privilege));
    }
    return result;
}

PrivilegeValueType DiscussionThread::getDiscussionThreadPrivilege(DiscussionThreadPrivilege privilege) const
{
    auto result = DiscussionThreadPrivilegeStore::getDiscussionThreadPrivilege(privilege);
    if (result) return result;

    for (auto tag : tags_)
    {
        assert(tag);
        result = maximumPrivilegeValue(result, tag->getDiscussionThreadPrivilege(privilege));
    }
    return result;
}

void DiscussionThread::insertMessage(DiscussionThreadMessagePtr message)
{
    if ( ! message) return;

    messages_.add(message);
    updateLatestMessageCreated(std::max(latestMessageCreated_, message->created()));
}

void DiscussionThread::insertMessages(DiscussionThreadMessageCollection& collection)
{
    Timestamp maxCreated{};
    for (DiscussionThreadMessagePtr message : collection.byId())
    {
        if (message)
        {
            maxCreated = std::max(maxCreated, message->created());
        }
    }
    messages_.add(collection);
    updateLatestMessageCreated(std::max(latestMessageCreated_, maxCreated));
}

void DiscussionThread::deleteDiscussionThreadMessage(DiscussionThreadMessagePtr message)
{
    if ( ! message) return;

    messages_.remove(message);
    refreshLatestMessageCreated();
}

void DiscussionThread::refreshLatestMessageCreated()
{
    auto& index = messages_.byCreated();
    auto it = index.rbegin();
    if (it == index.rend() || (! *it))
    {
        updateLatestMessageCreated(0);
        return;
    }
    updateLatestMessageCreated((*it)->created());
}

void DiscussionThread::addVisitorSinceLastEdit(IdTypeRef userId)
{
    if (static_cast<int_fast32_t>(visitorsSinceLastEdit_.size()) >=
        Configuration::getGlobalConfig()->discussionThread.maxUsersInVisitedSinceLastChange)
    {
        visitorsSinceLastEdit_.clear();
    }
    visitorsSinceLastEdit_.insert(userId.value());
}

bool DiscussionThread::hasVisitedSinceLastEdit(IdTypeRef userId) const
{
    return visitorsSinceLastEdit_.find(userId.value()) != visitorsSinceLastEdit_.end();
}

void DiscussionThread::resetVisitorsSinceLastEdit()
{
    visitorsSinceLastEdit_.clear();
}

bool DiscussionThread::addTag(EntityPointer<DiscussionTag> tag)
{
    assert(tag);
    latestVisibleChange() = Context::getCurrentTime();
    return std::get<1>(tags_.insert(tag));
}

bool DiscussionThread::removeTag(EntityPointer<DiscussionTag> tag)
{
    assert(tag);
    latestVisibleChange() = Context::getCurrentTime();
    return tags_.erase(tag) > 0;
}

bool DiscussionThread::addCategory(EntityPointer<DiscussionCategory> category)
{
    assert(category);
    latestVisibleChange() = Context::getCurrentTime();
    return std::get<1>(categories_.insert(category));
}

bool DiscussionThread::removeCategory(EntityPointer<DiscussionCategory> category)
{
    assert(category);
    latestVisibleChange() = Context::getCurrentTime();
    return categories_.erase(category) > 0;
}
