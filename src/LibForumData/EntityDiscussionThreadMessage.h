#pragma once

#include "AuthorizationPrivileges.h"
#include "EntityCommonTypes.h"
#include "EntityMessageCommentCollection.h"
#include "StringHelpers.h"

#include <string>
#include <map>

#include <boost/noncopyable.hpp>

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

                   auto lastUpdated()         const { return lastUpdated_; }
            const auto& lastUpdatedDetails()  const { return lastUpdatedDetails_; }
             StringView lastUpdatedReason()   const { return lastUpdatedReason_; }
                   auto lastUpdatedBy()       const { return lastUpdatedBy_.toConst(); }
            
            const auto& comments()            const { return comments_; }
                   auto solvedCommentsCount() const { return solvedCommentsCount_; }

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

            auto voteScore() const
            {
                VoteScoreType upVoteCount = 0, downVoteCount = 0;
                if (upVotes_) upVoteCount = static_cast<VoteScoreType>(upVotes_->size());
                if (downVotes_) downVoteCount = static_cast<VoteScoreType>(downVotes_->size());

                return upVoteCount - downVoteCount;
            } 

            Authorization::PrivilegeValueType getDiscussionThreadMessagePrivilege(
                    Authorization::DiscussionThreadMessagePrivilege privilege) const override;
            
            Authorization::PrivilegeValueType getDiscussionThreadMessagePrivilege(
                    Authorization::DiscussionThreadMessagePrivilege privilege,
                    Authorization::PrivilegeValueType discussionThreadLevelValue) const
            {
                auto result = DiscussionThreadMessagePrivilegeStore::getDiscussionThreadMessagePrivilege(privilege);
                return Authorization::minimumPrivilegeValue(result, discussionThreadLevelValue);
            }

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
                                                            
            auto& lastUpdated()         { return lastUpdated_; }
            auto& lastUpdatedDetails()  { return lastUpdatedDetails_; }
            auto& lastUpdatedReason()   { return lastUpdatedReason_; }
            auto& lastUpdatedBy()       { return lastUpdatedBy_; }

            auto& content()             { return content_; }
            
            auto& comments()            { return comments_; }
            auto& solvedCommentsCount() { return solvedCommentsCount_; }

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

            /**
             * Removes the vote of a user
             * @return TRUE if there was an up or down vote from the user
             */
            bool removeVote(EntityPointer<User> user)
            {
                return (upVotes_ && (upVotes_->erase(user) > 0)) || (downVotes_ && (downVotes_->erase(user) > 0));
            }

        private:
            IdType id_;
            Timestamp created_ = 0;
            VisitDetails creationDetails_;

            User& createdBy_;
            EntityPointer<DiscussionThread> parentThread_;

            Helpers::ImmutableString content_;

            Timestamp lastUpdated_ = 0;
            VisitDetails lastUpdatedDetails_;
            std::string lastUpdatedReason_;
            EntityPointer<User> lastUpdatedBy_;
            
            MessageCommentCollection comments_;
            int32_t solvedCommentsCount_ = 0;

            std::unique_ptr<VoteCollection> upVotes_;
            std::unique_ptr<VoteCollection> downVotes_;
        };

        typedef EntityPointer<DiscussionThreadMessage> DiscussionThreadMessagePtr;
        typedef EntityPointer<const DiscussionThreadMessage> DiscussionThreadMessageConstPtr;
    }
}
