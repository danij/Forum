#pragma once

#include "EntityCommonTypes.h"
#include "EntityDiscussionThreadCollection.h"
#include "EntityDiscussionThreadMessageCollection.h"
#include "EntityMessageCommentCollection.h"

#include <string>
#include <set>

#include <boost/noncopyable.hpp>

namespace Forum
{
    namespace Entities
    {
        /**
        * Stores a user that creates content
        * Repositories are responsible for updating the relationships between this message and other entities
        */
        class User final : public StoresEntityPointer<User>,
                           private boost::noncopyable
        {
        public:
            const auto& id()                const { return id_; }

                   auto created()           const { return created_; }
            const auto& creationDetails()   const { return creationDetails_; }
                           
            const auto& auth()              const { return auth_; }
            const auto& name()              const { return name_; }

             StringView info()              const { return info_; }

                   auto lastSeen()          const { return lastSeen_; }

            const auto& threads()           const { return threads_; }
            const auto& subscribedThreads() const { return subscribedThreads_; }
            const auto& threadMessages()    const { return threadMessages_; }

                   auto threadCount()       const { return threads_.count(); }
                   auto messageCount()      const { return threadMessages_.count(); }
                   
                   auto votedMessages()     const { return Helpers::toConst(votedMessages_); }
            const auto& messageComments()   const { return messageComments_; }

            enum ChangeType : uint32_t
            {
                None = 0,
                Name,
                Info
            };

            struct ChangeNotification
            {
                std::function<void(const User&)> onPrepareUpdateAuth;
                std::function<void(const User&)> onUpdateAuth;

                std::function<void(const User&)> onPrepareUpdateName;
                std::function<void(const User&)> onUpdateName;
                
                std::function<void(const User&)> onPrepareUpdateLastSeen;
                std::function<void(const User&)> onUpdateLastSeen;
                
                std::function<void(const User&)> onPrepareUpdateThreadCount;
                std::function<void(const User&)> onUpdateThreadCount;
                
                std::function<void(const User&)> onPrepareUpdateMessageCount;
                std::function<void(const User&)> onUpdateMessageCount;
            };

            static auto& changeNotifications() { return changeNotifications_; }

            User(IdType id, Timestamp created, VisitDetails creationDetails)
                : id_(std::move(id)), created_(created), creationDetails_(std::move(creationDetails))
            {
                threads_.onPrepareCountChange()        = [this]() { changeNotifications_.onPrepareUpdateThreadCount(*this); };
                threads_.onCountChange()               = [this]() { changeNotifications_.onUpdateThreadCount(*this); };

                threadMessages_.onPrepareCountChange() = [this]() { changeNotifications_.onPrepareUpdateMessageCount(*this); };
                threadMessages_.onCountChange()        = [this]() { changeNotifications_.onUpdateMessageCount(*this); };
            }

            explicit User(StringView name) : id_(IdType::empty), name_(name)
            {}

            auto& info()              { return info_; }
            auto& threads()           { return threads_; }
            auto& subscribedThreads() { return subscribedThreads_; }
            auto& threadMessages()    { return threadMessages_; }
            auto& votedMessages()     { return votedMessages_; }
            auto& messageComments()   { return messageComments_; }

            void updateAuth(std::string&& value)
            {
                changeNotifications_.onPrepareUpdateAuth(*this);
                auth_ = std::move(value);
                changeNotifications_.onUpdateAuth(*this);
            }

            void updateName(Helpers::StringWithSortKey&& name)
            {
                changeNotifications_.onPrepareUpdateName(*this);
                name_ = std::move(name);
                changeNotifications_.onUpdateName(*this);
            }

            void updateLastSeen(Timestamp value)
            {
                changeNotifications_.onPrepareUpdateLastSeen(*this);
                lastSeen_ = value;
                changeNotifications_.onUpdateLastSeen(*this);
            }

            void registerVote(DiscussionThreadMessagePtr message)
            {
                votedMessages_.insert(message);
            }

        private:
            static ChangeNotification changeNotifications_;

            IdType id_;
            Timestamp created_{0};
            VisitDetails creationDetails_;

            std::string auth_;
            Helpers::StringWithSortKey name_;
            std::string info_;

            Timestamp lastSeen_{0};
            
            DiscussionThreadCollectionWithOrderedId threads_;
            DiscussionThreadCollectionWithOrderedId subscribedThreads_;
                                    
            DiscussionThreadMessageCollection threadMessages_;
            std::set<DiscussionThreadMessagePtr> votedMessages_;
            
            MessageCommentCollection messageComments_;
        };

        typedef EntityPointer<User> UserPtr;
        typedef EntityPointer<const User> UserConstPtr;
    }
}
