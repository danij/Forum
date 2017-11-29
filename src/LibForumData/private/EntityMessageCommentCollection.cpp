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

#include "EntityMessageCommentCollection.h"

using namespace Forum::Entities;

bool MessageCommentCollection::add(MessageCommentPtr comment)
{
    if ( ! std::get<1>(byId_.insert(comment))) return false;
    byCreated_.insert(comment);

    return true;
}

bool MessageCommentCollection::remove(MessageCommentPtr comment)
{
    {
        auto itById = byId_.find(comment->id());
        if (itById == byId_.end()) return false;

        byId_.erase(itById);
    }
    eraseFromNonUniqueCollection(byCreated_, comment, comment->created());

    return true;
}
