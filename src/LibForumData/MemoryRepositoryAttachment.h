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

#include "MemoryRepositoryCommon.h"
#include "Authorization.h"

namespace Forum::Repository
{
    class MemoryRepositoryAttachment final : public MemoryRepositoryBase,
                                             public IAttachmentRepository, public IAttachmentDirectWriteRepository
    {
    public:
        explicit MemoryRepositoryAttachment(MemoryStoreRef store, 
                                            Authorization::AttachmentAuthorizationRef authorization);

        StatusCode getAttachments(RetrieveAttachmentsBy by, OutStream& output) const override;
        StatusCode getAttachmentsOfUser(Entities::IdTypeRef id, RetrieveAttachmentsBy by, 
                                        OutStream& output) const override;
        StatusCode canGetAttachment(Entities::IdTypeRef id, OutStream& output) const override;

        StatusCode addNewAttachment(StringView name, uint64_t size, OutStream& output) override;
        StatusWithResource<Entities::AttachmentPtr> addNewAttachment(Entities::EntityCollection& collection,
                                                                     Entities::IdTypeRef id, StringView name, 
                                                                     uint64_t size, bool approved) override;
        StatusCode changeAttachmentName(Entities::IdTypeRef id, StringView newName,  OutStream& output) override;
        StatusCode changeAttachmentName(Entities::EntityCollection& collection, Entities::IdTypeRef id, 
                                        StringView newName) override;
        StatusCode changeAttachmentApproval(Entities::IdTypeRef id, bool newApproval, OutStream& output) override;
        StatusCode changeAttachmentApproval(Entities::EntityCollection& collection, Entities::IdTypeRef id, 
                                            bool newApproval) override;
        StatusCode deleteAttachment(Entities::IdTypeRef id, OutStream& output) override;
        StatusCode deleteAttachment(Entities::EntityCollection& collection, Entities::IdTypeRef id) override;

        StatusCode addAttachmentToDiscussionThreadMessage(Entities::IdTypeRef attachmentId, 
                                                          Entities::IdTypeRef messageId, OutStream& output) override;
        StatusWithResource<Entities::AttachmentPtr> addAttachmentToDiscussionThreadMessage(
                Entities::EntityCollection& collection, Entities::IdTypeRef attachmentId, 
                Entities::IdTypeRef messageId) override;
        StatusCode removeAttachmentFromDiscussionThreadMessage(Entities::IdTypeRef attachmentId, 
                                                               Entities::IdTypeRef messageId, OutStream& output) override;
        StatusCode removeAttachmentFromDiscussionThreadMessage(Entities::EntityCollection& collection,
                                                               Entities::IdTypeRef attachmentId, 
                                                               Entities::IdTypeRef messageId) override;
    private:
        Authorization::AttachmentAuthorizationRef authorization_;
    };
}
