#pragma once

#include "MemoryRepositoryCommon.h"
#include "Authorization.h"

namespace Forum
{
    namespace Repository
    {
        class MemoryRepositoryDiscussionThreadMessage final : public MemoryRepositoryBase, 
                                                              public IDiscussionThreadMessageRepository
        {
        public:
            explicit MemoryRepositoryDiscussionThreadMessage(MemoryStoreRef store,
                                                             Authorization::DiscussionThreadMessageAuthorizationRef authorization);

            StatusCode addNewDiscussionMessageInThread(const Entities::IdType& threadId,
                                                       const StringView& content, OutStream& output) override;
            StatusCode deleteDiscussionMessage(const Entities::IdType& id, OutStream& output) override;
            StatusCode changeDiscussionThreadMessageContent(const Entities::IdType& id, const StringView& newContent,
                                                            const StringView& changeReason, 
                                                            OutStream& output) override;
            StatusCode moveDiscussionThreadMessage(const Entities::IdType& messageId, const Entities::IdType& intoThreadId, 
                                                   OutStream& output) override;
            StatusCode upVoteDiscussionThreadMessage(const Entities::IdType& id, OutStream& output) override;
            StatusCode downVoteDiscussionThreadMessage(const Entities::IdType& id, OutStream& output) override;
            StatusCode resetVoteDiscussionThreadMessage(const Entities::IdType& id, OutStream& output) override;

            StatusCode getDiscussionThreadMessagesOfUserByCreated(const Entities::IdType& id,
                                                                  OutStream& output) const override;

            StatusCode addCommentToDiscussionThreadMessage(const Entities::IdType& messageId, const StringView& content, 
                                                           OutStream& output) override;
            StatusCode getMessageComments(OutStream& output) const override;
            StatusCode getMessageCommentsOfDiscussionThreadMessage(const Entities::IdType& id, 
                                                                   OutStream& output) const override;
            StatusCode getMessageCommentsOfUser(const Entities::IdType& id, OutStream& output) const override;
            StatusCode setMessageCommentToSolved(const Entities::IdType& id, OutStream& output) override;

        private:
            StatusCode voteDiscussionThreadMessage(const Entities::IdType& id, OutStream& output, bool up);

            Authorization::DiscussionThreadMessageAuthorizationRef authorization_;
        };
    }
}
