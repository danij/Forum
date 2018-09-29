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

#include "EntityPrivateMessageCollection.h"

using namespace Forum::Entities;

bool PrivateMessageCollection::add(const PrivateMessagePtr messagePtr)
{
    if ( ! std::get<1>(byId_.insert(messagePtr))) return false;
    byCreated_.insert(messagePtr);

    return true;
}

bool PrivateMessageCollection::remove(PrivateMessagePtr messagePtr)
{
    {
        const auto itById = byId_.find(messagePtr->id());
        if (itById == byId_.end()) return false;

        byId_.erase(itById);
    }
    eraseFromNonUniqueCollection(byCreated_, messagePtr, messagePtr->created());

    return true;
}

bool PrivateMessageGlobalCollection::add(const PrivateMessagePtr messagePtr)
{
    if ( ! std::get<1>(byId_.insert(messagePtr))) return false;

    return true;
}

bool PrivateMessageGlobalCollection::remove(PrivateMessagePtr messagePtr)
{
    {
        const auto itById = byId_.find(messagePtr->id());
        if (itById == byId_.end()) return false;

        byId_.erase(itById);
    }

    return true;
}
