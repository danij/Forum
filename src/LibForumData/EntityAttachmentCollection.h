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

#include "ConstCollectionAdapter.h"
#include "EntityAttachment.h"

#include <boost/noncopyable.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ranked_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

namespace Forum::Entities
{
    class AttachmentCollection final : boost::noncopyable
    {
    public:
        bool add(AttachmentPtr attachmentPtr);
        bool remove(AttachmentPtr attachmentPtr);

        void stopBatchInsert();

        void prepareUpdateName(AttachmentPtr attachmentPtr);
        void updateName(AttachmentPtr attachmentPtr);

        void prepareUpdateApproval(AttachmentPtr attachmentPtr);
        void updateApproval(AttachmentPtr attachmentPtr);

        auto count()       const { return byId_.size(); }
        auto totalSize()   const { return totalSize_; }

        auto  byId()       const { return Helpers::toConst(byId_); }
        auto& byId()             { return byId_; }

        auto  byCreated()  const { return Helpers::toConst(byCreated_); }
        auto& byCreated()        { return byCreated_; }

        auto  byName()     const { return Helpers::toConst(byName_); }
        auto& byName()           { return byName_; }

        auto  bySize()     const { return Helpers::toConst(bySize_); }
        auto& bySize()           { return bySize_; }

        auto  byApproval() const { return Helpers::toConst(byApproval_); }
        auto& byApproval()       { return byApproval_; }

    private:
        HASHED_UNIQUE_COLLECTION(Attachment, id) byId_;

        RANKED_COLLECTION(Attachment, created) byCreated_;

        RANKED_COLLECTION(Attachment, name) byName_;
        RANKED_COLLECTION_ITERATOR(byName_) byNameUpdateIt_;

        RANKED_COLLECTION(Attachment, size) bySize_;

        RANKED_COLLECTION(Attachment, approvedAndCreated) byApproval_;
        RANKED_COLLECTION_ITERATOR(byApproval_) byApprovalUpdateIt_;

        uint64_t totalSize_{ 0 };
    };
}
