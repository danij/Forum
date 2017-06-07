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

            StatusCode addNewDiscussionMessageInThread(Entities::IdTypeRef threadId,
                                                       StringView content, OutStream& output) override;
            StatusCode deleteDiscussionMessage(Entities::IdTypeRef id, OutStream& output) override;
            StatusCode changeDiscussionThreadMessageContent(Entities::IdTypeRef id, StringView newContent,
                                                            StringView changeReason, 
                                                            OutStream& output) override;
            StatusCode moveDiscussionThreadMessage(Entities::IdTypeRef messageId, Entities::IdTypeRef intoThreadId, 
                                                   OutStream& output) override;
            StatusCode upVoteDiscussionThreadMessage(Entities::IdTypeRef id, OutStream& output) override;
            StatusCode downVoteDiscussionThreadMessage(Entities::IdTypeRef id, OutStream& output) override;
            StatusCode resetVoteDiscussionThreadMessage(Entities::IdTypeRef id, OutStream& output) override;

            StatusCode getDiscussionThreadMessagesOfUserByCreated(Entities::IdTypeRef id,
                                                                  OutStream& output) const override;

            StatusCode addCommentToDiscussionThreadMessage(Entities::IdTypeRef messageId, StringView content, 
                                                           OutStream& output) override;
            StatusCode getMessageComments(OutStream& output) const override;
            StatusCode getMessageCommentsOfDiscussionThreadMessage(Entities::IdTypeRef id, 
                                                                   OutStream& output) const override;
            StatusCode getMessageCommentsOfUser(Entities::IdTypeRef id, OutStream& output) const override;
            StatusCode setMessageCommentToSolved(Entities::IdTypeRef id, OutStream& output) override;

        private:
            StatusCode voteDiscussionThreadMessage(Entities::IdTypeRef id, OutStream& output, bool up);

            Authorization::DiscussionThreadMessageAuthorizationRef authorization_;
        };
    }
}
