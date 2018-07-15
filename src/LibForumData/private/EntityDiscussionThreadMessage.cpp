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

#include "EntityDiscussionThreadMessage.h"
#include "EntityDiscussionThread.h"

using namespace Forum::Entities;
using namespace Forum::Authorization;

PrivilegeValueType DiscussionThreadMessage::getDiscussionThreadMessagePrivilege(
        DiscussionThreadMessagePrivilege privilege) const
{
    if (const auto result = DiscussionThreadMessagePrivilegeStore::getDiscussionThreadMessagePrivilege(privilege)) 
            return result;

    assert(parentThread_);
    return parentThread_->getDiscussionThreadMessagePrivilege(privilege);
}

PrivilegeValueType DiscussionThreadMessage::getDiscussionThreadMessagePrivilege(
        DiscussionThreadMessagePrivilege privilege, PrivilegeValueType discussionThreadLevelValue) const
{
    auto result = DiscussionThreadMessagePrivilegeStore::getDiscussionThreadMessagePrivilege(privilege);

    return result ? result : discussionThreadLevelValue;
}
