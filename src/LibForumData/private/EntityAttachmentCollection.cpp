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
#include "ContextProviders.h"

using namespace Forum::Entities;

bool AttachmentCollection::add(const AttachmentPtr attachmentPtr)
{
    if ( ! std::get<1>(byId_.insert(attachmentPtr))) return false;

    if ( ! Context::isBatchInsertInProgress())
    {
        byCreated_.insert(attachmentPtr);
        byName_.insert(attachmentPtr);
        bySize_.insert(attachmentPtr);
        byApproval_.insert(attachmentPtr);
    }

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
    if ( ! Context::isBatchInsertInProgress())
    {
        eraseFromNonUniqueCollection(byCreated_, attachmentPtr, attachmentPtr->created());
        eraseFromNonUniqueCollection(byName_, attachmentPtr, attachmentPtr->name());
        eraseFromNonUniqueCollection(bySize_, attachmentPtr, attachmentPtr->size());
        eraseFromNonUniqueCollection(byApproval_, attachmentPtr, attachmentPtr->approvedAndCreated());
    }
    totalSize_ -= attachmentPtr->size();

    return true;
}

void AttachmentCollection::stopBatchInsert()
{
    if ( ! Context::isBatchInsertInProgress()) return;

    byCreated_.clear();
    byCreated_.insert(byId_.begin(), byId_.end());
    
    byName_.clear();
    byName_.insert(byId_.begin(), byId_.end());
    
    bySize_.clear();
    bySize_.insert(byId_.begin(), byId_.end());

    byApproval_.clear();
    byApproval_.insert(byId_.begin(), byId_.end());
}

void AttachmentCollection::prepareUpdateName(AttachmentPtr attachmentPtr)
{
    if (Context::isBatchInsertInProgress()) return;

    byNameUpdateIt_ = findInNonUniqueCollection(byName_, attachmentPtr, attachmentPtr->name());
}

void AttachmentCollection::updateName(AttachmentPtr attachmentPtr)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byNameUpdateIt_ != byName_.end())
    {
        replaceItemInContainer(byName_, byNameUpdateIt_, attachmentPtr);
    }
}

void AttachmentCollection::prepareUpdateApproval(AttachmentPtr attachmentPtr)
{
    if (Context::isBatchInsertInProgress()) return;

    byApprovalUpdateIt_ = findInNonUniqueCollection(byApproval_, attachmentPtr, attachmentPtr->approvedAndCreated());
}

void AttachmentCollection::updateApproval(AttachmentPtr attachmentPtr)
{
    if (Context::isBatchInsertInProgress()) return;

    if (byApprovalUpdateIt_ != byApproval_.end())
    {
        replaceItemInContainer(byApproval_, byApprovalUpdateIt_, attachmentPtr);
    }
}
