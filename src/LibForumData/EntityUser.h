#pragma once

#include "EntityDiscussionThreadCollectionBase.h"
#include "EntityDiscussionThreadMessageCollectionBase.h"
#include "EntityCommonTypes.h"

#include <string>
#include <memory>
#include <set>

namespace Forum
{
    namespace Entities
    {
        struct User final : public Identifiable, public CreatedMixin,
                            public DiscussionThreadCollectionBase, public DiscussionThreadMessageCollectionBase
        {
            const std::string& name()     const { return name_; }
                  std::string& name()           { return name_; }
            const Timestamp&   lastSeen() const { return lastSeen_; }
                  Timestamp&   lastSeen()       { return lastSeen_; }
            auto&              votedMessages()  { return votedMessages_; }

            enum ChangeType : uint32_t
            {
                None = 0,
                Name
            };

            User() : lastSeen_(0) {}
            /**
             * Only used to construct the anonymous user
             */
            explicit User(const std::string& name) : name_(name), lastSeen_(0) {}

            void registerVote(const DiscussionThreadMessageRef& message)
            {
                votedMessages_.insert(DiscussionThreadMessageWeakRef(message));
            }

        private:
            std::string name_;
            Timestamp lastSeen_;
            std::set<DiscussionThreadMessageWeakRef, std::owner_less<DiscussionThreadMessageWeakRef>> votedMessages_;

        };

        typedef std::shared_ptr<User> UserRef;
        typedef std::  weak_ptr<User> UserWeakRef;
    }
}
