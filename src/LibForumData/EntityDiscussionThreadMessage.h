#pragma once

#include "AuthorizationPrivileges.h"
#include "EntityCommonTypes.h"
#include "EntityMessageCommentCollectionBase.h"
#include "StringHelpers.h"

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
        struct DiscussionThreadMessage final : public Identifiable, 
                                               public CreatedMixin, 
                                               public LastUpdatedMixinWithBy<User>,
                                               public MessageCommentCollectionBase<OrderedIndexForId>,
                                               public Authorization::DiscussionThreadMessagePrivilegeStore
        {
            typedef int_fast32_t VoteScoreType;

                  StringView                 content()             const { return content_; }
                  Helpers::ImmutableString&  content()                   { return content_; }
            const User&                      createdBy()           const { return createdBy_; }
                  User&                      createdBy()                 { return createdBy_; }
                                                                   
            auto                             upVotes()             const { return Helpers::toConst(upVotes_); }
            auto                             downVotes()           const { return Helpers::toConst(downVotes_); }
            VoteScoreType                    voteScore()           const 
                { return static_cast<VoteScoreType>(upVotes_.size()) - static_cast<VoteScoreType>(downVotes_.size()); }

            int_fast32_t              solvedCommentsCount() const { return solvedCommentsCount_; }
            int_fast32_t&             solvedCommentsCount()       { return solvedCommentsCount_; }

            std::weak_ptr<DiscussionThread>& parentThread()       { return parentThread_; }

            Authorization::PrivilegeValueType getDiscussionThreadMessagePrivilege(Authorization::DiscussionThreadMessagePrivilege privilege) const override
            {
                Authorization::PrivilegeValueType discussionThreadLevelValue;
                executeActionWithParentThreadIfAvailable([&discussionThreadLevelValue, privilege](auto& parentThread)
                {
                    discussionThreadLevelValue = parentThread.getDiscussionThreadMessagePrivilege(privilege);
                });

                return getDiscussionThreadMessagePrivilege(privilege, discussionThreadLevelValue);
            }

            Authorization::PrivilegeValueType getDiscussionThreadMessagePrivilege(
                    Authorization::DiscussionThreadMessagePrivilege privilege,
                    Authorization::PrivilegeValueType discussionThreadLevelValue) const
            {
                auto result = DiscussionThreadMessagePrivilegeStore::getDiscussionThreadMessagePrivilege(privilege);
                return Authorization::minimumPrivilegeValue(result, discussionThreadLevelValue);
            }

            template<typename TAction>
            void executeActionWithParentThreadIfAvailable(TAction&& action) const
            {
                if (auto parentThreadShared = parentThread_.lock())
                {
                    action(const_cast<const DiscussionThread&>(*parentThreadShared));
                }
            }

            template<typename TAction>
            void executeActionWithParentThreadIfAvailable(TAction&& action)
            {
                if (auto parentMessageThread = parentThread_.lock())
                {
                    action(*parentMessageThread);
                }
            }

            enum ChangeType : uint32_t
            {
                None = 0,
                Content
            };

            explicit DiscussionThreadMessage(User& createdBy) : createdBy_(createdBy), solvedCommentsCount_(0) {}

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
            Helpers::ImmutableString content_;
            User& createdBy_;
            std::weak_ptr<DiscussionThread> parentThread_;
            int_fast32_t solvedCommentsCount_;
            //using maps as they use less memory than unordered_maps
            //number of votes/message will usually be small
            std::map<std::weak_ptr<User>, Timestamp, std::owner_less<std::weak_ptr<User>>> upVotes_;
            std::map<std::weak_ptr<User>, Timestamp, std::owner_less<std::weak_ptr<User>>> downVotes_;
        };

        typedef std::shared_ptr<DiscussionThreadMessage> DiscussionThreadMessageRef;
        typedef std::  weak_ptr<DiscussionThreadMessage> DiscussionThreadMessageWeakRef;
    }
}
