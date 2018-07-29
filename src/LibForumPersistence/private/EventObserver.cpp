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

#include "EventObserver.h"
#include "PersistenceFormat.h"
#include "FileAppender.h"
#include "TypeHelpers.h"
#include "Logging.h"
#include "SeparateThreadConsumer.h"

#include <chrono>
#include <map>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>
#include <cstring>

using namespace Forum;
using namespace Forum::Persistence;
using namespace Forum::Repository;
using namespace Forum::Entities;
using namespace Forum::Authorization;
using namespace Forum::Helpers;

struct BlobPart
{
    const char* address;
    BlobSizeType size;
    bool includeSizePrefix;

    size_t totalSize() const
    {
        return size + (includeSizePrefix ? sizeof(uint32_t) : 0);
    }
};

class EventCollector final : public SeparateThreadConsumer<EventCollector, SeparateThreadConsumerBlob>
{
public:
    EventCollector(const std::filesystem::path& destinationFolder, const time_t refreshEverySeconds)
        : appender_(destinationFolder, refreshEverySeconds)
    {
    }

private:
    friend class SeparateThreadConsumer<EventCollector, SeparateThreadConsumerBlob>;

    void onFail(const uint32_t failNr)
    {
        if (0 == failNr)
        {
            FORUM_LOG_WARNING << "Persistence queue is full";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }

    void consumeValues(SeparateThreadConsumerBlob* values, const size_t nrOfValues)
    {
        appender_.append(values, nrOfValues);

        for (size_t i = 0; i < nrOfValues; ++i)
        {
            SeparateThreadConsumerBlob::free(values[i]);
        }
    }

    void onThreadFinish()
    {}

    FileAppender appender_;
};

struct EventObserver::EventObserverImpl final : private boost::noncopyable
{
    EventObserverImpl(ReadEvents& readEvents, WriteEvents& writeEvents,
                      const std::filesystem::path& destinationFolder, time_t refreshEverySeconds)
        : readEvents(readEvents), writeEvents(writeEvents), collector(destinationFolder, refreshEverySeconds)
    {
        timerThread = std::thread([this]() {this->timerLoop();});
        bindObservers();
    }

    ~EventObserverImpl()
    {
        for (auto& connection : connections)
        {
            connection.disconnect();
        }
        stopTimerThread = true;
        timerThread.join();
    }

    void timerLoop()
    {
        while ( ! stopTimerThread)
        {
            if (timerThreadCurrentIncrement >= updateThreadVisitedEveryIncrement)
            {
                updateThreadVisited();
                timerThreadCurrentIncrement = 0;
            }
            std::this_thread::sleep_for(timerThreadCheckEverySeconds);
            ++timerThreadCurrentIncrement;
        }
    }

    static constexpr EventContextVersionType ContextVersion = 1;

    void recordBlob(EventType eventType, EventVersionType version, BlobPart* parts, const size_t nrOfParts)
    {
        const auto totalSize = std::accumulate(parts, parts + nrOfParts, size_t(0), [](const size_t total, BlobPart& part)
        {
           return total + part.totalSize();
        }) + EventHeaderSize;

        const auto blob = SeparateThreadConsumerBlob::allocateNew(totalSize);

        char* buffer = blob.buffer;

        writeValue(buffer, eventType); buffer += sizeof(eventType);
        writeValue(buffer, version); buffer += sizeof(version);
        writeValue(buffer, ContextVersion); buffer += sizeof(ContextVersion);

        for (size_t i = 0; i < nrOfParts; ++i)
        {
            const auto& part = parts[i];
            if (part.includeSizePrefix)
            {
                writeValue(buffer, part.size);
                buffer += sizeof(part.size);
            }
            if (part.size > 0)
            {
                buffer = std::copy(part.address, part.address + part.size, buffer);
            }
        }

        collector.enqueue(blob);
    }

    void bindObservers()
    {
        connections.push_back(writeEvents.                         onAddNewUser.connect([this](auto context, auto& user)                       { this->onAddNewUser                         (context, user); }));
        connections.push_back(writeEvents.                         onChangeUser.connect([this](auto context, auto& user, auto change)          { this->onChangeUser                         (context, user, change); }));
        connections.push_back(writeEvents.                         onDeleteUser.connect([this](auto context, auto& user)                       { this->onDeleteUser                         (context, user); }));
        connections.push_back(writeEvents.             onAddNewDiscussionThread.connect([this](auto context, auto& thread)                     { this->onAddNewDiscussionThread             (context, thread); }));
        connections.push_back(writeEvents.             onChangeDiscussionThread.connect([this](auto context, auto& thread, auto change)        { this->onChangeDiscussionThread             (context, thread, change); }));
        connections.push_back(writeEvents.             onDeleteDiscussionThread.connect([this](auto context, auto& thread)                     { this->onDeleteDiscussionThread             (context, thread); }));
        connections.push_back(writeEvents.             onMergeDiscussionThreads.connect([this](auto context, auto& fromThread, auto& toThread) { this->onMergeDiscussionThreads             (context, fromThread, toThread); }));
        connections.push_back(writeEvents.        onMoveDiscussionThreadMessage.connect([this](auto context, auto& message, auto& intoThread)  { this->onMoveDiscussionThreadMessage        (context, message, intoThread); }));
        connections.push_back(writeEvents.        onSubscribeToDiscussionThread.connect([this](auto context, auto& thread)                     { this->onSubscribeToDiscussionThread        (context, thread); }));
        connections.push_back(writeEvents.    onUnsubscribeFromDiscussionThread.connect([this](auto context, auto& thread)                     { this->onUnsubscribeFromDiscussionThread    (context, thread); }));
        connections.push_back(writeEvents.      onAddNewDiscussionThreadMessage.connect([this](auto context, auto& message)                    { this->onAddNewDiscussionThreadMessage      (context, message); }));
        connections.push_back(writeEvents.      onChangeDiscussionThreadMessage.connect([this](auto context, auto& message, auto change)       { this->onChangeDiscussionThreadMessage      (context, message, change); }));
        connections.push_back(writeEvents.      onDeleteDiscussionThreadMessage.connect([this](auto context, auto& message)                    { this->onDeleteDiscussionThreadMessage      (context, message); }));
        connections.push_back(writeEvents.      onDiscussionThreadMessageUpVote.connect([this](auto context, auto& message)                    { this->onDiscussionThreadMessageUpVote      (context, message); }));
        connections.push_back(writeEvents.    onDiscussionThreadMessageDownVote.connect([this](auto context, auto& message)                    { this->onDiscussionThreadMessageDownVote    (context, message); }));
        connections.push_back(writeEvents.   onDiscussionThreadMessageResetVote.connect([this](auto context, auto& message)                    { this->onDiscussionThreadMessageResetVote   (context, message); }));
        connections.push_back(writeEvents.onAddCommentToDiscussionThreadMessage.connect([this](auto context, auto& comment)                    { this->onAddCommentToDiscussionThreadMessage(context, comment); }));
        connections.push_back(writeEvents.onSolveDiscussionThreadMessageComment.connect([this](auto context, auto& comment)                    { this->onSolveDiscussionThreadMessageComment(context, comment); }));
        connections.push_back(writeEvents.                onAddNewDiscussionTag.connect([this](auto context, auto& tag)                        { this->onAddNewDiscussionTag                (context, tag); }));
        connections.push_back(writeEvents.                onChangeDiscussionTag.connect([this](auto context, auto& tag, auto change)           { this->onChangeDiscussionTag                (context, tag, change); }));
        connections.push_back(writeEvents.                onDeleteDiscussionTag.connect([this](auto context, auto& tag)                        { this->onDeleteDiscussionTag                (context, tag); }));
        connections.push_back(writeEvents.           onAddDiscussionTagToThread.connect([this](auto context, auto& tag, auto& thread)          { this->onAddDiscussionTagToThread           (context, tag, thread); }));
        connections.push_back(writeEvents.      onRemoveDiscussionTagFromThread.connect([this](auto context, auto& tag, auto& thread)          { this->onRemoveDiscussionTagFromThread      (context, tag, thread); }));
        connections.push_back(writeEvents.                onMergeDiscussionTags.connect([this](auto context, auto& fromTag, auto& toTag)       { this->onMergeDiscussionTags                (context, fromTag, toTag); }));
        connections.push_back(writeEvents.           onAddNewDiscussionCategory.connect([this](auto context, auto& category)                   { this->onAddNewDiscussionCategory           (context, category); }));
        connections.push_back(writeEvents.           onChangeDiscussionCategory.connect([this](auto context, auto& category, auto change)      { this->onChangeDiscussionCategory           (context, category, change); }));
        connections.push_back(writeEvents.           onDeleteDiscussionCategory.connect([this](auto context, auto& category)                   { this->onDeleteDiscussionCategory           (context, category); }));
        connections.push_back(writeEvents.         onAddDiscussionTagToCategory.connect([this](auto context, auto& tag, auto& category)        { this->onAddDiscussionTagToCategory         (context, tag, category); }));
        connections.push_back(writeEvents.    onRemoveDiscussionTagFromCategory.connect([this](auto context, auto& tag, auto& category)        { this->onRemoveDiscussionTagFromCategory    (context, tag, category); }));

        connections.push_back(readEvents.             onGetDiscussionThreadById.connect([this](auto context, auto& thread)                     { this->onGetDiscussionThreadById            (context, thread); }));

        //authorization
        connections.push_back(writeEvents.onChangeDiscussionThreadMessageRequiredPrivilegeForThreadMessage.connect([this](auto context, auto& message, auto privilege, auto value)             { this->changeDiscussionThreadMessageRequiredPrivilegeForThreadMessage(context, message, privilege, value); }));
        connections.push_back(writeEvents.       onChangeDiscussionThreadMessageRequiredPrivilegeForThread.connect([this](auto context, auto& thread, auto privilege, auto value)              { this->changeDiscussionThreadMessageRequiredPrivilegeForThread       (context, thread, privilege, value); }));
        connections.push_back(writeEvents.          onChangeDiscussionThreadMessageRequiredPrivilegeForTag.connect([this](auto context, auto& tag, auto privilege, auto value)                 { this->changeDiscussionThreadMessageRequiredPrivilegeForTag          (context, tag, privilege, value); }));
        connections.push_back(writeEvents.       onChangeDiscussionThreadMessageRequiredPrivilegeForumWide.connect([this](auto context, auto privilege, auto value)                            { this->changeDiscussionThreadMessageRequiredPrivilegeForumWide       (context, privilege, value); }));
        connections.push_back(writeEvents.              onChangeDiscussionThreadRequiredPrivilegeForThread.connect([this](auto context, auto& thread, auto privilege, auto value)              { this->changeDiscussionThreadRequiredPrivilegeForThread              (context, thread, privilege, value); }));
        connections.push_back(writeEvents.                 onChangeDiscussionThreadRequiredPrivilegeForTag.connect([this](auto context, auto& tag, auto privilege, auto value)                 { this->changeDiscussionThreadRequiredPrivilegeForTag                 (context, tag, privilege, value); }));
        connections.push_back(writeEvents.              onChangeDiscussionThreadRequiredPrivilegeForumWide.connect([this](auto context, auto privilege, auto value)                            { this->changeDiscussionThreadRequiredPrivilegeForumWide              (context, privilege, value); }));
        connections.push_back(writeEvents.                    onChangeDiscussionTagRequiredPrivilegeForTag.connect([this](auto context, auto& tag, auto privilege, auto value)                 { this->changeDiscussionTagRequiredPrivilegeForTag                    (context, tag, privilege, value); }));
        connections.push_back(writeEvents.                 onChangeDiscussionTagRequiredPrivilegeForumWide.connect([this](auto context, auto privilege, auto value)                            { this->changeDiscussionTagRequiredPrivilegeForumWide                 (context, privilege, value); }));
        connections.push_back(writeEvents.          onChangeDiscussionCategoryRequiredPrivilegeForCategory.connect([this](auto context, auto& category, auto privilege, auto value)            { this->changeDiscussionCategoryRequiredPrivilegeForCategory          (context, category, privilege, value); }));
        connections.push_back(writeEvents.            onChangeDiscussionCategoryRequiredPrivilegeForumWide.connect([this](auto context, auto privilege, auto value)                            { this->changeDiscussionCategoryRequiredPrivilegeForumWide            (context, privilege, value); }));
        connections.push_back(writeEvents.                              onChangeForumWideRequiredPrivilege.connect([this](auto context, auto privilege, auto value)                            { this->changeForumWideRequiredPrivilege                              (context, privilege, value); }));
        connections.push_back(writeEvents.                          onChangeForumWideDefaultPrivilegeLevel.connect([this](auto context, auto privilegeDuration, auto value, auto duration)     { this->changeForumWideDefaultPrivilegeLevel                          (context, privilegeDuration, value, duration); }));
        connections.push_back(writeEvents.                        onAssignDiscussionThreadMessagePrivilege.connect([this](auto context, auto& message, auto& user, auto value, auto duration)  { this->assignDiscussionThreadMessagePrivilege                        (context, message, user, value, duration); }));
        connections.push_back(writeEvents.                               onAssignDiscussionThreadPrivilege.connect([this](auto context, auto& thread, auto& user, auto value, auto duration)   { this->assignDiscussionThreadPrivilege                               (context, thread, user, value, duration); }));
        connections.push_back(writeEvents.                                  onAssignDiscussionTagPrivilege.connect([this](auto context, auto& tag, auto& user, auto value, auto duration)      { this->assignDiscussionTagPrivilege                                  (context, tag, user, value, duration); }));
        connections.push_back(writeEvents.                             onAssignDiscussionCategoryPrivilege.connect([this](auto context, auto& category, auto& user, auto value, auto duration) { this->assignDiscussionCategoryPrivilege                             (context, category, user, value, duration); }));
        connections.push_back(writeEvents.                                      onAssignForumWidePrivilege.connect([this](auto context, auto& user, auto value, auto duration)                 { this->assignForumWidePrivilege                                      (context, user, value, duration); }));
    }

    static constexpr size_t UuidSize = boost::uuids::uuid::static_size();
    static const PersistentTimestampType ZeroTimestamp;
    static const Helpers::IpAddress ZeroIpAddress;

    typedef BlobSizeType SizeType;

#define POINTER(x) reinterpret_cast<const char*>(x)

#define ADD_CONTEXT_BLOB_PARTS \
    { POINTER(&contextTimestamp), sizeof(contextTimestamp), false }, \
    { POINTER(&context.performedBy.id().value().data), UuidSize, false }, \
    { POINTER(context.ipAddress.data()), static_cast<SizeType>(context.ipAddress.dataSize()), false }

#define ADD_EMPTY_CONTEXT_BLOB_PARTS \
    { POINTER(&ZeroTimestamp), sizeof(ZeroTimestamp), false }, \
    { POINTER(&UuidString::empty.value().data), UuidSize, false }, \
    { POINTER(ZeroIpAddress.data()), static_cast<SizeType>(ZeroIpAddress.dataSize()), false }

    void onAddNewUser(ObserverContext context, const User& user)
    {
        PersistentTimestampType contextTimestamp = context.timestamp; //make sure the size is fixed for serialization
        auto userName = user.name().string();

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&user.id().value().data), UuidSize, false }, \
            { POINTER(user.auth().data()), static_cast<SizeType>(user.auth().size()), true }, \
            { POINTER(userName.data()), static_cast<SizeType>(userName.size()), true }, \
        };

        recordBlob(EventType::ADD_NEW_USER, 1, parts, std::size(parts));
    }

    void onChangeUser(ObserverContext context, const User& user, User::ChangeType change)
    {
        switch (change)
        {
        case User::Name:
            onChangeUserName(context, user);
            break;
        case User::Info:
            onChangeUserInfo(context, user);
            break;
        case User::Title:
            onChangeUserTitle(context, user);
            break;
        case User::Signature:
            onChangeUserSignature(context, user);
            break;
        case User::Logo:
            onChangeUserLogo(context, user);
            break;
        default:
            break;
        }
    }

    void onChangeUserName(ObserverContext context, const User& user)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        auto userName = user.name().string();

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&user.id().value().data), UuidSize, false },
            { POINTER(userName.data()), static_cast<SizeType>(userName.size()), true }
        };

        recordBlob(EventType::CHANGE_USER_NAME, 1, parts, std::size(parts));
    }

    void onChangeUserInfo(ObserverContext context, const User& user)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        auto userInfo = user.info().string();

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&user.id().value().data), UuidSize, false },
            { POINTER(userInfo.data()), static_cast<SizeType>(userInfo.size()), true }
        };

        recordBlob(EventType::CHANGE_USER_INFO, 1, parts, std::size(parts));
    }

    void onChangeUserTitle(ObserverContext context, const User& user)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        auto userTitle = user.title().string();

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&user.id().value().data), UuidSize, false },
            { POINTER(userTitle.data()), static_cast<SizeType>(userTitle.size()), true }
        };

        recordBlob(EventType::CHANGE_USER_TITLE, 1, parts, std::size(parts));
    }

    void onChangeUserSignature(ObserverContext context, const User& user)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        auto userSignature = user.signature().string();

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&user.id().value().data), UuidSize, false },
            { POINTER(userSignature.data()), static_cast<SizeType>(userSignature.size()), true }
        };

        recordBlob(EventType::CHANGE_USER_SIGNATURE, 1, parts, std::size(parts));
    }

    void onChangeUserLogo(ObserverContext context, const User& user)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        StringView userLogo = user.logo();

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&user.id().value().data), UuidSize, false },
            { POINTER(userLogo.data()), static_cast<SizeType>(userLogo.size()), true }
        };

        recordBlob(EventType::CHANGE_USER_LOGO, 1, parts, std::size(parts));
    }

    void onDeleteUser(ObserverContext context, const User& user)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&user.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::DELETE_USER, 1, parts, std::size(parts));
    }

    void onAddNewDiscussionThread(ObserverContext context, const DiscussionThread& thread)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        auto threadName = thread.name().string();

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&thread.id().value().data), UuidSize, false },
            { POINTER(threadName.data()), static_cast<SizeType>(threadName.size()), true }
        };

        recordBlob(EventType::ADD_NEW_DISCUSSION_THREAD, 1, parts, std::size(parts));
    }

    void onChangeDiscussionThread(ObserverContext context, const DiscussionThread& thread,
                                  DiscussionThread::ChangeType change)
    {
        switch (change)
        {
        case DiscussionThread::Name:
            onChangeDiscussionThreadName(context, thread);
            break;
        case DiscussionThread::PinDisplayOrder:
            onChangeDiscussionThreadPinDisplayOrder(context, thread);
            break;
        default:
            break;
        }
    }

    void onChangeDiscussionThreadName(ObserverContext context, const DiscussionThread& thread)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        auto threadName = thread.name().string();

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&thread.id().value().data), UuidSize, false },
            { POINTER(threadName.data()), static_cast<SizeType>(threadName.size()), true }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_NAME, 1, parts, std::size(parts));
    }

    void onChangeDiscussionThreadPinDisplayOrder(ObserverContext context, const DiscussionThread& thread)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        uint16_t pinDisplayOrder = thread.pinDisplayOrder();

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&thread.id().value().data), UuidSize, false },
            { POINTER(&pinDisplayOrder), static_cast<SizeType>(sizeof(pinDisplayOrder)), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_PIN_DISPLAY_ORDER, 1, parts, std::size(parts));
    }

    void onDeleteDiscussionThread(ObserverContext context, const DiscussionThread& thread)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&thread.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::DELETE_DISCUSSION_THREAD, 1, parts, std::size(parts));
    }

    void onMergeDiscussionThreads(ObserverContext context, const DiscussionThread& fromThread,
                                  const DiscussionThread& toThread)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&fromThread.id().value().data), UuidSize, false },
            { POINTER(&toThread.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::MERGE_DISCUSSION_THREADS, 1, parts, std::size(parts));
    }

    void onMoveDiscussionThreadMessage(ObserverContext context, const DiscussionThreadMessage& message,
                                       const DiscussionThread& intoThread)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&message.id().value().data), UuidSize, false },
            { POINTER(&intoThread.id().value().data), UuidSize, false },
        };

        recordBlob(EventType::MOVE_DISCUSSION_THREAD_MESSAGE, 1, parts, std::size(parts));
    }

    void onSubscribeToDiscussionThread(ObserverContext context, const DiscussionThread& thread)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&thread.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::SUBSCRIBE_TO_DISCUSSION_THREAD, 1, parts, std::size(parts));
    }

    void onUnsubscribeFromDiscussionThread(ObserverContext context, const DiscussionThread& thread)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&thread.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::UNSUBSCRIBE_FROM_DISCUSSION_THREAD, 1, parts, std::size(parts));
    }

    void onAddNewDiscussionThreadMessage(ObserverContext context, const DiscussionThreadMessage& message)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        assert(message.parentThread());
        auto parentThreadId = message.parentThread()->id();

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&message.id().value().data), UuidSize, false },
            { POINTER(&parentThreadId.value().data), UuidSize, false },
            { POINTER(message.content().data()), static_cast<SizeType>(message.content().size()), true }
        };

        recordBlob(EventType::ADD_NEW_DISCUSSION_THREAD_MESSAGE, 1, parts, std::size(parts));
    }

    void onChangeDiscussionThreadMessage(ObserverContext context, const DiscussionThreadMessage& message,
                                         DiscussionThreadMessage::ChangeType change)
    {
        switch (change)
        {
        case DiscussionThreadMessage::Content:
            onChangeDiscussionThreadMessageContentContent(context, message);
            break;
        default:
            break;
        }
    }

    void onChangeDiscussionThreadMessageContentContent(ObserverContext context, const DiscussionThreadMessage& message)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&message.id().value().data), UuidSize, false },
            { POINTER(message.content().data()), static_cast<SizeType>(message.content().size()), true },
            { POINTER(message.lastUpdatedReason().data()), static_cast<SizeType>(message.lastUpdatedReason().size()), true }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT, 1, parts, std::size(parts));
    }

    void onDeleteDiscussionThreadMessage(ObserverContext context, const DiscussionThreadMessage& message)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&message.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::DELETE_DISCUSSION_THREAD_MESSAGE, 1, parts, std::size(parts));
    }

    void onDiscussionThreadMessageUpVote(ObserverContext context, const DiscussionThreadMessage& message)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&message.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::DISCUSSION_THREAD_MESSAGE_UP_VOTE, 1, parts, std::size(parts));
    }

    void onDiscussionThreadMessageDownVote(ObserverContext context, const DiscussionThreadMessage& message)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&message.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::DISCUSSION_THREAD_MESSAGE_DOWN_VOTE, 1, parts, std::size(parts));
    }

    void onDiscussionThreadMessageResetVote(ObserverContext context, const DiscussionThreadMessage& message)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&message.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::DISCUSSION_THREAD_MESSAGE_RESET_VOTE, 1, parts, std::size(parts));
    }

    void onAddCommentToDiscussionThreadMessage(ObserverContext context, const MessageComment& comment)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        auto parentMessageId = comment.parentMessage().id();

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&comment.id().value().data), UuidSize, false },
            { POINTER(&parentMessageId.value().data), UuidSize, false },
            { POINTER(comment.content().data()), static_cast<SizeType>(comment.content().size()), true }
        };

        recordBlob(EventType::ADD_COMMENT_TO_DISCUSSION_THREAD_MESSAGE, 1, parts, std::size(parts));
    }

    void onSolveDiscussionThreadMessageComment(ObserverContext context, const MessageComment& comment)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&comment.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::SOLVE_DISCUSSION_THREAD_MESSAGE_COMMENT, 1, parts, std::size(parts));
    }

    void onAddNewDiscussionTag(ObserverContext context, const DiscussionTag& tag)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        auto tagName = tag.name().string();

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&tag.id().value().data), UuidSize, false },
            { POINTER(tagName.data()), static_cast<SizeType>(tagName.size()), true }
        };

        recordBlob(EventType::ADD_NEW_DISCUSSION_TAG, 1, parts, std::size(parts));
    }

    void onChangeDiscussionTag(ObserverContext context, const DiscussionTag& tag, DiscussionTag::ChangeType change)
    {
        switch (change)
        {
        case DiscussionTag::Name:
            onChangeDiscussionTagName(context, tag);
            break;
        case DiscussionTag::UIBlob:
            onChangeDiscussionTagUIBlob(context, tag);
            break;
        default:
            break;
        }
    }

    void onChangeDiscussionTagName(ObserverContext context, const DiscussionTag& tag)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        auto tagName = tag.name().string();

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&tag.id().value().data), UuidSize, false },
            { POINTER(tagName.data()), static_cast<SizeType>(tagName.size()), true }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_TAG_NAME, 1, parts, std::size(parts));
    }

    void onChangeDiscussionTagUIBlob(ObserverContext context, const DiscussionTag& tag)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&tag.id().value().data), UuidSize, false },
            { POINTER(tag.uiBlob().data()), static_cast<SizeType>(tag.uiBlob().size()), true }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_TAG_UI_BLOB, 1, parts, std::size(parts));
    }

    void onDeleteDiscussionTag(ObserverContext context, const DiscussionTag& tag)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&tag.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::DELETE_DISCUSSION_TAG, 1, parts, std::size(parts));
    }

    void onAddDiscussionTagToThread(ObserverContext context, const DiscussionTag& tag, const DiscussionThread& thread)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&tag.id().value().data), UuidSize, false },
            { POINTER(&thread.id().value().data), UuidSize, false },
        };

        recordBlob(EventType::ADD_DISCUSSION_TAG_TO_THREAD, 1, parts, std::size(parts));
    }

    void onRemoveDiscussionTagFromThread(ObserverContext context, const DiscussionTag& tag, const DiscussionThread& thread)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&tag.id().value().data), UuidSize, false },
            { POINTER(&thread.id().value().data), UuidSize, false },
        };

        recordBlob(EventType::REMOVE_DISCUSSION_TAG_FROM_THREAD, 1, parts, std::size(parts));
    }

    void onMergeDiscussionTags(ObserverContext context, const DiscussionTag& fromTag, const DiscussionTag& toTag)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&fromTag.id().value().data), UuidSize, false },
            { POINTER(&toTag.id().value().data), UuidSize, false },
        };

        recordBlob(EventType::MERGE_DISCUSSION_TAGS, 1, parts, std::size(parts));
    }

    void onAddNewDiscussionCategory(ObserverContext context, const DiscussionCategory& category)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        UuidString parentCategoryId = {};
        if (category.parent())
        {
            parentCategoryId = category.parent()->id();
        }
        auto categoryName = category.name().string();

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&category.id().value().data), UuidSize, false },
            { POINTER(&parentCategoryId.value().data), UuidSize, false },
            { POINTER(categoryName.data()), static_cast<SizeType>(categoryName.size()), true }
        };

        recordBlob(EventType::ADD_NEW_DISCUSSION_CATEGORY, 1, parts, std::size(parts));
    }

    void onChangeDiscussionCategory(ObserverContext context, const DiscussionCategory& category,
                                    DiscussionCategory::ChangeType change)
    {
        switch (change)
        {
        case DiscussionCategory::Name:
            onChangeDiscussionCategoryName(context, category);
            break;
        case DiscussionCategory::Description:
            onChangeDiscussionCategoryDescription(context, category);
            break;
        case DiscussionCategory::DisplayOrder:
            onChangeDiscussionCategoryDisplayOrder(context, category);
            break;
        case DiscussionCategory::Parent:
            onChangeDiscussionCategoryParent(context, category);
            break;
        default:
            break;
        }
    }

    void onChangeDiscussionCategoryName(ObserverContext context, const DiscussionCategory& category)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        auto categoryName = category.name().string();
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&category.id().value().data), UuidSize, false },
            { POINTER(categoryName.data()), static_cast<SizeType>(categoryName.size()), true }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_CATEGORY_NAME, 1, parts, std::size(parts));
    }

    void onChangeDiscussionCategoryDescription(ObserverContext context, const DiscussionCategory& category)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&category.id().value().data), UuidSize, false },
            { POINTER(category.description().data()), static_cast<SizeType>(category.description().size()), true }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_CATEGORY_DESCRIPTION, 1, parts, std::size(parts));
    }

    void onChangeDiscussionCategoryDisplayOrder(ObserverContext context, const DiscussionCategory& category)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        int16_t displayOrder = category.displayOrder();
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&category.id().value().data), UuidSize, false },
            { POINTER(&displayOrder), sizeof(displayOrder), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER, 1, parts, std::size(parts));
    }

    void onChangeDiscussionCategoryParent(ObserverContext context, const DiscussionCategory& category)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        UuidString parentCategoryId = {};
        if (category.parent())
        {
            parentCategoryId = category.parent()->id();
        }

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&category.id().value().data), UuidSize, false },
            { POINTER(&parentCategoryId.value().data), UuidSize, false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_CATEGORY_PARENT, 1, parts, std::size(parts));
    }

    void onDeleteDiscussionCategory(ObserverContext context, const DiscussionCategory& category)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&category.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::DELETE_DISCUSSION_CATEGORY, 1, parts, std::size(parts));
    }

    void onAddDiscussionTagToCategory(ObserverContext context, const DiscussionTag& tag,
                                      const DiscussionCategory& category)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&tag.id().value().data), UuidSize, false },
            { POINTER(&category.id().value().data), UuidSize, false },
        };

        recordBlob(EventType::ADD_DISCUSSION_TAG_TO_CATEGORY, 1, parts, std::size(parts));
    }

    void onRemoveDiscussionTagFromCategory(ObserverContext context, const DiscussionTag& tag,
                                           const DiscussionCategory& category)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&tag.id().value().data), UuidSize, false },
            { POINTER(&category.id().value().data), UuidSize, false },
        };

        recordBlob(EventType::REMOVE_DISCUSSION_TAG_FROM_CATEGORY, 1, parts, std::size(parts));
    }

    void onGetDiscussionThreadById(ObserverContext context, const DiscussionThread& thread)
    {
        std::lock_guard<decltype(threadVisitedMutex)> lock(threadVisitedMutex);

        auto& id = thread.id();
        auto it = cachedNrOfThreadVisits.find(id);
        if (it != cachedNrOfThreadVisits.end())
        {
            ++it->second;
        }
        else
        {
            cachedNrOfThreadVisits.insert(std::make_pair(id, 1));
        }
    }

    void changeDiscussionThreadMessageRequiredPrivilegeForThreadMessage(ObserverContext context,
                                                                        const DiscussionThreadMessage& message,
                                                                        DiscussionThreadMessagePrivilege privilege,
                                                                        PrivilegeValueIntType value)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&message.id().value().data), UuidSize, false },
            { POINTER(&currentPrivilege), sizeof(currentPrivilege), false },
            { POINTER(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_THREAD_MESSAGE, 1, parts, std::size(parts));
    }

    void changeDiscussionThreadMessageRequiredPrivilegeForThread(ObserverContext context,
                                                                 const DiscussionThread& thread,
                                                                 DiscussionThreadMessagePrivilege privilege,
                                                                 PrivilegeValueIntType value)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&thread.id().value().data), UuidSize, false },
            { POINTER(&currentPrivilege), sizeof(currentPrivilege), false },
            { POINTER(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_THREAD, 1, parts, std::size(parts));
    }

    void changeDiscussionThreadMessageRequiredPrivilegeForTag(ObserverContext context,
                                                              const DiscussionTag& tag,
                                                              DiscussionThreadMessagePrivilege privilege,
                                                              PrivilegeValueIntType value)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&tag.id().value().data), UuidSize, false },
            { POINTER(&currentPrivilege), sizeof(currentPrivilege), false },
            { POINTER(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_TAG, 1, parts, std::size(parts));
    }

    void changeDiscussionThreadMessageRequiredPrivilegeForumWide(ObserverContext context,
                                                                 DiscussionThreadMessagePrivilege privilege,
                                                                 PrivilegeValueIntType value)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&currentPrivilege), sizeof(currentPrivilege), false },
            { POINTER(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FORUM_WIDE, 1, parts, std::size(parts));
    }

    void changeDiscussionThreadRequiredPrivilegeForThread(ObserverContext context,
                                                          const DiscussionThread& thread,
                                                          DiscussionThreadPrivilege privilege,
                                                          PrivilegeValueIntType value)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&thread.id().value().data), UuidSize, false },
            { POINTER(&currentPrivilege), sizeof(currentPrivilege), false },
            { POINTER(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE_FOR_THREAD, 1, parts, std::size(parts));
    }

    void changeDiscussionThreadRequiredPrivilegeForTag(ObserverContext context,
                                                       const DiscussionTag& tag,
                                                       DiscussionThreadPrivilege privilege,
                                                       PrivilegeValueIntType value)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&tag.id().value().data), UuidSize, false },
            { POINTER(&currentPrivilege), sizeof(currentPrivilege), false },
            { POINTER(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE_FOR_TAG, 1, parts, std::size(parts));
    }

    void changeDiscussionThreadRequiredPrivilegeForumWide(ObserverContext context,
                                                          DiscussionThreadPrivilege privilege,
                                                          PrivilegeValueIntType value)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&currentPrivilege), sizeof(currentPrivilege), false },
            { POINTER(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE_FORUM_WIDE, 1, parts, std::size(parts));
    }

    void changeDiscussionTagRequiredPrivilegeForTag(ObserverContext context,
                                                    const DiscussionTag& tag,
                                                    DiscussionTagPrivilege privilege,
                                                    PrivilegeValueIntType value)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&tag.id().value().data), UuidSize, false },
            { POINTER(&currentPrivilege), sizeof(currentPrivilege), false },
            { POINTER(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_TAG_REQUIRED_PRIVILEGE_FOR_TAG, 1, parts, std::size(parts));
    }

    void changeDiscussionTagRequiredPrivilegeForumWide(ObserverContext context,
                                                       DiscussionTagPrivilege privilege,
                                                       PrivilegeValueIntType value)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&currentPrivilege), sizeof(currentPrivilege), false },
            { POINTER(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_TAG_REQUIRED_PRIVILEGE_FORUM_WIDE, 1, parts, std::size(parts));
    }

    void changeDiscussionCategoryRequiredPrivilegeForCategory(ObserverContext context,
                                                              const DiscussionCategory& category,
                                                              DiscussionCategoryPrivilege privilege,
                                                              PrivilegeValueIntType value)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&category.id().value().data), UuidSize, false },
            { POINTER(&currentPrivilege), sizeof(currentPrivilege), false },
            { POINTER(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_CATEGORY_REQUIRED_PRIVILEGE_FOR_CATEGORY, 1, parts, std::size(parts));
    }

    void changeDiscussionCategoryRequiredPrivilegeForumWide(ObserverContext context,
                                                            DiscussionCategoryPrivilege privilege,
                                                            PrivilegeValueIntType value)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&currentPrivilege), sizeof(currentPrivilege), false },
            { POINTER(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_CATEGORY_REQUIRED_PRIVILEGE_FORUM_WIDE, 1, parts, std::size(parts));
    }

    void changeForumWideRequiredPrivilege(ObserverContext context, ForumWidePrivilege privilege,
                                          PrivilegeValueIntType value)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&currentPrivilege), sizeof(currentPrivilege), false },
            { POINTER(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_FORUM_WIDE_REQUIRED_PRIVILEGE, 1, parts, std::size(parts));
    }

    void changeForumWideDefaultPrivilegeLevel(ObserverContext context,
                                              ForumWideDefaultPrivilegeDuration privilegeDuration,
                                              PrivilegeValueIntType value, PrivilegeDurationIntType duration)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilegeLevel = static_cast<PersistentPrivilegeEnumType>(privilegeDuration);
        PersistentPrivilegeValueType currentValue = static_cast<PersistentPrivilegeDurationType>(value);
        PersistentPrivilegeDurationType currentDuration = static_cast<PersistentPrivilegeDurationType>(duration);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&currentPrivilegeLevel), sizeof(currentPrivilegeLevel), false },
            { POINTER(&currentValue), sizeof(currentValue), false },
            { POINTER(&currentDuration), sizeof(currentDuration), false }
        };

        recordBlob(EventType::CHANGE_FORUM_WIDE_DEFAULT_PRIVILEGE_LEVEL, 1, parts, std::size(parts));
    }

    void assignDiscussionThreadMessagePrivilege(ObserverContext context,
                                                const DiscussionThreadMessage& message,
                                                const User& user,
                                                PrivilegeValueIntType value,
                                                PrivilegeDurationIntType duration)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);
        PersistentPrivilegeDurationType currentDurationValue = static_cast<PersistentPrivilegeDurationType>(duration);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&message.id().value().data), UuidSize, false },
            { POINTER(&user.id().value().data), UuidSize, false },
            { POINTER(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false },
            { POINTER(&currentDurationValue), sizeof(currentDurationValue), false }
        };

        recordBlob(EventType::ASSIGN_DISCUSSION_THREAD_MESSAGE_PRIVILEGE, 1, parts, std::size(parts));
    }

    void assignDiscussionThreadPrivilege(ObserverContext context, const DiscussionThread& thread, const User& user,
                                         PrivilegeValueIntType value, PrivilegeDurationIntType duration)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);
        PersistentPrivilegeDurationType currentDurationValue = static_cast<PersistentPrivilegeDurationType>(duration);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&thread.id().value().data), UuidSize, false },
            { POINTER(&user.id().value().data), UuidSize, false },
            { POINTER(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false },
            { POINTER(&currentDurationValue), sizeof(currentDurationValue), false }
        };

        recordBlob(EventType::ASSIGN_DISCUSSION_THREAD_PRIVILEGE, 1, parts, std::size(parts));
    }

    void assignDiscussionTagPrivilege(ObserverContext context, const DiscussionTag& tag, const User& user,
                                      PrivilegeValueIntType value, PrivilegeDurationIntType duration)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);
        PersistentPrivilegeDurationType currentDurationValue = static_cast<PersistentPrivilegeDurationType>(duration);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&tag.id().value().data), UuidSize, false },
            { POINTER(&user.id().value().data), UuidSize, false },
            { POINTER(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false },
            { POINTER(&currentDurationValue), sizeof(currentDurationValue), false }
        };

        recordBlob(EventType::ASSIGN_DISCUSSION_TAG_PRIVILEGE, 1, parts, std::size(parts));
    }

    void assignDiscussionCategoryPrivilege(ObserverContext context, const DiscussionCategory& category,
                                           const User& user, PrivilegeValueIntType value,
                                           PrivilegeDurationIntType duration)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);
        PersistentPrivilegeDurationType currentDurationValue = static_cast<PersistentPrivilegeDurationType>(duration);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&category.id().value().data), UuidSize, false },
            { POINTER(&user.id().value().data), UuidSize, false },
            { POINTER(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false },
            { POINTER(&currentDurationValue), sizeof(currentDurationValue), false }
        };

        recordBlob(EventType::ASSIGN_DISCUSSION_CATEGORY_PRIVILEGE, 1, parts, std::size(parts));
    }

    void assignForumWidePrivilege(ObserverContext context, const User& user, PrivilegeValueIntType value,
                                  PrivilegeDurationIntType duration)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);
        PersistentPrivilegeDurationType currentDurationValue = static_cast<PersistentPrivilegeDurationType>(duration);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { POINTER(&user.id().value().data), UuidSize, false },
            { POINTER(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false },
            { POINTER(&currentDurationValue), sizeof(currentDurationValue), false }
        };

        recordBlob(EventType::ASSIGN_FORUM_WIDE_PRIVILEGE, 1, parts, std::size(parts));
    }

    void updateThreadVisited()
    {
        std::lock_guard<decltype(threadVisitedMutex)> lock(threadVisitedMutex);

        for (auto& pair : cachedNrOfThreadVisits)
        {
            BlobPart parts[] =
            {
                ADD_EMPTY_CONTEXT_BLOB_PARTS,
                { POINTER(&pair.first.value().data), UuidSize, false },
                { POINTER(&pair.second), sizeof(pair.second), false },
            };

            recordBlob(EventType::INCREMENT_DISCUSSION_THREAD_NUMBER_OF_VISITS, 1, parts, std::size(parts));
        }
        cachedNrOfThreadVisits.clear();
    }

    ReadEvents& readEvents;
    WriteEvents& writeEvents;
    std::vector<boost::signals2::connection> connections;
    EventCollector collector;
    std::thread timerThread;
    std::atomic_bool stopTimerThread{ false };
    static const std::chrono::seconds timerThreadCheckEverySeconds;
    static constexpr uint32_t updateThreadVisitedEveryIncrement = 30;
    uint32_t timerThreadCurrentIncrement = 0;
    std::mutex threadVisitedMutex;
    std::map<IdType, uint32_t> cachedNrOfThreadVisits;
};

const PersistentTimestampType EventObserver::EventObserverImpl::ZeroTimestamp{ 0 };
const IpAddress EventObserver::EventObserverImpl::ZeroIpAddress{};
const std::chrono::seconds EventObserver::EventObserverImpl::timerThreadCheckEverySeconds{ 1 };


EventObserver::EventObserver(ReadEvents& readEvents, WriteEvents& writeEvents,
                             const std::filesystem::path& destinationFolder, time_t refreshEverySeconds)
    : impl_(new EventObserverImpl(readEvents, writeEvents, destinationFolder, refreshEverySeconds))
{
}

EventObserver::~EventObserver()
{
    delete impl_;
}
