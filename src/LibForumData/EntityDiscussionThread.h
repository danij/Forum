#pragma once

#include "Configuration.h"
#include "EntityCommonTypes.h"
#include "EntityDiscussionThreadMessageCollectionBase.h"

#include <atomic>
#include <string>
#include <memory>
#include <set>

namespace Forum
{
    namespace Entities
    {
        struct User;
        struct DiscussionTag;
        struct DiscussionCategory;

        struct DiscussionThread final : public Identifiable, public CreatedMixin, public LastUpdatedMixin, 
                                        public DiscussionThreadMessageCollectionBase, public IndicateDeletionInProgress,
                                        public std::enable_shared_from_this<DiscussionThread>
        {
            const std::string& name()                      const { return name_; }
                  std::string& name()                            { return name_; }
            const User&        createdBy()                 const { return createdBy_; }
                  User&        createdBy()                       { return createdBy_; }

                  auto         tags()                      const { return Helpers::toConst(tags_); }
                  auto&        tagsWeak()                        { return tags_; }

                  auto         categories()                const { return Helpers::toConst(categories_); }
                  auto&        categoriesWeak()                  { return categories_; }

                  auto         nrOfVisitorsSinceLastEdit() const { return visitorsSinceLastEdit_.size(); }

            DiscussionThreadMessage::VoteScoreType voteScore() const
            {
                if (messages_.size())
                {
                    return (*messagesById().begin())->voteScore();
                }
                return 0;
            }
            
            /**
             * Thread-safe reference to the number of times the thread was visited.
             * Can be updated even for const values as it is not refenced in any index.
             * @return An atomic integer of at least 64-bits
             */
            std::atomic_int_fast64_t& visited() const { return visited_; }

            enum ChangeType : uint32_t
            {
                None = 0,
                Name
            };

            explicit DiscussionThread(User& createdBy) : createdBy_(createdBy), visited_(0) {};

            void addVisitorSinceLastEdit(const IdType& userId)
            {
                if (static_cast<int_fast32_t>(visitorsSinceLastEdit_.size()) >=
                    Configuration::getGlobalConfig()->discussionThread.maxUsersInVisitedSinceLastChange)
                {
                    visitorsSinceLastEdit_.clear();
                }
                visitorsSinceLastEdit_.insert(userId.value());
            }

            bool hasVisitedSinceLastEdit(const IdType& userId) const
            {
                return visitorsSinceLastEdit_.find(userId.value()) != visitorsSinceLastEdit_.end();
            }

            void resetVisitorsSinceLastEdit()
            {
                visitorsSinceLastEdit_.clear();
            }

            bool addTag(std::weak_ptr<DiscussionTag> tag)
            {
                return std::get<1>(tags_.insert(std::move(tag)));
            }

            bool removeTag(const std::weak_ptr<DiscussionTag>& tag)
            {
                return tags_.erase(tag) > 0;
            }

            bool addCategory(std::weak_ptr<DiscussionCategory> category)
            {
                return std::get<1>(categories_.insert(std::move(category)));
            }

            bool removeCategory(const std::weak_ptr<DiscussionCategory>& category)
            {
                return categories_.erase(category) > 0;
            }

            Timestamp latestMessageCreated() const
            {
                auto index = messagesByCreated();
                if ( ! index.size())
                {
                    return 0;
                }
                return (*index.rbegin())->created();
            }

        private:
            std::string name_;
            User& createdBy_;
            mutable std::atomic_int_fast64_t visited_;
            std::set<boost::uuids::uuid> visitorsSinceLastEdit_;
            std::set<std::weak_ptr<DiscussionTag>, std::owner_less<std::weak_ptr<DiscussionTag>>> tags_;
            std::set<std::weak_ptr<DiscussionCategory>, std::owner_less<std::weak_ptr<DiscussionCategory>>> categories_;
        };

        typedef std::shared_ptr<DiscussionThread> DiscussionThreadRef;
    }
}
