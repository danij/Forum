#pragma once

#include "AuthorizationPrivileges.h"
#include "EntityCommonTypes.h"
#include "EntityMessageCommentCollection.h"
#include "StringHelpers.h"

#include <string>
#include <map>

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

namespace Forum
{
    namespace Entities
    {
        class User;
        class DiscussionThread;

        /**
         * Stores a message part of a discussion thread
         * Repositories are responsible for updating the relationships between this message and other entities
         * When cloning a message, the repository needs to reintroduce it in all collections it was part of
         */
        class DiscussionThreadMessage final : public Authorization::DiscussionThreadMessagePrivilegeStore,
                                              private boost::noncopyable
        {
        public:
            typedef int_fast32_t VoteScoreType;
            //using maps as they use less memory than unordered_maps
            //number of votes/message will usually be small
            typedef std::map<EntityPointer<User>, Timestamp> VoteCollection;

            const auto& id()                  const { return id_; }

                   auto created()             const { return created_; }
            const auto& creationDetails()     const { return creationDetails_; }

            const auto& createdBy()           const { return createdBy_; }
                   auto parentThread()        const { return parentThread_.toConst(); }

             StringView content()             const { return content_; }

            const auto& comments() const
            {
                static const MessageCommentCollection emptyMessageCommentCollection;

                return comments_ ? *comments_ : emptyMessageCommentCollection;
            }
            auto solvedCommentsCount()        const { return solvedCommentsCount_; }

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

            boost::optional<Timestamp> votedAt(EntityPointer<User> user) const
            {
                if (upVotes_)
                {
                    auto it = upVotes_->find(user);
                    if (it != upVotes_->end())
                    {
                        return it->second;
                    }
                }
                if (downVotes_)
                {
                    auto it = downVotes_->find(user);
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
                Content
            };

            DiscussionThreadMessage(IdType id, User& createdBy, Timestamp created, VisitDetails creationDetails)
                : id_(std::move(id)), created_(created), creationDetails_(std::move(creationDetails)),
                  createdBy_(createdBy)
            {}

            auto& createdBy()           { return createdBy_; }
            auto& parentThread()        { return parentThread_; }

            auto& content()             { return content_; }

            auto* comments()            { return comments_.get(); }
            void  addComment(MessageCommentPtr comment)
            {
                if ( ! comments_) comments_.reset(new MessageCommentCollection);
                comments_->add(comment);
            }
            void  removeComment(MessageCommentPtr comment)
            {
                if ( ! comments_) return;
                comments_->remove(comment);
            }

            auto& solvedCommentsCount() { return solvedCommentsCount_; }

            void updateLastUpdated(Timestamp at)
            {
                if ( ! lastUpdated_) lastUpdated_.reset(new LastUpdatedInfo());
                lastUpdated_->at = at;
            }

            void updateLastUpdatedDetails(VisitDetails&& details)
            {
                if ( ! lastUpdated_) lastUpdated_.reset(new LastUpdatedInfo());
                lastUpdated_->details = std::move(details);
            }

            void updateLastUpdatedReason(std::string&& reason)
            {
                if ( ! lastUpdated_) lastUpdated_.reset(new LastUpdatedInfo());
                lastUpdated_->reason = std::move(reason);
            }

            void updateLastUpdatedBy(EntityPointer<User> by)
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
            RemoveVoteStatus removeVote(EntityPointer<User> user)
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

            Helpers::WholeChangeableString content_;

            std::unique_ptr<LastUpdatedInfo> lastUpdated_;

            std::unique_ptr<MessageCommentCollection> comments_;
            int32_t solvedCommentsCount_{0};

            std::unique_ptr<VoteCollection> upVotes_;
            std::unique_ptr<VoteCollection> downVotes_;
        };

        typedef EntityPointer<DiscussionThreadMessage> DiscussionThreadMessagePtr;
        typedef EntityPointer<const DiscussionThreadMessage> DiscussionThreadMessageConstPtr;
    }
}
