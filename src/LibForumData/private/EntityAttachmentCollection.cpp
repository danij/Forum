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

#include "EntityAttachmentCollection.h"

using namespace Forum::Entities;

bool AttachmentCollection::add(const AttachmentPtr attachmentPtr)
{
    if ( ! std::get<1>(byId_.insert(attachmentPtr))) return false;
    byCreated_.insert(attachmentPtr);

    totalSize_ += attachmentPtr->size();

    return true;
}

bool AttachmentCollection::remove(const AttachmentPtr attachmentPtr)
{
    {
        const auto itById = byId_.find(attachmentPtr->id());
        if (itById == byId_.end()) return false;

        byId_.erase(itById);
    }
    eraseFromNonUniqueCollection(byCreated_, attachmentPtr, attachmentPtr->created());

    totalSize_ += attachmentPtr->size();

    return true;
}
