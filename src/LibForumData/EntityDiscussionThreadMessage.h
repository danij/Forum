#pragma once

#include "EntityCommonTypes.h"
#include "EntityMessageCommentCollectionBase.h"

#include <boost/noncopyable.hpp>

#include <memory>
#include <string>
#include <map>

namespace Forum
{
    namespace Entities
    {
        struct User;
        struct DiscussionThread;
        
        /**
         * Stores a message part of a discussion thread
         * Repositories are responsible for updating the relationships between this message and other entities
         * When cloning a message, the repository needs to reintroduce it in all collections it was part of
         */
        struct DiscussionThreadMessage final : public Identifiable, public CreatedMixin, public LastUpdatedMixin<User>,
                                               public MessageCommentCollectionBase
        {
            typedef int_fast32_t VoteScoreType;

            const std::string&        content()            const { return content_; }
                  std::string&        content()                  { return content_; }
            const User&               createdBy()          const { return createdBy_; }
                  User&               createdBy()                { return createdBy_; }
            const DiscussionThread&   parentThread()       const { return parentThread_; }
                  DiscussionThread&   parentThread()             { return parentThread_; }

            auto                      upVotes()            const { return Helpers::toConst(upVotes_); }
            auto                      downVotes()          const { return Helpers::toConst(downVotes_); }
            VoteScoreType             voteScore()          const 
                { return static_cast<VoteScoreType>(upVotes_.size()) - static_cast<VoteScoreType>(downVotes_.size()); }

            enum ChangeType : uint32_t
            {
                None = 0,
                Content
            };

            DiscussionThreadMessage(User& createdBy, DiscussionThread& parentThread)
                : createdBy_(createdBy), parentThread_(parentThread) {}

            DiscussionThreadMessage(DiscussionThreadMessage& other, DiscussionThread& newParent)
                : content_(std::move(other.content_)), createdBy_(other.createdBy_), parentThread_(newParent)
            {
                id() = other.id();
                created() = other.created();
                creationDetails() = other.creationDetails();
                lastUpdated() = other.lastUpdated();
                lastUpdatedDetails() = other.lastUpdatedDetails();
                lastUpdatedBy() = other.lastUpdatedBy();
                lastUpdatedReason() = other.lastUpdatedReason();
            }

            bool hasVoted(const std::weak_ptr<User>& user) const
            {
                return (upVotes_.find(user) != upVotes_.end()) || (downVotes_.find(user) != downVotes_.end());
            }

            void addUpVote(std::weak_ptr<User> user, const Timestamp& at)
            {
                upVotes_.insert(std::make_pair(std::move(user), at));
            }

            void addDownVote(std::weak_ptr<User> user, const Timestamp& at)
            {
                downVotes_.insert(std::make_pair(std::move(user), at));
            }

            /**
             * Removes the vote of a user
             * @return TRUE if there was an up or down vote from the user
             */
            bool removeVote(std::weak_ptr<User> user)
            {
                return upVotes_.erase(user) > 0 || downVotes_.erase(user) > 0;
            }

        private:
            std::string content_;
            User& createdBy_;
            DiscussionThread& parentThread_;
            //using maps as they use less memory than unordered_maps
            //number of votes/message will usually be small
            std::map<std::weak_ptr<User>, Timestamp, std::owner_less<std::weak_ptr<User>>> upVotes_;
            std::map<std::weak_ptr<User>, Timestamp, std::owner_less<std::weak_ptr<User>>> downVotes_;
        };

        typedef std::shared_ptr<DiscussionThreadMessage> DiscussionThreadMessageRef;
        typedef std::  weak_ptr<DiscussionThreadMessage> DiscussionThreadMessageWeakRef;
    }
}
