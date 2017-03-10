#pragma once

#include "MemoryRepositoryCommon.h"

#include <boost/regex/icu.hpp>

namespace Forum
{
    namespace Repository
    {
        class MemoryRepositoryDiscussionThreadMessage final : public MemoryRepositoryBase, 
                                                              public IDiscussionThreadMessageRepository
        {
        public:
            explicit MemoryRepositoryDiscussionThreadMessage(MemoryStoreRef store);

            StatusCode addNewDiscussionMessageInThread(const Entities::IdType& threadId,
                                                       const std::string& content, std::ostream& output) override;
            StatusCode deleteDiscussionMessage(const Entities::IdType& id, std::ostream& output) override;
            StatusCode changeDiscussionThreadMessageContent(const Entities::IdType& id, const std::string& newContent,
                                                            const std::string& changeReason, 
                                                            std::ostream& output) override;
            StatusCode moveDiscussionThreadMessage(const Entities::IdType& messageId, const Entities::IdType& intoThreadId, 
                                                   std::ostream& output) override;
            StatusCode upVoteDiscussionThreadMessage(const Entities::IdType& id, std::ostream& output) override;
            StatusCode downVoteDiscussionThreadMessage(const Entities::IdType& id, std::ostream& output) override;
            StatusCode resetVoteDiscussionThreadMessage(const Entities::IdType& id, std::ostream& output) override;

            StatusCode getDiscussionThreadMessagesOfUserByCreated(const Entities::IdType& id,
                                                                  std::ostream& output) const override;

            StatusCode addCommentToDiscussionThreadMessage(const Entities::IdType& messageId, const std::string& content, 
                                                           std::ostream& output) override;
            StatusCode getMessageComments(std::ostream& output) const override;
            StatusCode getMessageCommentsOfDiscussionThreadMessage(const Entities::IdType& id, 
                                                                   std::ostream& output) const override;
            StatusCode getMessageCommentsOfUser(const Entities::IdType& id, std::ostream& output) const override;
            StatusCode setMessageCommentToSolved(const Entities::IdType& id, std::ostream& output) override;

        private:
            StatusCode voteDiscussionThreadMessage(const Entities::IdType& id, std::ostream& output, bool up);

            boost::u32regex validDiscussionMessageContentRegex;
            boost::u32regex validDiscussionMessageCommentRegex;
            boost::u32regex validDiscussionMessageChangeReasonRegex;
        };
    }
}
