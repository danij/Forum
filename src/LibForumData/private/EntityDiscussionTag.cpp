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

#include "EntityDiscussionTag.h"
#include "EntityCollection.h"

using namespace Forum;
using namespace Forum::Entities;
using namespace Forum::Authorization;

DiscussionTag::ChangeNotification DiscussionTag::changeNotifications_;

PrivilegeValueType DiscussionTag::getDiscussionThreadMessagePrivilege(DiscussionThreadMessagePrivilege privilege) const
{
    auto result = DiscussionThreadMessagePrivilegeStore::getDiscussionThreadMessagePrivilege(privilege);
    if (result) return result;

    return forumWidePrivileges_.getDiscussionThreadMessagePrivilege(privilege);
}

PrivilegeValueType DiscussionTag::getDiscussionThreadPrivilege(DiscussionThreadPrivilege privilege) const
{
    auto result = DiscussionThreadPrivilegeStore::getDiscussionThreadPrivilege(privilege);
    if (result) return result;

    return forumWidePrivileges_.getDiscussionThreadPrivilege(privilege);
}

PrivilegeValueType DiscussionTag::getDiscussionTagPrivilege(DiscussionTagPrivilege privilege) const
{
    auto result = DiscussionTagPrivilegeStore::getDiscussionTagPrivilege(privilege);
    if (result) return result;

    return forumWidePrivileges_.getDiscussionTagPrivilege(privilege);
}

bool DiscussionTag::insertDiscussionThread(DiscussionThreadPtr thread)
{
    assert(thread);
    changeNotifications_.onPrepareUpdateThreadCount(*this);

    if ( ! threads_.add(thread))
    {
        return false;
    }
    messageCount_ += static_cast<decltype(messageCount_)>(thread->messageCount());

    for (auto category : categories_)
    {
        assert(category);
        category->insertDiscussionThread(thread);
    }
    changeNotifications_.onUpdateThreadCount(*this);
    return true;
}

bool DiscussionTag::deleteDiscussionThread(DiscussionThreadPtr thread)
{
    assert(thread);
    changeNotifications_.onPrepareUpdateThreadCount(*this);

    if ( ! threads_.remove(thread))
    {
        return false;
    }
    messageCount_ -= static_cast<decltype(messageCount_)>(thread->messageCount());

    for (auto category : categories_)
    {
        assert(category);
        //called from detaching a tag from a thread
        category->deleteDiscussionThreadIfNoOtherTagsReferenceIt(thread);
    }
    changeNotifications_.onUpdateThreadCount(*this);
    return true;
}

bool DiscussionTag::addCategory(EntityPointer<DiscussionCategory> category)
{
    assert(category);
    return std::get<1>(categories_.insert(std::move(category)));
}

bool DiscussionTag::removeCategory(EntityPointer<DiscussionCategory> category)
{
    assert(category);
    return categories_.erase(category) > 0;
}
