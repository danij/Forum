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
        /**
        * Stores a user that creates content
        * Repositories are responsible for updating the relationships between this message and other entities
        */
        struct User final : public Identifiable,
                            public CreatedMixin,
                            public DiscussionThreadCollectionBase<OrderedIndexForId>,
                            public DiscussionThreadMessageCollectionBase<OrderedIndexForId>,
                            public MessageCommentCollectionBase<OrderedIndexForId>
        {
      const std::string& auth()        const { return auth_; }
            std::string& auth()              { return auth_; }
            StringView   name()        const { return name_; }
            std::string& name()              { return name_; }
            StringView   info()        const { return info_; }
            std::string& info()              { return info_; }
            Timestamp    lastSeen()    const { return lastSeen_; }
            Timestamp&   lastSeen()          { return lastSeen_; }
            auto&        votedMessages()     { return votedMessages_; }

            auto&        subscribedThreads() { return subscribedThreads_; }

            auto subscribedThreadCount()                   const { return subscribedThreads_.threadCount(); }
            auto subscribedThreadsById()                   const { return subscribedThreads_.threadsById(); }
            auto subscribedThreadsByName()                 const { return subscribedThreads_.threadsByName(); }
            auto subscribedThreadsByCreated()              const { return subscribedThreads_.threadsByCreated(); }
            auto subscribedThreadsByLastUpdated()          const { return subscribedThreads_.threadsByLastUpdated(); }
            auto subscribedThreadsByLatestMessageCreated() const { return subscribedThreads_.threadsByLatestMessageCreated(); }
            auto subscribedThreadsByMessageCount()         const { return subscribedThreads_.threadsByMessageCount(); }
            auto subscribedThreadsByPinDisplayOrder()      const { return subscribedThreads_.threadsByPinDisplayOrder(); }

            enum ChangeType : uint32_t
            {
                None = 0,
                Name,
                Info
            };

            User() : lastSeen_(0) {}
            /**
             * Only used to construct the anonymous user
             */
            explicit User(StringView name) : name_(name.data(), name.size()), lastSeen_(0) {}

            void registerVote(const DiscussionThreadMessageRef& message)
            {
                votedMessages_.insert(DiscussionThreadMessageWeakRef(message));
            }

        private:
            std::string auth_;
            std::string name_;
            std::string info_;
            Timestamp lastSeen_;
            std::set<DiscussionThreadMessageWeakRef, std::owner_less<DiscussionThreadMessageWeakRef>> votedMessages_;
            DiscussionThreadCollectionBase<OrderedIndexForId> subscribedThreads_;
        };

        typedef std::shared_ptr<User> UserRef;
        typedef std::  weak_ptr<User> UserWeakRef;
    }
}
