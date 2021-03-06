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

#pragma once

//aggregate all entities headers
#include "EntityUser.h"
#include "EntityDiscussionThread.h"
#include "EntityDiscussionThreadMessage.h"
#include "EntityMessageComment.h"
#include "EntityDiscussionTag.h"
#include "EntityDiscussionCategory.h"

namespace Forum::Entities
{
    struct EntitiesCount
    {
        size_t nrOfUsers;
        size_t nrOfDiscussionThreads;
        size_t nrOfDiscussionMessages;
        size_t nrOfDiscussionTags;
        size_t nrOfDiscussionCategories;
        uint64_t nrOfVisitors;
    };
}
