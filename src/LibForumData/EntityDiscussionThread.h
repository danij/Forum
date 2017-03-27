#pragma once

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

        /**
        * Stores a discussion thread that contains messages
        * Repositories are responsible for updating the relationships between this message and other entities
        */
        struct DiscussionThread final : public Identifiable, 
                                        public CreatedMixin, 
                                        public LastUpdatedMixin<User>,
                                        public DiscussionThreadMessageCollectionBase<OrderedIndexForId>,
                                        public IndicateDeletionInProgress,
                                        public std::enable_shared_from_this<DiscussionThread>
        {
            const std::string& name()                      const { return name_; }
                  std::string& name()                            { return name_; }
            const User&        createdBy()                 const { return createdBy_; }
                  User&        createdBy()                       { return createdBy_; }
                  Timestamp    latestVisibleChange()       const { return latestVisibleChange_; }
                  Timestamp&   latestVisibleChange()             { return latestVisibleChange_; }

                  auto         tags()                      const { return Helpers::toConst(tags_); }
                  auto&        tagsWeak()                        { return tags_; }

                  auto         categories()                const { return Helpers::toConst(categories_); }
                  auto&        categoriesWeak()                  { return categories_; }

                  auto         nrOfVisitorsSinceLastEdit() const { return visitorsSinceLastEdit_.size(); }

                  auto&        subscribedUsers()                 { return subscribedUsers_; }
                  auto         subscribedUsersCount()      const { return subscribedUsers_.size(); }

            DiscussionThreadMessage::VoteScoreType voteScore() const;

            Timestamp latestMessageCreated() const;

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

            explicit DiscussionThread(User& createdBy) : createdBy_(createdBy), latestVisibleChange_(0), visited_(0) {};

            void addVisitorSinceLastEdit(const IdType& userId);
            bool hasVisitedSinceLastEdit(const IdType& userId) const;
            void resetVisitorsSinceLastEdit();

            bool addTag(std::weak_ptr<DiscussionTag> tag);
            bool removeTag(const std::weak_ptr<DiscussionTag>& tag);

            bool addCategory(std::weak_ptr<DiscussionCategory> category);
            bool removeCategory(const std::weak_ptr<DiscussionCategory>& category);

        private:
            std::string name_;
            User& createdBy_;
            //store the timestamp of the latest visibile change in order to be able to 
            //detect when to return a status that nothing has changed since a provided timestamp
            //Note: do not use as index in collection, the indexes would not always be updated
            Timestamp latestVisibleChange_;
            mutable std::atomic_int_fast64_t visited_;
            std::set<boost::uuids::uuid> visitorsSinceLastEdit_;
            std::set<std::weak_ptr<DiscussionTag>, std::owner_less<std::weak_ptr<DiscussionTag>>> tags_;
            std::set<std::weak_ptr<DiscussionCategory>, std::owner_less<std::weak_ptr<DiscussionCategory>>> categories_;
            std::set<std::weak_ptr<User>, std::owner_less<std::weak_ptr<User>>> subscribedUsers_;
        };

        typedef std::shared_ptr<DiscussionThread> DiscussionThreadRef;
    }
}
