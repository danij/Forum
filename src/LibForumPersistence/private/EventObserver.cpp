#include "EventObserver.h"
#include "PersistenceBlob.h"
#include "PersistenceFormat.h"
#include "FileAppender.h"
#include "Logging.h"

#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <numeric>
#include <thread>
#include <type_traits>
#include <vector>

#include <boost/lockfree/queue.hpp>

using namespace Forum;
using namespace Forum::Persistence;
using namespace Forum::Repository;
using namespace Forum::Entities;

struct BlobPart
{
    const char* address;
    uint32_t size;
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
    static constexpr size_t MaxBlobsInQueue = 10000;
    boost::lockfree::queue<Blob, boost::lockfree::capacity<MaxBlobsInQueue>> queue_;
    std::thread writeThread_;
    std::atomic_bool stopWriteThread_ = false;
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
        *reinterpret_cast<EventType*>(buffer) = eventType;
        buffer += sizeof(eventType);

        *reinterpret_cast<std::add_pointer<decltype(version)>::type>(buffer) = version;
        buffer += sizeof(version);

        *reinterpret_cast<std::add_pointer<std::remove_const<decltype(ContextVersion)>::type>::type>(buffer) = ContextVersion;
        buffer += sizeof(ContextVersion);

        for (size_t i = 0; i < nrOfParts; ++i)
        {
            const auto& part = parts[i];
            if (part.includeSizePrefix)
            {
                *reinterpret_cast<std::add_pointer<decltype(BlobPart::size)>::type>(buffer) = part.size;
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
        connections.push_back(writeEvents.                         onAddNewUser.connect([this](auto context, auto& user)                       { this->onAddNewUser(context, user); }));
        connections.push_back(writeEvents.                         onChangeUser.connect([this](auto context, auto& user, auto change)          { this->onChangeUser(context, user, change); }));
        connections.push_back(writeEvents.                         onDeleteUser.connect([this](auto context, auto& user)                       { this->onDeleteUser(context, user); }));
        connections.push_back(writeEvents.             onAddNewDiscussionThread.connect([this](auto context, auto& thread)                     { this->onAddNewDiscussionThread(context, thread); }));
        connections.push_back(writeEvents.             onChangeDiscussionThread.connect([this](auto context, auto& thread, auto change)        { this->onChangeDiscussionThread(context, thread, change); }));
        connections.push_back(writeEvents.             onDeleteDiscussionThread.connect([this](auto context, auto& thread)                     { this->onDeleteDiscussionThread(context, thread); }));
        connections.push_back(writeEvents.             onMergeDiscussionThreads.connect([this](auto context, auto& fromThread, auto& toThread) { this->onMergeDiscussionThreads(context, fromThread, toThread); }));
        connections.push_back(writeEvents.        onMoveDiscussionThreadMessage.connect([this](auto context, auto& message, auto& intoThread)  { this->onMoveDiscussionThreadMessage(context, message, intoThread); }));
        connections.push_back(writeEvents.        onSubscribeToDiscussionThread.connect([this](auto context, auto& thread)                     { this->onSubscribeToDiscussionThread(context, thread); }));
        connections.push_back(writeEvents.    onUnsubscribeFromDiscussionThread.connect([this](auto context, auto& thread)                     { this->onUnsubscribeFromDiscussionThread(context, thread); }));
        connections.push_back(writeEvents.      onAddNewDiscussionThreadMessage.connect([this](auto context, auto& message)                    { this->onAddNewDiscussionThreadMessage(context, message); }));
        connections.push_back(writeEvents.      onChangeDiscussionThreadMessage.connect([this](auto context, auto& message, auto change)       { this->onChangeDiscussionThreadMessage(context, message, change); }));
        connections.push_back(writeEvents.      onDeleteDiscussionThreadMessage.connect([this](auto context, auto& message)                    { this->onDeleteDiscussionThreadMessage(context, message); }));
        connections.push_back(writeEvents.      onDiscussionThreadMessageUpVote.connect([this](auto context, auto& message)                    { this->onDiscussionThreadMessageUpVote(context, message); }));
        connections.push_back(writeEvents.    onDiscussionThreadMessageDownVote.connect([this](auto context, auto& message)                    { this->onDiscussionThreadMessageDownVote(context, message); }));
        connections.push_back(writeEvents.   onDiscussionThreadMessageResetVote.connect([this](auto context, auto& message)                    { this->onDiscussionThreadMessageResetVote(context, message); }));
        connections.push_back(writeEvents.onAddCommentToDiscussionThreadMessage.connect([this](auto context, auto& comment)                    { this->onAddCommentToDiscussionThreadMessage(context, comment); }));
        connections.push_back(writeEvents.onSolveDiscussionThreadMessageComment.connect([this](auto context, auto& comment)                    { this->onSolveDiscussionThreadMessageComment(context, comment); }));
        connections.push_back(writeEvents.                onAddNewDiscussionTag.connect([this](auto context, auto& tag)                        { this->onAddNewDiscussionTag(context, tag); }));
        connections.push_back(writeEvents.                onChangeDiscussionTag.connect([this](auto context, auto& tag, auto change)           { this->onChangeDiscussionTag(context, tag, change); }));
        connections.push_back(writeEvents.                onDeleteDiscussionTag.connect([this](auto context, auto& tag)                        { this->onDeleteDiscussionTag(context, tag); }));
        connections.push_back(writeEvents.           onAddDiscussionTagToThread.connect([this](auto context, auto& tag, auto& thread)          { this->onAddDiscussionTagToThread(context, tag, thread); }));
        connections.push_back(writeEvents.      onRemoveDiscussionTagFromThread.connect([this](auto context, auto& tag, auto& thread)          { this->onRemoveDiscussionTagFromThread(context, tag, thread); }));
        connections.push_back(writeEvents.                onMergeDiscussionTags.connect([this](auto context, auto& fromTag, auto& toTag)       { this->onMergeDiscussionTags(context, fromTag, toTag); }));
        connections.push_back(writeEvents.           onAddNewDiscussionCategory.connect([this](auto context, auto& category)                   { this->onAddNewDiscussionCategory(context, category); }));
        connections.push_back(writeEvents.           onChangeDiscussionCategory.connect([this](auto context, auto& category, auto change)      { this->onChangeDiscussionCategory(context, category, change); }));
        connections.push_back(writeEvents.           onDeleteDiscussionCategory.connect([this](auto context, auto& category)                   { this->onDeleteDiscussionCategory(context, category); }));
        connections.push_back(writeEvents.         onAddDiscussionTagToCategory.connect([this](auto context, auto& tag, auto& category)        { this->onAddDiscussionTagToCategory(context, tag, category); }));
        connections.push_back(writeEvents.    onRemoveDiscussionTagFromCategory.connect([this](auto context, auto& tag, auto& category)        { this->onRemoveDiscussionTagFromCategory(context, tag, category); }));

        connections.push_back(readEvents.             onGetDiscussionThreadById.connect([this](auto context, auto& thread)                     { this->onGetDiscussionThreadById(context, thread); }));

    }

    static constexpr size_t UuidSize = boost::uuids::uuid::static_size();
    static constexpr PersistentTimestampType ZeroTimestamp = 0;
    static const Helpers::IpAddress ZeroIpAddress;

#define ADD_CONTEXT_BLOB_PARTS \
    { reinterpret_cast<const char*>(&contextTimestamp), sizeof(contextTimestamp), false }, \
    { reinterpret_cast<const char*>(&context.performedBy.id().value().data), UuidSize, false }, \
    { reinterpret_cast<const char*>(context.ipAddress.data()), context.ipAddress.dataSize(), false }

#define ADD_EMPTY_CONTEXT_BLOB_PARTS \
    { reinterpret_cast<const char*>(&ZeroTimestamp), sizeof(ZeroTimestamp), false }, \
    { reinterpret_cast<const char*>(&UuidString::empty.value().data), UuidSize, false }, \
    { reinterpret_cast<const char*>(ZeroIpAddress.data()), ZeroIpAddress.dataSize(), false }
        
    void onAddNewUser(ObserverContext context, const User& user)
    {
        PersistentTimestampType contextTimestamp = context.timestamp; //make sure the size is fixed for serialization
        BlobPart parts[] = 
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&user.id().value().data), UuidSize, false }, \
            { reinterpret_cast<const char*>(user.name().data()), user.name().size(), true }, \
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
        }
    }

    void onChangeUserName(ObserverContext context, const User& user)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&user.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(user.name().data()), user.name().size(), true }
        };

        recordBlob(EventType::CHANGE_USER_NAME, 1, parts, std::extent<decltype(parts)>::value);
    }

    void onChangeUserInfo(ObserverContext context, const User& user)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&user.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(user.info().data()), user.info().size(), true }
        };

        recordBlob(EventType::CHANGE_USER_INFO, 1, parts, std::extent<decltype(parts)>::value);
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
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&thread.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(thread.name().data()), thread.name().size(), true }
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
        }
    }

    void onChangeDiscussionThreadName(ObserverContext context, const DiscussionThread& thread)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&thread.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(thread.name().data()), thread.name().size(), true }
        };

        recordBlob(EventType::CHANGE_DISCUSSION_THREAD_NAME, 1, parts, std::extent<decltype(parts)>::value);
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
        UuidString parentThreadId = {};
        message.executeActionWithParentThreadIfAvailable([&parentThreadId](auto& thread)
        {
            parentThreadId = thread.id();
        });
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&message.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&parentThreadId.value().data), UuidSize, false },
            { reinterpret_cast<const char*>(message.content().data()), message.content().size(), true }
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
        }
    }

    void onChangeDiscussionThreadMessageContentContent(ObserverContext context, const DiscussionThreadMessage& message)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&message.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(message.content().data()), message.content().size(), true },
            { reinterpret_cast<const char*>(message.lastUpdatedReason().data()), message.lastUpdatedReason().size(), true }
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
        UuidString parentMessageId = {};
        comment.executeActionWithParentMessageIfAvailable([&parentMessageId](auto& message)
        {
            parentMessageId = message.id();
        });
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&comment.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&parentMessageId.value().data), UuidSize, false },
            { reinterpret_cast<const char*>(comment.content().data()), comment.content().size(), true }
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
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&tag.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(tag.name().data()), tag.name().size(), true }
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
        }    
    }

    void onChangeDiscussionTagName(ObserverContext context, const DiscussionTag& tag)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&tag.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(tag.name().data()), tag.name().size(), true }
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
            { reinterpret_cast<const char*>(tag.uiBlob().data()), tag.uiBlob().size(), true }
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
        category.executeActionWithParentCategoryIfAvailable([&parentCategoryId](auto& parentCategory)
        {
            parentCategoryId = parentCategory.id();
        });        

        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&category.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(&parentCategoryId.value().data), UuidSize, false },
            { reinterpret_cast<const char*>(category.name().data()), category.name().size(), true }
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
        }
    }

    void onChangeDiscussionCategoryName(ObserverContext context, const DiscussionCategory& category)
    {
        PersistentTimestampType contextTimestamp = context.timestamp;
        BlobPart parts[] =
        {
            ADD_CONTEXT_BLOB_PARTS,
            { reinterpret_cast<const char*>(&category.id().value().data), UuidSize, false },
            { reinterpret_cast<const char*>(category.name().data()), category.name().size(), true }
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
            { reinterpret_cast<const char*>(category.description().data()), category.description().size(), true }
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
        category.executeActionWithParentCategoryIfAvailable([&parentCategoryId](auto& parentCategory)
        {
            parentCategoryId = parentCategory.id();
        });

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
    }

    ReadEvents& readEvents;
    WriteEvents& writeEvents;
    std::vector<boost::signals2::connection> connections;
    EventCollector collector;
    std::thread timerThread;
    std::atomic_bool stopTimerThread = false;
    static constexpr std::chrono::seconds timerThreadCheckEverySeconds{ 1 };
    static constexpr uint32_t updateThreadVisitedEveryIncrement = 30;
    uint32_t timerThreadCurrentIncrement = 0;
    std::mutex threadVisitedMutex;
    std::map<IdType, uint32_t> cachedNrOfThreadVisits;
};

const Helpers::IpAddress EventObserver::EventObserverImpl::ZeroIpAddress{};

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
