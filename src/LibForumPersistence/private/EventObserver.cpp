#include "EventObserver.h"
#include "PersistenceBlob.h"
#include "PersistenceFormat.h"
#include "FileAppender.h"
#include "TypeHelpers.h"
#include "Logging.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <numeric>
#include <thread>
#include <type_traits>
#include <vector>
#include <cstring>

#include <boost/lockfree/queue.hpp>

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

class EventCollector final : private boost::noncopyable
{
public:
    EventCollector(const boost::filesystem::path& destinationFolder, time_t refreshEverySeconds)
        : appender_(destinationFolder, refreshEverySeconds)
    {
        writeThread_ = std::thread([this]() { this->writeLoop(); });
    }

    ~EventCollector()
    {
        stopWriteThread_ = true;
        blobInQueueCondition_.notify_one();
        writeThread_.join();
    }

    void addToQueue(const Blob& blob)
    {
        bool loggedWarning = false;
        while ( ! queue_.bounded_push(blob))
        {
            if ( ! loggedWarning)
            {
                FORUM_LOG_WARNING << "Persistence queue is full";
                loggedWarning = true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(400));
        }
        blobInQueueCondition_.notify_one();
    }

private:

    void writeLoop()
    {
        while ( ! stopWriteThread_)
        {
            std::unique_lock<decltype(conditionMutex_)> lock(conditionMutex_);
            blobInQueueCondition_.wait(lock, [this]() { return ! queue_.empty() || stopWriteThread_; });
            writeBlobsInQueue();
        }
        writeBlobsInQueue();
    }

    void writeBlobsInQueue()
    {
        static thread_local Blob blobsToWrite[MaxBlobsInQueue];
        Blob* blobs = blobsToWrite;

        size_t nrOfBlobsToWrite = 0;
        queue_.consume_all([&blobs, &nrOfBlobsToWrite](Blob blob)
        {
            blobsToWrite[nrOfBlobsToWrite++] = blob;
        });

        appender_.append(blobsToWrite, nrOfBlobsToWrite);

        for (size_t i = 0; i < nrOfBlobsToWrite; ++i)
        {
            Blob::free(blobsToWrite[i]);
        }
    }

    FileAppender appender_;
    static constexpr size_t MaxBlobsInQueue = 32768;
    boost::lockfree::queue<Blob, boost::lockfree::capacity<MaxBlobsInQueue>> queue_;
    std::thread writeThread_;
    std::atomic_bool stopWriteThread_{ false };
    std::condition_variable blobInQueueCondition_;
    std::mutex conditionMutex_;
};

struct EventObserver::EventObserverImpl final : private boost::noncopyable
{
    EventObserverImpl(ReadEvents& readEvents, WriteEvents& writeEvents,
                      const boost::filesystem::path& destinationFolder, time_t refreshEverySeconds)
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

    void recordBlob(EventType eventType, EventVersionType version, BlobPart* parts, size_t nrOfParts)
    {
        auto totalSize = std::accumulate(parts, parts + nrOfParts, 0, [](uint32_t total, BlobPart& part)
        {
           return total + part.totalSize();
        }) + EventHeaderSize;

        auto blob = Blob(totalSize);

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

        collector.addToQueue(blob);
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
        connections.push_back(writeEvents.changeDiscussionThreadMessageRequiredPrivilegeForThreadMessage.connect([this](auto context, auto& message, auto privilege, auto value)                             { this->changeDiscussionThreadMessageRequiredPrivilegeForThreadMessage(context, message, privilege, value); }));
        connections.push_back(writeEvents.       changeDiscussionThreadMessageRequiredPrivilegeForThread.connect([this](auto context, auto& thread, auto privilege, auto value)                              { this->changeDiscussionThreadMessageRequiredPrivilegeForThread       (context, thread, privilege, value); }));
        connections.push_back(writeEvents.          changeDiscussionThreadMessageRequiredPrivilegeForTag.connect([this](auto context, auto& tag, auto privilege, auto value)                                 { this->changeDiscussionThreadMessageRequiredPrivilegeForTag          (context, tag, privilege, value); }));
        connections.push_back(writeEvents.       changeDiscussionThreadMessageRequiredPrivilegeForumWide.connect([this](auto context, auto privilege, auto value)                                            { this->changeDiscussionThreadMessageRequiredPrivilegeForumWide       (context, privilege, value); }));
        connections.push_back(writeEvents.              changeDiscussionThreadRequiredPrivilegeForThread.connect([this](auto context, auto& thread, auto privilege, auto value)                              { this->changeDiscussionThreadRequiredPrivilegeForThread              (context, thread, privilege, value); }));
        connections.push_back(writeEvents.                 changeDiscussionThreadRequiredPrivilegeForTag.connect([this](auto context, auto& tag, auto privilege, auto value)                                 { this->changeDiscussionThreadRequiredPrivilegeForTag                 (context, tag, privilege, value); }));
        connections.push_back(writeEvents.              changeDiscussionThreadRequiredPrivilegeForumWide.connect([this](auto context, auto privilege, auto value)                                            { this->changeDiscussionThreadRequiredPrivilegeForumWide              (context, privilege, value); }));
        connections.push_back(writeEvents.                    changeDiscussionTagRequiredPrivilegeForTag.connect([this](auto context, auto& tag, auto privilege, auto value)                                 { this->changeDiscussionTagRequiredPrivilegeForTag                    (context, tag, privilege, value); }));
        connections.push_back(writeEvents.                 changeDiscussionTagRequiredPrivilegeForumWide.connect([this](auto context, auto privilege, auto value)                                            { this->changeDiscussionTagRequiredPrivilegeForumWide                 (context, privilege, value); }));
        connections.push_back(writeEvents.          changeDiscussionCategoryRequiredPrivilegeForCategory.connect([this](auto context, auto& category, auto privilege, auto value)                            { this->changeDiscussionCategoryRequiredPrivilegeForCategory          (context, category, privilege, value); }));
        connections.push_back(writeEvents.            changeDiscussionCategoryRequiredPrivilegeForumWide.connect([this](auto context, auto privilege, auto value)                                            { this->changeDiscussionCategoryRequiredPrivilegeForumWide            (context, privilege, value); }));
        connections.push_back(writeEvents.                              changeForumWideRequiredPrivilege.connect([this](auto context, auto privilege, auto value)                                            { this->changeForumWideRequiredPrivilege                              (context, privilege, value); }));
        connections.push_back(writeEvents.changeDiscussionThreadMessageDefaultPrivilegeDurationForThread.connect([this](auto context, auto& thread, auto privilegeDuration, auto duration)                   { this->changeDiscussionThreadMessageDefaultPrivilegeDurationForThread(context, thread, privilegeDuration, duration); }));
        connections.push_back(writeEvents.   changeDiscussionThreadMessageDefaultPrivilegeDurationForTag.connect([this](auto context, auto& tag, auto privilegeDuration, auto duration)                      { this->changeDiscussionThreadMessageDefaultPrivilegeDurationForTag   (context, tag, privilegeDuration, duration); }));
        connections.push_back(writeEvents.changeDiscussionThreadMessageDefaultPrivilegeDurationForumWide.connect([this](auto context, auto privilegeDuration, auto duration)                                 { this->changeDiscussionThreadMessageDefaultPrivilegeDurationForumWide(context, privilegeDuration, duration); }));
        connections.push_back(writeEvents.                       changeForumWideDefaultPrivilegeDuration.connect([this](auto context, auto privilegeDuration, auto duration)                                 { this->changeForumWideDefaultPrivilegeDuration                       (context, privilegeDuration, duration); }));
        connections.push_back(writeEvents.        assignDiscussionThreadMessagePrivilegeForThreadMessage.connect([this](auto context, auto& message, auto& user, auto privilege, auto value, auto duration)  { this->assignDiscussionThreadMessagePrivilegeForThreadMessage        (context, message, user, privilege, value, duration); }));
        connections.push_back(writeEvents.               assignDiscussionThreadMessagePrivilegeForThread.connect([this](auto context, auto& thread, auto& user, auto privilege, auto value, auto duration)   { this->assignDiscussionThreadMessagePrivilegeForThread               (context, thread, user, privilege, value, duration); }));
        connections.push_back(writeEvents.                  assignDiscussionThreadMessagePrivilegeForTag.connect([this](auto context, auto& tag, auto& user, auto privilege, auto value, auto duration)      { this->assignDiscussionThreadMessagePrivilegeForTag                  (context, tag, user, privilege, value, duration); }));
        connections.push_back(writeEvents.               assignDiscussionThreadMessagePrivilegeForumWide.connect([this](auto context, auto& user, auto privilege, auto value, auto duration)                 { this->assignDiscussionThreadMessagePrivilegeForumWide               (context, user, privilege, value, duration); }));
        connections.push_back(writeEvents.                      assignDiscussionThreadPrivilegeForThread.connect([this](auto context, auto& thread, auto& user, auto privilege, auto value, auto duration)   { this->assignDiscussionThreadPrivilegeForThread                      (context, thread, user, privilege, value, duration); }));
        connections.push_back(writeEvents.                         assignDiscussionThreadPrivilegeForTag.connect([this](auto context, auto& tag, auto& user, auto privilege, auto value, auto duration)      { this->assignDiscussionThreadPrivilegeForTag                         (context, tag, user, privilege, value, duration); }));
        connections.push_back(writeEvents.                      assignDiscussionThreadPrivilegeForumWide.connect([this](auto context, auto& user, auto privilege, auto value, auto duration)                 { this->assignDiscussionThreadPrivilegeForumWide                      (context, user, privilege, value, duration); }));
        connections.push_back(writeEvents.                            assignDiscussionTagPrivilegeForTag.connect([this](auto context, auto& tag, auto& user, auto privilege, auto value, auto duration)      { this->assignDiscussionTagPrivilegeForTag                            (context, tag, user, privilege, value, duration); }));
        connections.push_back(writeEvents.                         assignDiscussionTagPrivilegeForumWide.connect([this](auto context, auto& user, auto privilege, auto value, auto duration)                 { this->assignDiscussionTagPrivilegeForumWide                         (context, user, privilege, value, duration); }));
        connections.push_back(writeEvents.                  assignDiscussionCategoryPrivilegeForCategory.connect([this](auto context, auto& category, auto& user, auto privilege, auto value, auto duration) { this->assignDiscussionCategoryPrivilegeForCategory                  (context, category, user, privilege, value, duration); }));
        connections.push_back(writeEvents.                    assignDiscussionCategoryPrivilegeForumWide.connect([this](auto context, auto& user, auto privilege, auto value, auto duration)                 { this->assignDiscussionCategoryPrivilegeForumWide                    (context, user, privilege, value, duration); }));
        connections.push_back(writeEvents.                                      assignForumWidePrivilege.connect([this](auto context, auto& user, auto privilege, auto value, auto duration)                 { this->assignForumWidePrivilege                                      (context, user, privilege, value, duration); }));
    }

    static constexpr size_t UuidSize = boost::uuids::uuid::static_size();
    static const PersistentTimestampType ZeroTimestamp;
    static const Helpers::IpAddress ZeroIpAddress;

    typedef BlobSizeType SizeType;

#define ADD_CONTEXT_BLOB_PARTS \
    { reinterpret_cast<const char*>(&contextTimestamp), sizeof(contextTimestamp), false }, \
    { reinterpret_cast<const char*>(&context.performedBy.id().value().data), UuidSize, false }, \
    { reinterpret_cast<const char*>(context.ipAddress.data()), static_cast<SizeType>(context.ipAddress.dataSize()), false }

#define ADD_EMPTY_CONTEXT_BLOB_PARTS \
    { reinterpret_cast<const char*>(&ZeroTimestamp), sizeof(ZeroTimestamp), false }, \
    { reinterpret_cast<const char*>(&UuidString::empty.value().data), UuidSize, false }, \
    { reinterpret_cast<const char*>(ZeroIpAddress.data()), static_cast<SizeType>(ZeroIpAddress.dataSize()), false }

    void onAddNewUser(ObserverContext context, const User& user)
    {
        PersistentTimestampType contextTimestamp = context.timestamp; //make sure the size is fixed for serialization
        auto userName = user.name().string();

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&user.id().value().data), UuidSize, false }, \
            { reinterpret_cast<const char*>(user.auth().data()), static_cast<SizeType>(user.auth().size()), true }, \
            { reinterpret_cast<const char*>(userName.data()), static_cast<SizeType>(userName.size()), true }, \
        };

        recordBlob(EventType::ADD_NEW_USER, 1, parts, std::extent<decltype(parts)>::value);
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
            { reinterpret_cast<const char*>(&user.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(userName.data()), static_cast<SizeType>(userName.size()), true }
        };

        recordBlob(EventType::CHANGE_USER_NAME, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onChangeUserInfo(ObserverContext context, const User& user)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        auto userInfo = user.info().string();

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&user.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(userInfo.data()), static_cast<SizeType>(userInfo.size()), true }
        };

        recordBlob(EventType::CHANGE_USER_INFO, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onChangeUserTitle(ObserverContext context, const User& user)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        auto userTitle = user.title().string();

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&user.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(userTitle.data()), static_cast<SizeType>(userTitle.size()), true }
        };

        recordBlob(EventType::CHANGE_USER_TITLE, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onDeleteUser(ObserverContext context, const User& user)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&user.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::DELETE_USER, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onAddNewDiscussionThread(ObserverContext context, const DiscussionThread& thread)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        auto threadName = thread.name().string();

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&thread.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(threadName.data()), static_cast<SizeType>(threadName.size()), true }
        };

        recordBlob(EventType::ADD_NEW_DISCUSSION_THREAD, 1, parts, std::extent<decltype(parts)>::value);
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
            { reinterpret_cast<const char*>(&thread.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(threadName.data()), static_cast<SizeType>(threadName.size()), true }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_NAME, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onChangeDiscussionThreadPinDisplayOrder(ObserverContext context, const DiscussionThread& thread)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&thread.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(thread.pinDisplayOrder()), static_cast<SizeType>(sizeof(uint16_t)), true }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_PIN_DISPLAY_ORDER, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onDeleteDiscussionThread(ObserverContext context, const DiscussionThread& thread)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&thread.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::DELETE_DISCUSSION_THREAD, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onMergeDiscussionThreads(ObserverContext context, const DiscussionThread& fromThread,
                                  const DiscussionThread& toThread)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&fromThread.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&toThread.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::MERGE_DISCUSSION_THREADS, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onMoveDiscussionThreadMessage(ObserverContext context, const DiscussionThreadMessage& message,
                                       const DiscussionThread& intoThread)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&message.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&intoThread.id().value().data), UuidSize, false },
        };

        recordBlob(EventType::MOVE_DISCUSSION_THREAD_MESSAGE, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onSubscribeToDiscussionThread(ObserverContext context, const DiscussionThread& thread)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&thread.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::SUBSCRIBE_TO_DISCUSSION_THREAD, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onUnsubscribeFromDiscussionThread(ObserverContext context, const DiscussionThread& thread)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&thread.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::UNSUBSCRIBE_FROM_DISCUSSION_THREAD, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onAddNewDiscussionThreadMessage(ObserverContext context, const DiscussionThreadMessage& message)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        assert(message.parentThread());
        auto parentThreadId = message.parentThread()->id();

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&message.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&parentThreadId.value().data), UuidSize, false },
            { reinterpret_cast<const char*>(message.content().data()), static_cast<SizeType>(message.content().size()), true }
        };

        recordBlob(EventType::ADD_NEW_DISCUSSION_THREAD_MESSAGE, 1, parts, std::extent<decltype(parts)>::value);
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
            { reinterpret_cast<const char*>(&message.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(message.content().data()), static_cast<SizeType>(message.content().size()), true },
            { reinterpret_cast<const char*>(message.lastUpdatedReason().data()), static_cast<SizeType>(message.lastUpdatedReason().size()), true }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onDeleteDiscussionThreadMessage(ObserverContext context, const DiscussionThreadMessage& message)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&message.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::DELETE_DISCUSSION_THREAD_MESSAGE, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onDiscussionThreadMessageUpVote(ObserverContext context, const DiscussionThreadMessage& message)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&message.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::DISCUSSION_THREAD_MESSAGE_UP_VOTE, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onDiscussionThreadMessageDownVote(ObserverContext context, const DiscussionThreadMessage& message)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&message.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::DISCUSSION_THREAD_MESSAGE_DOWN_VOTE, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onDiscussionThreadMessageResetVote(ObserverContext context, const DiscussionThreadMessage& message)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&message.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::DISCUSSION_THREAD_MESSAGE_RESET_VOTE, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onAddCommentToDiscussionThreadMessage(ObserverContext context, const MessageComment& comment)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        auto parentMessageId = comment.parentMessage().id();

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&comment.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&parentMessageId.value().data), UuidSize, false },
            { reinterpret_cast<const char*>(comment.content().data()), static_cast<SizeType>(comment.content().size()), true }
        };

        recordBlob(EventType::ADD_COMMENT_TO_DISCUSSION_THREAD_MESSAGE, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onSolveDiscussionThreadMessageComment(ObserverContext context, const MessageComment& comment)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&comment.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::SOLVE_DISCUSSION_THREAD_MESSAGE_COMMENT, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onAddNewDiscussionTag(ObserverContext context, const DiscussionTag& tag)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        auto tagName = tag.name().string();

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&tag.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(tagName.data()), static_cast<SizeType>(tagName.size()), true }
        };

        recordBlob(EventType::ADD_NEW_DISCUSSION_TAG, 1, parts, std::extent<decltype(parts)>::value);
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
            { reinterpret_cast<const char*>(&tag.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(tagName.data()), static_cast<SizeType>(tagName.size()), true }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_TAG_NAME, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onChangeDiscussionTagUIBlob(ObserverContext context, const DiscussionTag& tag)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&tag.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(tag.uiBlob().data()), static_cast<SizeType>(tag.uiBlob().size()), true }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_TAG_UI_BLOB, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onDeleteDiscussionTag(ObserverContext context, const DiscussionTag& tag)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&tag.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::DELETE_DISCUSSION_TAG, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onAddDiscussionTagToThread(ObserverContext context, const DiscussionTag& tag, const DiscussionThread& thread)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&tag.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&thread.id().value().data), UuidSize, false },
        };

        recordBlob(EventType::ADD_DISCUSSION_TAG_TO_THREAD, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onRemoveDiscussionTagFromThread(ObserverContext context, const DiscussionTag& tag, const DiscussionThread& thread)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&tag.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&thread.id().value().data), UuidSize, false },
        };

        recordBlob(EventType::REMOVE_DISCUSSION_TAG_FROM_THREAD, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onMergeDiscussionTags(ObserverContext context, const DiscussionTag& fromTag, const DiscussionTag& toTag)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&fromTag.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&toTag.id().value().data), UuidSize, false },
        };

        recordBlob(EventType::MERGE_DISCUSSION_TAGS, 1, parts, std::extent<decltype(parts)>::value);
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
            { reinterpret_cast<const char*>(&category.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&parentCategoryId.value().data), UuidSize, false },
            { reinterpret_cast<const char*>(categoryName.data()), static_cast<SizeType>(categoryName.size()), true }
        };

        recordBlob(EventType::ADD_NEW_DISCUSSION_CATEGORY, 1, parts, std::extent<decltype(parts)>::value);
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
            { reinterpret_cast<const char*>(&category.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(categoryName.data()), static_cast<SizeType>(categoryName.size()), true }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_CATEGORY_NAME, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onChangeDiscussionCategoryDescription(ObserverContext context, const DiscussionCategory& category)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&category.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(category.description().data()), static_cast<SizeType>(category.description().size()), true }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_CATEGORY_DESCRIPTION, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onChangeDiscussionCategoryDisplayOrder(ObserverContext context, const DiscussionCategory& category)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        int16_t displayOrder = category.displayOrder();
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&category.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&displayOrder), sizeof(displayOrder), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER, 1, parts, std::extent<decltype(parts)>::value);
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
            { reinterpret_cast<const char*>(&category.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&parentCategoryId.value().data), UuidSize, false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_CATEGORY_PARENT, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onDeleteDiscussionCategory(ObserverContext context, const DiscussionCategory& category)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&category.id().value().data), UuidSize, false }
        };

        recordBlob(EventType::DELETE_DISCUSSION_CATEGORY, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onAddDiscussionTagToCategory(ObserverContext context, const DiscussionTag& tag,
                                      const DiscussionCategory& category)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&tag.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&category.id().value().data), UuidSize, false },
        };

        recordBlob(EventType::ADD_DISCUSSION_TAG_TO_CATEGORY, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onRemoveDiscussionTagFromCategory(ObserverContext context, const DiscussionTag& tag,
                                           const DiscussionCategory& category)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&tag.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&category.id().value().data), UuidSize, false },
        };

        recordBlob(EventType::REMOVE_DISCUSSION_TAG_FROM_CATEGORY, 1, parts, std::extent<decltype(parts)>::value);
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
            { reinterpret_cast<const char*>(&message.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_THREAD_MESSAGE, 1, parts, std::extent<decltype(parts)>::value);
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
            { reinterpret_cast<const char*>(&thread.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_THREAD, 1, parts, std::extent<decltype(parts)>::value);
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
            { reinterpret_cast<const char*>(&tag.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_TAG, 1, parts, std::extent<decltype(parts)>::value);
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
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FORUM_WIDE, 1, parts, std::extent<decltype(parts)>::value);
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
            { reinterpret_cast<const char*>(&thread.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE_FOR_THREAD, 1, parts, std::extent<decltype(parts)>::value);
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
            { reinterpret_cast<const char*>(&tag.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE_FOR_TAG, 1, parts, std::extent<decltype(parts)>::value);
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
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE_FORUM_WIDE, 1, parts, std::extent<decltype(parts)>::value);
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
            { reinterpret_cast<const char*>(&tag.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_TAG_REQUIRED_PRIVILEGE_FOR_TAG, 1, parts, std::extent<decltype(parts)>::value);
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
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_TAG_REQUIRED_PRIVILEGE_FORUM_WIDE, 1, parts, std::extent<decltype(parts)>::value);
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
            { reinterpret_cast<const char*>(&category.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_CATEGORY_REQUIRED_PRIVILEGE_FOR_CATEGORY, 1, parts, std::extent<decltype(parts)>::value);
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
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_CATEGORY_REQUIRED_PRIVILEGE_FORUM_WIDE, 1, parts, std::extent<decltype(parts)>::value);
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
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false }
        };

        recordBlob(EventType::CHANGE_FORUM_WIDE_REQUIRED_PRIVILEGE, 1, parts, std::extent<decltype(parts)>::value);
    }

    void changeDiscussionThreadMessageDefaultPrivilegeDurationForThread(ObserverContext context,
                                                                        const DiscussionThread& thread,
                                                                        DiscussionThreadMessageDefaultPrivilegeDuration privilegeDuration,
                                                                        PrivilegeDefaultDurationIntType duration)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilegeDuration = static_cast<PersistentPrivilegeEnumType>(privilegeDuration);
        PersistentPrivilegeDurationType currentDurationValue = static_cast<PersistentPrivilegeDurationType>(duration);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&thread.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&currentPrivilegeDuration), sizeof(currentPrivilegeDuration), false },
            { reinterpret_cast<const char*>(&currentDurationValue), sizeof(currentDurationValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_MESSAGE_DEFAULT_PRIVILEGE_DURATION_FOR_THREAD, 1, parts, std::extent<decltype(parts)>::value);
    }

    void changeDiscussionThreadMessageDefaultPrivilegeDurationForTag(ObserverContext context,
                                                                     const DiscussionTag& tag,
                                                                     DiscussionThreadMessageDefaultPrivilegeDuration privilegeDuration,
                                                                     PrivilegeDefaultDurationIntType duration)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilegeDuration = static_cast<PersistentPrivilegeEnumType>(privilegeDuration);
        PersistentPrivilegeDurationType currentDurationValue = static_cast<PersistentPrivilegeDurationType>(duration);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&tag.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&currentPrivilegeDuration), sizeof(currentPrivilegeDuration), false },
            { reinterpret_cast<const char*>(&currentDurationValue), sizeof(currentDurationValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_MESSAGE_DEFAULT_PRIVILEGE_DURATION_FOR_TAG, 1, parts, std::extent<decltype(parts)>::value);
    }

    void changeDiscussionThreadMessageDefaultPrivilegeDurationForumWide(ObserverContext context,
                                                                        DiscussionThreadMessageDefaultPrivilegeDuration privilegeDuration,
                                                                        PrivilegeDefaultDurationIntType duration)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilegeDuration = static_cast<PersistentPrivilegeEnumType>(privilegeDuration);
        PersistentPrivilegeDurationType currentDurationValue = static_cast<PersistentPrivilegeDurationType>(duration);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&currentPrivilegeDuration), sizeof(currentPrivilegeDuration), false },
            { reinterpret_cast<const char*>(&currentDurationValue), sizeof(currentDurationValue), false }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_MESSAGE_DEFAULT_PRIVILEGE_DURATION_FORUM_WIDE, 1, parts, std::extent<decltype(parts)>::value);
    }

    void changeForumWideDefaultPrivilegeDuration(ObserverContext context,
                                                 ForumWideDefaultPrivilegeDuration privilegeDuration,
                                                 PrivilegeDefaultDurationIntType duration)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilegeDuration = static_cast<PersistentPrivilegeEnumType>(privilegeDuration);
        PersistentPrivilegeDurationType currentDurationValue = static_cast<PersistentPrivilegeDurationType>(duration);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&currentPrivilegeDuration), sizeof(currentPrivilegeDuration), false },
            { reinterpret_cast<const char*>(&currentDurationValue), sizeof(currentDurationValue), false }
        };

        recordBlob(EventType::CHANGE_FORUM_WIDE_DEFAULT_PRIVILEGE_DURATION, 1, parts, std::extent<decltype(parts)>::value);
    }

    void assignDiscussionThreadMessagePrivilegeForThreadMessage(ObserverContext context,
                                                                const DiscussionThreadMessage& message,
                                                                const User& user,
                                                                DiscussionThreadMessagePrivilege privilege,
                                                                PrivilegeValueIntType value,
                                                                PrivilegeDefaultDurationIntType duration)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);
        PersistentPrivilegeDurationType currentDurationValue = static_cast<PersistentPrivilegeDurationType>(duration);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&message.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&user.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false },
            { reinterpret_cast<const char*>(&currentDurationValue), sizeof(currentDurationValue), false }
        };

        recordBlob(EventType::ASSIGN_DISCUSSION_THREAD_MESSAGE_PRIVILEGE_FOR_THREAD_MESSAGE, 1, parts, std::extent<decltype(parts)>::value);
    }

    void assignDiscussionThreadMessagePrivilegeForThread(ObserverContext context,
                                                         const DiscussionThread& thread,
                                                         const User& user,
                                                         DiscussionThreadMessagePrivilege privilege,
                                                         PrivilegeValueIntType value,
                                                         PrivilegeDefaultDurationIntType duration)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);
        PersistentPrivilegeDurationType currentDurationValue = static_cast<PersistentPrivilegeDurationType>(duration);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&thread.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&user.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false },
            { reinterpret_cast<const char*>(&currentDurationValue), sizeof(currentDurationValue), false }
        };

        recordBlob(EventType::ASSIGN_DISCUSSION_THREAD_MESSAGE_PRIVILEGE_FOR_THREAD, 1, parts, std::extent<decltype(parts)>::value);
    }

    void assignDiscussionThreadMessagePrivilegeForTag(ObserverContext context, const DiscussionTag& tag,
                                                      const User& user, DiscussionThreadMessagePrivilege privilege,
                                                      PrivilegeValueIntType value,
                                                      PrivilegeDefaultDurationIntType duration)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);
        PersistentPrivilegeDurationType currentDurationValue = static_cast<PersistentPrivilegeDurationType>(duration);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&tag.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&user.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false },
            { reinterpret_cast<const char*>(&currentDurationValue), sizeof(currentDurationValue), false }
        };

        recordBlob(EventType::ASSIGN_DISCUSSION_THREAD_MESSAGE_PRIVILEGE_FOR_TAG, 1, parts, std::extent<decltype(parts)>::value);
    }

    void assignDiscussionThreadMessagePrivilegeForumWide(ObserverContext context, const User& user,
                                                         DiscussionThreadMessagePrivilege privilege,
                                                         PrivilegeValueIntType value,
                                                         PrivilegeDefaultDurationIntType duration)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);
        PersistentPrivilegeDurationType currentDurationValue = static_cast<PersistentPrivilegeDurationType>(duration);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&user.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false },
            { reinterpret_cast<const char*>(&currentDurationValue), sizeof(currentDurationValue), false }
        };

        recordBlob(EventType::ASSIGN_DISCUSSION_THREAD_MESSAGE_PRIVILEGE_FORUM_WIDE, 1, parts, std::extent<decltype(parts)>::value);
    }

    void assignDiscussionThreadPrivilegeForThread(ObserverContext context, const DiscussionThread& thread,
                                                  const User& user, DiscussionThreadPrivilege privilege,
                                                  PrivilegeValueIntType value, PrivilegeDefaultDurationIntType duration)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);
        PersistentPrivilegeDurationType currentDurationValue = static_cast<PersistentPrivilegeDurationType>(duration);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&thread.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&user.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false },
            { reinterpret_cast<const char*>(&currentDurationValue), sizeof(currentDurationValue), false }
        };

        recordBlob(EventType::ASSIGN_DISCUSSION_THREAD_PRIVILEGE_FOR_THREAD, 1, parts, std::extent<decltype(parts)>::value);
    }

    void assignDiscussionThreadPrivilegeForTag(ObserverContext context, const DiscussionTag& tag, const User& user,
                                               DiscussionThreadPrivilege privilege, PrivilegeValueIntType value,
                                               PrivilegeDefaultDurationIntType duration)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);
        PersistentPrivilegeDurationType currentDurationValue = static_cast<PersistentPrivilegeDurationType>(duration);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&tag.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&user.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false },
            { reinterpret_cast<const char*>(&currentDurationValue), sizeof(currentDurationValue), false }
        };

        recordBlob(EventType::ASSIGN_DISCUSSION_THREAD_PRIVILEGE_FOR_TAG, 1, parts, std::extent<decltype(parts)>::value);
    }

    void assignDiscussionThreadPrivilegeForumWide(ObserverContext context, const User& user,
                                                  DiscussionThreadPrivilege privilege, PrivilegeValueIntType value,
                                                  PrivilegeDefaultDurationIntType duration)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);
        PersistentPrivilegeDurationType currentDurationValue = static_cast<PersistentPrivilegeDurationType>(duration);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&user.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false },
            { reinterpret_cast<const char*>(&currentDurationValue), sizeof(currentDurationValue), false }
        };

        recordBlob(EventType::ASSIGN_DISCUSSION_THREAD_PRIVILEGE_FORUM_WIDE, 1, parts, std::extent<decltype(parts)>::value);
    }

    void assignDiscussionTagPrivilegeForTag(ObserverContext context, const DiscussionTag& tag, const User& user,
                                            DiscussionTagPrivilege privilege, PrivilegeValueIntType value,
                                            PrivilegeDefaultDurationIntType duration)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);
        PersistentPrivilegeDurationType currentDurationValue = static_cast<PersistentPrivilegeDurationType>(duration);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&tag.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&user.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false },
            { reinterpret_cast<const char*>(&currentDurationValue), sizeof(currentDurationValue), false }
        };

        recordBlob(EventType::ASSIGN_DISCUSSION_TAG_PRIVILEGE_FOR_TAG, 1, parts, std::extent<decltype(parts)>::value);
    }

    void assignDiscussionTagPrivilegeForumWide(ObserverContext context, const User& user,
                                               DiscussionTagPrivilege privilege, PrivilegeValueIntType value,
                                               PrivilegeDefaultDurationIntType duration)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);
        PersistentPrivilegeDurationType currentDurationValue = static_cast<PersistentPrivilegeDurationType>(duration);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&user.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false },
            { reinterpret_cast<const char*>(&currentDurationValue), sizeof(currentDurationValue), false }
        };

        recordBlob(EventType::ASSIGN_DISCUSSION_TAG_PRIVILEGE_FORUM_WIDE, 1, parts, std::extent<decltype(parts)>::value);
    }

    void assignDiscussionCategoryPrivilegeForCategory(ObserverContext context, const DiscussionCategory& category,
                                                      const User& user, DiscussionCategoryPrivilege privilege,
                                                      PrivilegeValueIntType value,
                                                      PrivilegeDefaultDurationIntType duration)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);
        PersistentPrivilegeDurationType currentDurationValue = static_cast<PersistentPrivilegeDurationType>(duration);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&category.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&user.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false },
            { reinterpret_cast<const char*>(&currentDurationValue), sizeof(currentDurationValue), false }
        };

        recordBlob(EventType::ASSIGN_DISCUSSION_CATEGORY_PRIVILEGE_FOR_CATEGORY, 1, parts, std::extent<decltype(parts)>::value);
    }

    void assignDiscussionCategoryPrivilegeForumWide(ObserverContext context, const User& user,
                                                    DiscussionCategoryPrivilege privilege, PrivilegeValueIntType value,
                                                    PrivilegeDefaultDurationIntType duration)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);
        PersistentPrivilegeDurationType currentDurationValue = static_cast<PersistentPrivilegeDurationType>(duration);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&user.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false },
            { reinterpret_cast<const char*>(&currentDurationValue), sizeof(currentDurationValue), false }
        };

        recordBlob(EventType::ASSIGN_DISCUSSION_CATEGORY_PRIVILEGE_FORUM_WIDE, 1, parts, std::extent<decltype(parts)>::value);
    }

    void assignForumWidePrivilege(ObserverContext context, const User& user, ForumWidePrivilege privilege,
                                  PrivilegeValueIntType value, PrivilegeDefaultDurationIntType duration)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        PersistentPrivilegeEnumType currentPrivilege = static_cast<PersistentPrivilegeEnumType>(privilege);
        PersistentPrivilegeValueType currentPrivilegeValue = static_cast<PersistentPrivilegeValueType>(value);
        PersistentPrivilegeDurationType currentDurationValue = static_cast<PersistentPrivilegeDurationType>(duration);

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&user.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&currentPrivilege), sizeof(currentPrivilege), false },
            { reinterpret_cast<const char*>(&currentPrivilegeValue), sizeof(currentPrivilegeValue), false },
            { reinterpret_cast<const char*>(&currentDurationValue), sizeof(currentDurationValue), false }
        };

        recordBlob(EventType::ASSIGN_FORUM_WIDE_PRIVILEGE, 1, parts, std::extent<decltype(parts)>::value);
    }

    void updateThreadVisited()
    {
        std::lock_guard<decltype(threadVisitedMutex)> lock(threadVisitedMutex);

        for (auto& pair : cachedNrOfThreadVisits)
        {
            BlobPart parts[] =
            {
                ADD_EMPTY_CONTEXT_BLOB_PARTS,
                { reinterpret_cast<const char*>(&pair.first.value().data), UuidSize, false },
                { reinterpret_cast<const char*>(&pair.second), sizeof(pair.second), false },
            };

            recordBlob(EventType::INCREMENT_DISCUSSION_THREAD_NUMBER_OF_VISITS, 1, parts, std::extent<decltype(parts)>::value);
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

const PersistentTimestampType Forum::Persistence::EventObserver::EventObserverImpl::ZeroTimestamp{ 0 };
const Helpers::IpAddress Forum::Persistence::EventObserver::EventObserverImpl::ZeroIpAddress{};
const std::chrono::seconds Forum::Persistence::EventObserver::EventObserverImpl::timerThreadCheckEverySeconds{ 1 };


EventObserver::EventObserver(ReadEvents& readEvents, WriteEvents& writeEvents,
                             const boost::filesystem::path& destinationFolder, time_t refreshEverySeconds)
    : impl_(new EventObserverImpl(readEvents, writeEvents, destinationFolder, refreshEverySeconds))
{
}

EventObserver::~EventObserver()
{
    if (impl_)
    {
        delete impl_;
    }
}
