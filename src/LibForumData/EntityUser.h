#pragma once

#include "EntityCommonTypes.h"
#include "EntityDiscussionThreadCollection.h"
#include "EntityDiscussionThreadMessageCollection.h"
#include "EntityMessageCommentCollection.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <set>

#include <boost/noncopyable.hpp>
#include <boost/circular_buffer.hpp>

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

            const auto& info()              const { return info_; }
            const auto& title()             const { return title_; }
            const auto& signature()         const { return signature_; }
             StringView logo()              const { return logo_; }
                   bool hasLogo()           const { return logo_.size() > 0; }

                   auto lastSeen()          const { return lastSeen_; }

                   auto receivedUpVotes()   const { return receivedUpVotes_; }
                   auto receivedDownVotes() const { return receivedDownVotes_; }

            const auto& threads()           const { return threads_; }
            const auto& subscribedThreads() const { return subscribedThreads_; }
            const auto& threadMessages()    const { return threadMessages_; }

                   auto threadCount()       const { return threads_.count(); }
                   auto messageCount()      const { return threadMessages_.count(); }

            auto votedMessages()     const
            {
                if ( ! votedMessages_) return Helpers::toConst(emptyVotedMessages_);
                return Helpers::toConst(*votedMessages_);
            }
            const auto& messageComments()   const
            {
                if ( ! messageComments_) return emptyMessageComments_;
                return *messageComments_;
            }
            const auto& voteHistory() const { return voteHistory_; }

            enum ChangeType : uint32_t
            {
                None = 0,
                Name,
                Info,
                Title,
                Signature,
                Logo,
            };

            typedef Helpers::JsonReadyStringWithSortKey<64> NameType;
            typedef Json::JsonReadyString<4> InfoType;
            typedef Json::JsonReadyString<4> TitleType;
            typedef Json::JsonReadyString<4> SignatureType;

            enum class ReceivedVoteHistoryEntryType : uint8_t
            {
                UpVote,
                DownVote,
                ResetVote
            };

            struct ReceivedVoteHistory final
            {
                IdType discussionThreadMessageId;
                IdType voterId;
                Timestamp at;
                ReceivedVoteHistoryEntryType type;
            };

            struct ChangeNotification final
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

            User(IdType id, NameType&& name, Timestamp created, VisitDetails creationDetails)
                : id_(std::move(id)), created_(created), creationDetails_(std::move(creationDetails)),
                  name_(std::move(name)), info_({}), title_({}), signature_({})
            {
                threads_.onPrepareCountChange()        = [this]() { changeNotifications_.onPrepareUpdateThreadCount(*this); };
                threads_.onCountChange()               = [this]() { changeNotifications_.onUpdateThreadCount(*this); };

                threadMessages_.onPrepareCountChange() = [this]() { changeNotifications_.onPrepareUpdateMessageCount(*this); };
                threadMessages_.onCountChange()        = [this]() { changeNotifications_.onUpdateMessageCount(*this); };
            }

            explicit User(StringView name) : id_(IdType::empty), name_(name), info_({}), title_({}), signature_({})
            {}

            auto& info()              { return info_; }
            auto& title()             { return title_; }
            auto& signature()         { return signature_; }
            auto& logo()              { return logo_; }
            auto& receivedUpVotes()   { return receivedUpVotes_; }
            auto& receivedDownVotes() { return receivedDownVotes_; }

            auto& threads()           { return threads_; }
            auto& subscribedThreads() { return subscribedThreads_; }
            auto& threadMessages()    { return threadMessages_; }
            auto& votedMessages()
            {
                if ( ! votedMessages_) votedMessages_.reset(new std::set<DiscussionThreadMessagePtr>);
                return *votedMessages_;
            }
            auto& messageComments()
            {
                if ( ! messageComments_) messageComments_.reset(new MessageCommentCollection);
                return *messageComments_;
            }
            auto& voteHistory()                    { return voteHistory_; }
            auto& voteHistoryLastRetrieved() const { return voteHistoryLastRetrieved_; }

            void updateAuth(std::string&& value)
            {
                changeNotifications_.onPrepareUpdateAuth(*this);
                auth_ = std::move(value);
                changeNotifications_.onUpdateAuth(*this);
            }

            void updateName(NameType&& name)
            {
                changeNotifications_.onPrepareUpdateName(*this);
                name_ = std::move(name);
                changeNotifications_.onUpdateName(*this);
            }

            void updateLastSeen(Timestamp value)
            {
                if (lastSeen_ == value) return;

                changeNotifications_.onPrepareUpdateLastSeen(*this);
                lastSeen_ = value;
                changeNotifications_.onUpdateLastSeen(*this);
            }

            void registerVote(DiscussionThreadMessagePtr message)
            {
                votedMessages().insert(message);
            }

            void removeVote(DiscussionThreadMessagePtr message)
            {
                auto set = votedMessages();
                auto it = set.find(message);
                if (it != set.end())
                {
                    set.erase(it);
                }
            }

        private:
            static ChangeNotification changeNotifications_;
            static const std::set<DiscussionThreadMessagePtr> emptyVotedMessages_;
            static const MessageCommentCollection emptyMessageComments_;

            IdType id_;
            Timestamp created_{0};
            VisitDetails creationDetails_;

            std::string auth_;
            NameType name_;
            InfoType info_;
            TitleType title_;
            SignatureType signature_;
            std::string logo_;

            Timestamp lastSeen_{0};

            DiscussionThreadCollectionWithOrderedId threads_;
            DiscussionThreadCollectionWithHashedId subscribedThreads_;

            DiscussionThreadMessageCollection threadMessages_;
            std::unique_ptr<std::set<DiscussionThreadMessagePtr>> votedMessages_;

            std::unique_ptr<MessageCommentCollection> messageComments_;

            static constexpr size_t MaxVotesInHistory = 64;
            boost::circular_buffer_space_optimized<ReceivedVoteHistory> voteHistory_{ MaxVotesInHistory };
            mutable std::atomic<int64_t> voteHistoryLastRetrieved_{ 0 };

            int_fast32_t receivedUpVotes_{ 0 };
            int_fast32_t receivedDownVotes_{ 0 };
        };

        typedef EntityPointer<User> UserPtr;
        typedef EntityPointer<const User> UserConstPtr;
    }
}
