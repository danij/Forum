#pragma once

#include "EntityCommonTypes.h"

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

        struct DiscussionThreadMessage final : public Identifiable, public CreatedMixin, public LastUpdatedMixin, 
                                               private boost::noncopyable
        {
            struct CreationDetails
            {
                IpType ip;
                UserAgentType userAgent;
            };

            struct LastUpdatedDetails : public CreationDetails
            {
                std::weak_ptr<User> by;
            };

            typedef int_fast32_t VoteScoreType;

            const std::string&        content()            const { return content_; }
                  std::string&        content()                  { return content_; }
            const User&               createdBy()          const { return createdBy_; }
                  User&               createdBy()                { return createdBy_; }
            const DiscussionThread&   parentThread()       const { return parentThread_; }
                  DiscussionThread&   parentThread()             { return parentThread_; }
            const CreationDetails&    creationDetails()    const { return creationDetails_; }
                  CreationDetails&    creationDetails()          { return creationDetails_; }
            const LastUpdatedDetails& lastUpdatedDetails() const { return lastUpdatedDetails_; }
                  LastUpdatedDetails& lastUpdatedDetails()       { return lastUpdatedDetails_; }
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

            DiscussionThreadMessage(const DiscussionThreadMessage& other, DiscussionThread& newParent)
                : content_(std::move(other.content_)), createdBy_(other.createdBy_), parentThread_(newParent),
                  creationDetails_(std::move(other.creationDetails_)), 
                  lastUpdatedDetails_(std::move(other.lastUpdatedDetails_))
            {
                id() = other.id();
                created() = other.created();
                lastUpdated() = other.lastUpdated();
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
            LastUpdatedDetails creationDetails_;
            LastUpdatedDetails lastUpdatedDetails_;
            //using maps as they use less memory than unordered_maps
            //number of votes/message will usually be small
            std::map<std::weak_ptr<User>, Timestamp, std::owner_less<std::weak_ptr<User>>> upVotes_;
            std::map<std::weak_ptr<User>, Timestamp, std::owner_less<std::weak_ptr<User>>> downVotes_;
        };

        typedef std::shared_ptr<DiscussionThreadMessage> DiscussionThreadMessageRef;
        typedef std::  weak_ptr<DiscussionThreadMessage> DiscussionThreadMessageWeakRef;
    }
}
