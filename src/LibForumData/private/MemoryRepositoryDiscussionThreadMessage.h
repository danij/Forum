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

namespace Forum
{
    namespace Repository
    {
        class MemoryRepositoryDiscussionThreadMessage final : public MemoryRepositoryBase,
                                                              public IDiscussionThreadMessageRepository,
                                                              public IDiscussionThreadMessageDirectWriteRepository
        {
        public:
            explicit MemoryRepositoryDiscussionThreadMessage(MemoryStoreRef store,
                                                             Authorization::DiscussionThreadMessageAuthorizationRef authorization);

            StatusCode addNewDiscussionMessageInThread(Entities::IdTypeRef threadId,
                                                       StringView content, OutStream& output) override;
            StatusWithResource<Entities::DiscussionThreadMessagePtr>
                addNewDiscussionMessageInThread(Entities::EntityCollection& collection, Entities::IdTypeRef messageId,
                                                Entities::IdTypeRef threadId, StringView content) override;
            StatusWithResource<Entities::DiscussionThreadMessagePtr>
                addNewDiscussionMessageInThread(Entities::EntityCollection& collection, Entities::IdTypeRef messageId,
                                                Entities::IdTypeRef threadId, size_t contentSize,
                                                size_t contentOffset) override;
            StatusCode deleteDiscussionMessage(Entities::IdTypeRef id, OutStream& output) override;
            StatusCode deleteDiscussionMessage(Entities::EntityCollection& collection, Entities::IdTypeRef id) override;
            StatusCode changeDiscussionThreadMessageContent(Entities::IdTypeRef id, StringView newContent,
                                                            StringView changeReason,
                                                            OutStream& output) override;
            StatusCode changeDiscussionThreadMessageContent(Entities::EntityCollection& collection,
                                                            Entities::IdTypeRef id, StringView newContent,
                                                            StringView changeReason) override;
            StatusCode moveDiscussionThreadMessage(Entities::IdTypeRef messageId, Entities::IdTypeRef intoThreadId,
                                                   OutStream& output) override;
            StatusCode moveDiscussionThreadMessage(Entities::EntityCollection& collection,
                                                   Entities::IdTypeRef messageId,
                                                   Entities::IdTypeRef intoThreadId) override;
            StatusCode upVoteDiscussionThreadMessage(Entities::IdTypeRef id, OutStream& output) override;
            StatusCode upVoteDiscussionThreadMessage(Entities::EntityCollection& collection, Entities::IdTypeRef id) override;
            StatusCode downVoteDiscussionThreadMessage(Entities::IdTypeRef id, OutStream& output) override;
            StatusCode downVoteDiscussionThreadMessage(Entities::EntityCollection& collection, Entities::IdTypeRef id) override;
            StatusCode resetVoteDiscussionThreadMessage(Entities::IdTypeRef id, OutStream& output) override;
            StatusCode resetVoteDiscussionThreadMessage(Entities::EntityCollection& collection, Entities::IdTypeRef id) override;

            StatusCode getMultipleDiscussionThreadMessagesById(StringView ids, 
                                                               OutStream& output) const override;
            StatusCode getDiscussionThreadMessagesOfUserByCreated(Entities::IdTypeRef id,
                                                                  OutStream& output) const override;
            StatusCode getLatestDiscussionThreadMessages(OutStream& output) const override;
            StatusCode getDiscussionThreadMessageRank(Entities::IdTypeRef id, OutStream& output) const override;

            StatusCode addCommentToDiscussionThreadMessage(Entities::IdTypeRef messageId, StringView content,
                                                           OutStream& output) override;
            StatusWithResource<Entities::MessageCommentPtr>
                addCommentToDiscussionThreadMessage(Entities::EntityCollection& collection, Entities::IdTypeRef commentId,
                                                    Entities::IdTypeRef messageId, StringView content) override;

            StatusCode getMessageComments(OutStream& output) const override;
            StatusCode getMessageCommentsOfDiscussionThreadMessage(Entities::IdTypeRef id,
                                                                   OutStream& output) const override;
            StatusCode getMessageCommentsOfUser(Entities::IdTypeRef id, OutStream& output) const override;
            StatusCode setMessageCommentToSolved(Entities::IdTypeRef id, OutStream& output) override;
            StatusCode setMessageCommentToSolved(Entities::EntityCollection& collection, Entities::IdTypeRef id) override;

        private:
            StatusWithResource<Entities::DiscussionThreadMessagePtr>
                addNewDiscussionMessageInThread(Entities::EntityCollection& collection, Entities::IdTypeRef messageId,
                                                Entities::IdTypeRef threadId, StringView content,
                                                size_t contentSize, size_t contentOffset);

            StatusCode voteDiscussionThreadMessage(Entities::IdTypeRef id, OutStream& output, bool up);
            StatusCode voteDiscussionThreadMessage(Entities::EntityCollection& collection, Entities::IdTypeRef id, bool up);

            Authorization::DiscussionThreadMessageAuthorizationRef authorization_;
        };
    }
}
