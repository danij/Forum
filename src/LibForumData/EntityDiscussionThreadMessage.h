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

#include "AuthorizationPrivileges.h"
#include "EntityCommonTypes.h"
#include "EntityMessageCommentCollection.h"
#include "EntityAttachment.h"
#include "StringHelpers.h"

#include <string>

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

namespace Forum::Entities
{
    class User;
    class DiscussionThread;

    /**
     * Stores a message part of a discussion thread
     * Repositories are responsible for updating the relationships between this message and other entities
     * When cloning a message, the repository needs to reintroduce it in all collections it was part of
     */
    class DiscussionThreadMessage final : public Authorization::DiscussionThreadMessagePrivilegeStore,
                                          boost::noncopyable
    {
    public:
        typedef int_fast32_t VoteScoreType;
        //using flat maps/sets as they use less memory than tree/hash based maps/sets
        //number of votes/message will usually be small
        typedef boost::container::flat_map<EntityPointer<User>, Timestamp> VoteCollection;
        //number of attachments will usually be small
        typedef boost::container::flat_set<AttachmentPtr> AttachmentCollection;

        const auto& id()                  const { return id_; }

               auto created()             const { return created_; }
        const auto& creationDetails()     const { return creationDetails_; }

        const auto& createdBy()           const { return createdBy_; }
               auto parentThread()        const { return parentThread_.toConst(); }

         StringView content()             const { return content_; }

        const auto& comments() const
        {
            static const MessageCommentCollectionLowMemory emptyMessageCommentCollection;

            return comments_ ? *comments_ : emptyMessageCommentCollection;
        }
        auto attachments() const
        {
            static const AttachmentCollection emptyAttachmentCollection;

            return Helpers::toConst(attachments_ ? *attachments_ : emptyAttachmentCollection);
        }
        auto solvedCommentsCount()        const { return solvedCommentsCount_; }
        auto approved()                   const { return 0 != approved_; }

        auto lastUpdated() const
        {
            return lastUpdated_ ? lastUpdated_->at : Timestamp{ 0 };
        }

        const auto& lastUpdatedDetails() const
        {
            static const VisitDetails lastUpdatedDetailsDefault{};
            return lastUpdated_ ? lastUpdated_->details : lastUpdatedDetailsDefault;
        }

        StringView lastUpdatedReason() const
        {
            return lastUpdated_ ? lastUpdated_->reason : StringView{};
        }

        auto lastUpdatedBy() const
        {
            return lastUpdated_ ? lastUpdated_->by.toConst() : EntityPointer<const User>{};
        }


        bool hasVoted(EntityPointer<User> user) const
        {
            return (upVotes_ && (upVotes_->find(user) != upVotes_->end()))
                || (downVotes_ && (downVotes_->find(user) != downVotes_->end()));
        }

        auto upVotes() const
        {
            static const VoteCollection emptyVoteCollection;
            return Helpers::toConst(upVotes_ ? *upVotes_ : emptyVoteCollection);
        }

        auto downVotes() const
        {
            static const VoteCollection emptyVoteCollection;
            return Helpers::toConst(downVotes_ ? *downVotes_ : emptyVoteCollection);
        }

        boost::optional<Timestamp> votedAt(const EntityPointer<User> user) const
        {
            if (upVotes_)
            {
                const auto it = upVotes_->find(user);
                if (it != upVotes_->end())
                {
                    return it->second;
                }
            }
            if (downVotes_)
            {
                const auto it = downVotes_->find(user);
                if (it != downVotes_->end())
                {
                    return it->second;
                }
            }
            return{};
        }

        auto voteScore() const
        {
            VoteScoreType upVoteCount = 0, downVoteCount = 0;
            if (upVotes_) upVoteCount = static_cast<VoteScoreType>(upVotes_->size());
            if (downVotes_) downVoteCount = static_cast<VoteScoreType>(downVotes_->size());

            return upVoteCount - downVoteCount;
        }

        Authorization::PrivilegeValueType getDiscussionThreadMessagePrivilege(
                Authorization::DiscussionThreadMessagePrivilege privilege) const override;

        //optimization should the discussionThreadLevelValue already be available
        Authorization::PrivilegeValueType getDiscussionThreadMessagePrivilege(
                Authorization::DiscussionThreadMessagePrivilege privilege,
                Authorization::PrivilegeValueType discussionThreadLevelValue) const;

        enum ChangeType : uint32_t
        {
            None = 0,
            Content,
            Approval
        };

        DiscussionThreadMessage(const IdType id, User& createdBy, const Timestamp created, 
                                const VisitDetails creationDetails, const bool approved)
            : id_(id), created_(created), creationDetails_(creationDetails), createdBy_(createdBy)
        {
            solvedCommentsCount_ = 0;
            approved_ = approved ? 1 : 0;
        }

        auto& createdBy()           { return createdBy_; }
        auto& parentThread()        { return parentThread_; }

        auto& content()             { return content_; }

        auto* comments()            { return comments_.get(); }
        void  addComment(const MessageCommentPtr comment)
        {
            if ( ! comments_) comments_.reset(new MessageCommentCollectionLowMemory);
            comments_->add(comment);
        }
        void  removeComment(const MessageCommentPtr comment)
        {
            if ( ! comments_) return;
            comments_->remove(comment);
        }
        auto& attachments()
        {
            static const AttachmentCollection emptyAttachmentCollection;

            return attachments_ ? *attachments_ : emptyAttachmentCollection;
        }
        void  addAttachment(const AttachmentPtr attachmentPtr)
        {
            if ( ! attachments_) attachments_.reset(new AttachmentCollection);
            attachments_->insert(attachmentPtr);
        }
        void  removeAttachment(const AttachmentPtr attachmentPtr)
        {
            if ( ! attachments_) return;
            attachments_->erase(attachmentPtr);
        }

        void incrementSolvedCommentsCount() { solvedCommentsCount_ += 1; }
        void decrementSolvedCommentsCount() { solvedCommentsCount_ -= 1; }

        void approve() { approved_ = 1; }
        void unapprove() { approved_ = 0; }

        void updateLastUpdated(const Timestamp at)
        {
            if ( ! lastUpdated_) lastUpdated_.reset(new LastUpdatedInfo());
            lastUpdated_->at = at;
        }

        void updateLastUpdatedDetails(VisitDetails&& details)
        {
            if ( ! lastUpdated_) lastUpdated_.reset(new LastUpdatedInfo());
            lastUpdated_->details = details;
        }

        void updateLastUpdatedReason(std::string&& reason)
        {
            if ( ! lastUpdated_) lastUpdated_.reset(new LastUpdatedInfo());
            lastUpdated_->reason = std::move(reason);
        }

        void updateLastUpdatedBy(const EntityPointer<User> by)
        {
            if ( ! lastUpdated_) lastUpdated_.reset(new LastUpdatedInfo());
            lastUpdated_->by = by;
        }

        auto& upVotes() { return upVotes_; }

        auto& downVotes() { return downVotes_; }

        void addUpVote(EntityPointer<User> user, const Timestamp& at)
        {
            if ( ! upVotes_) upVotes_.reset(new VoteCollection);
            upVotes_->insert(std::make_pair(user, at));
        }

        void addDownVote(EntityPointer<User> user, const Timestamp& at)
        {
            if ( ! downVotes_) downVotes_.reset(new VoteCollection);
            downVotes_->insert(std::make_pair(user, at));
        }

        enum class RemoveVoteStatus
        {
            Missing,
            WasUpVote,
            WasDownVote
        };

        /**
         * Removes the vote of a user
         * @return TRUE if there was an up or down vote from the user
         */
        RemoveVoteStatus removeVote(const EntityPointer<User> user)
        {
            if (upVotes_ && (upVotes_->erase(user) > 0))
            {
                return RemoveVoteStatus::WasUpVote;
            }
            if (downVotes_ && (downVotes_->erase(user) > 0))
            {
                return RemoveVoteStatus::WasDownVote;
            }
            return RemoveVoteStatus::Missing;
        }

    private:
        IdType id_;
        Timestamp created_{0};
        VisitDetails creationDetails_;

        User& createdBy_;
        EntityPointer<DiscussionThread> parentThread_;
        uint16_t solvedCommentsCount_ : 15;
        uint16_t approved_ : 1;

        Helpers::WholeChangeableString content_;

        std::unique_ptr<LastUpdatedInfo> lastUpdated_;

        std::unique_ptr<MessageCommentCollectionLowMemory> comments_;

        std::unique_ptr<VoteCollection> upVotes_;
        std::unique_ptr<VoteCollection> downVotes_;

        std::unique_ptr<AttachmentCollection> attachments_;
    };

    typedef EntityPointer<DiscussionThreadMessage> DiscussionThreadMessagePtr;
    typedef EntityPointer<const DiscussionThreadMessage> DiscussionThreadMessageConstPtr;
}
