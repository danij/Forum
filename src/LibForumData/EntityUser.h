/*
Fast Forum Backend
Copyright (C) 2016-present Daniel Jurcau

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "EntityCommonTypes.h"
#include "EntityDiscussionThreadCollection.h"
#include "EntityDiscussionThreadMessageCollection.h"
#include "EntityMessageCommentCollection.h"
#include "EntityPrivateMessageCollection.h"
#include "EntityAttachmentCollection.h"
#include "CircularBuffer.h"
#include "SpinLock.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

#include <boost/noncopyable.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/container/flat_set.hpp>

namespace Forum::Entities
{
    /**
    * Stores a user that creates content
    * Repositories are responsible for updating the relationships between this message and other entities
    */
    class User final : boost::noncopyable
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
        const auto& logo()              const { return logo_; }
               bool hasLogo()           const { return ! logo_.empty(); }

               auto lastSeen()          const { return lastSeen_; }

               auto receivedUpVotes()   const { return receivedUpVotes_; }
               auto receivedDownVotes() const { return receivedDownVotes_; }

        const auto& threads()           const { return threads_; }
        const auto& subscribedThreads() const { return subscribedThreads_; }
        const auto& threadMessages()    const { return threadMessages_; }

               auto threadCount()       const { return threads_.count(); }
               auto messageCount()      const { return threadMessages_.count(); }

        auto votedMessages() const
        {
            if ( ! votedMessages_) return Helpers::toConst(emptyVotedMessages_);
            return Helpers::toConst(*votedMessages_);
        }
        const auto& messageComments() const
        {
            if ( ! messageComments_) return emptyMessageComments_;
            return *messageComments_;
        }
        const auto& receivedPrivateMessages() const
        {
            if ( ! receivedPrivateMessages_) return emptyPrivateMessages_;
            return *receivedPrivateMessages_;
        }
        const auto& sentPrivateMessages() const
        {
            if ( ! sentPrivateMessages_) return emptyPrivateMessages_;
            return *sentPrivateMessages_;
        }
        const auto& attachments() const
        {
            if ( ! attachments_) return emptyAttachments_;
            return *attachments_;
        }

        const auto& voteHistory()     const { return voteHistory_; }
        const auto& quoteHistory()    const { return quoteHistory_; }

              auto  attachmentQuota() const { return attachmentQuota_; }

        enum ChangeType : uint32_t
        {
            None = 0,
            Name,
            Info,
            Title,
            Signature,
            Logo,
            AttachmentQuota
        };

        typedef Helpers::JsonReadyStringWithSortKey<64> NameType;
        typedef Json::JsonReadyString<> InfoType;
        typedef Json::JsonReadyString<> TitleType;
        typedef Json::JsonReadyString<> SignatureType;
        typedef Json::JsonReadyString<> LogoType;

        typedef boost::container::flat_set<DiscussionThreadMessagePtr> VotedMessagesType;

        enum class ReceivedVoteHistoryEntryType : uint8_t
        {
            UpVote,
            DownVote,
            ResetVote
        };

        struct ReceivedVoteHistory final
        {
            IdType discussionThreadMessageId;
            Timestamp at;
            ReceivedVoteHistoryEntryType type;
        };

        struct ChangeNotification final
        {
            std::function<void(User&)> onPrepareUpdateAuth;
            std::function<void(User&)> onUpdateAuth;

            std::function<void(User&)> onPrepareUpdateName;
            std::function<void(User&)> onUpdateName;

            std::function<void(User&)> onPrepareUpdateLastSeen;
            std::function<void(User&)> onUpdateLastSeen;

            std::function<void(User&)> onPrepareUpdateThreadCount;
            std::function<void(User&)> onUpdateThreadCount;

            std::function<void(User&)> onPrepareUpdateMessageCount;
            std::function<void(User&)> onUpdateMessageCount;
        };

        static auto& changeNotifications() { return changeNotifications_; }

        User(const IdType id, NameType&& name, const Timestamp created, VisitDetails creationDetails)
            : id_(id), created_(created), creationDetails_(creationDetails),
              name_(std::move(name)), info_({}), title_({}), signature_({}), logo_({})
        {
            threads_.onPrepareCountChange()
                = [](void* state) { changeNotifications_.onPrepareUpdateThreadCount(*reinterpret_cast<User*>(state)); };
            threads_.onPrepareCountChange().state() = this;
            threads_.onCountChange()
                = [](void* state) { changeNotifications_.onUpdateThreadCount(*reinterpret_cast<User*>(state)); };
            threads_.onCountChange().state() = this;
                
            threadMessages_.onPrepareCountChange()
                = [](void* state) { changeNotifications_.onPrepareUpdateMessageCount(*reinterpret_cast<User*>(state)); };
            threadMessages_.onPrepareCountChange().state() = this;
            threadMessages_.onCountChange()
                = [](void* state) { changeNotifications_.onUpdateMessageCount(*reinterpret_cast<User*>(state)); };
            threadMessages_.onCountChange().state() = this;
        }

        explicit User(const StringView name) : id_(IdType::empty), name_(name), info_({}), title_({}), signature_({}), logo_({})
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
            if ( ! votedMessages_) votedMessages_.reset(new VotedMessagesType);
            return *votedMessages_;
        }
        auto& messageComments()
        {
            if ( ! messageComments_) messageComments_.reset(new MessageCommentCollectionLowMemory);
            return *messageComments_;
        }
        auto& receivedPrivateMessages()
        {
            if ( ! receivedPrivateMessages_) receivedPrivateMessages_.reset(new PrivateMessageCollection);
            return *receivedPrivateMessages_;
        }
        auto& sentPrivateMessages()
        {
            if ( ! sentPrivateMessages_) sentPrivateMessages_.reset(new PrivateMessageCollection);
            return *sentPrivateMessages_;
        }
        auto& attachments()
        {
            if ( ! attachments_) attachments_.reset(new AttachmentCollection);
            return *attachments_;
        }

        auto& voteHistory()                    { return voteHistory_; }
        auto& voteHistoryLastRetrieved() const { return voteHistoryLastRetrieved_; }

        auto& quoteHistory()                   { return quoteHistory_; }

        auto& showInOnlineUsers()        const { return showInOnlineUsers_; }

        auto& voteHistoryNotRead()       const { return voteHistoryNotRead_; }
        auto& quotesHistoryNotRead()     const { return quotesHistoryNotRead_; }
        auto& privateMessagesNotRead()   const { return privateMessagesNotRead_; }

        auto& attachmentQuota()                { return attachmentQuota_; }

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

        void updateLastSeen(const Timestamp value)
        {
            if (lastSeen_ == value) return;

            changeNotifications_.onPrepareUpdateLastSeen(*this);
            lastSeen_ = value;
            changeNotifications_.onUpdateLastSeen(*this);
        }

        void registerVote(const DiscussionThreadMessagePtr message)
        {
            votedMessages().insert(message);
        }

        void removeVote(const DiscussionThreadMessagePtr message)
        {
            auto& set = votedMessages();
            const auto it = set.find(message);
            if (it != set.end())
            {
                set.erase(it);
            }
        }

        auto latestPageVisited(IdTypeRef threadId) const
        {
            std::lock_guard<decltype(latestThreadPageVisitedLock_)> _(latestThreadPageVisitedLock_);

            const auto it = latestThreadPageVisited_.find(threadId);
            if (it != latestThreadPageVisited_.end())
            {
                return it->second;
            }
            return decltype(it->second){};
        }

        bool updateLatestPageVisited(IdTypeRef threadId, uint32_t pageNumber) const
        {
            std::lock_guard<decltype(latestThreadPageVisitedLock_)> _(latestThreadPageVisitedLock_);

            auto it = latestThreadPageVisited_.find(threadId);
            if (it == latestThreadPageVisited_.end())
            {
                latestThreadPageVisited_.insert(std::make_pair(threadId, pageNumber));
                return true;
            }
            if (it->second < pageNumber)
            {
                it->second = pageNumber;
                return true;
            }
            return false;
        }

    private:
        static ChangeNotification changeNotifications_;
        static const VotedMessagesType emptyVotedMessages_;
        static const MessageCommentCollectionLowMemory emptyMessageComments_;
        static const PrivateMessageCollection emptyPrivateMessages_;
        static const AttachmentCollection emptyAttachments_;

        IdType id_;
        Timestamp created_{0};
        VisitDetails creationDetails_;

        std::string auth_;
        NameType name_;
        InfoType info_;
        TitleType title_;
        SignatureType signature_;
        LogoType logo_;

        Timestamp lastSeen_{0};
        boost::optional<uint64_t> attachmentQuota_{};

        DiscussionThreadCollectionLowMemory threads_;
        DiscussionThreadCollectionLowMemory subscribedThreads_;

        DiscussionThreadMessageCollectionLowMemory threadMessages_;
        std::unique_ptr<VotedMessagesType> votedMessages_;

        std::unique_ptr<MessageCommentCollectionLowMemory> messageComments_;

        static constexpr size_t MaxVotesInHistory = 16;
        Helpers::CircularBuffer<ReceivedVoteHistory, MaxVotesInHistory> voteHistory_{};
        mutable std::atomic<Timestamp> voteHistoryLastRetrieved_{ 0 };

        int_fast32_t receivedUpVotes_{ 0 };
        int_fast32_t receivedDownVotes_{ 0 };

        mutable std::atomic<uint16_t> voteHistoryNotRead_{ 0 };
        mutable std::atomic<uint16_t> quotesHistoryNotRead_{ 0 };
        mutable std::atomic<uint16_t> privateMessagesNotRead_{ 0 };

        static constexpr size_t MaxQuotesInHistory = 16;
        Helpers::CircularBuffer<IdType, MaxQuotesInHistory> quoteHistory_{};

        mutable std::atomic_bool showInOnlineUsers_{ false };

        mutable std::unordered_map<IdType, uint32_t> latestThreadPageVisited_;
        mutable Helpers::SpinLock latestThreadPageVisitedLock_;

        std::unique_ptr<PrivateMessageCollection> receivedPrivateMessages_;
        std::unique_ptr<PrivateMessageCollection> sentPrivateMessages_;

        std::unique_ptr<AttachmentCollection> attachments_;
    };

    typedef User* UserPtr;
    typedef const User* UserConstPtr;
}
