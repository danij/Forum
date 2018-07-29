#include "ForumSearchUpdatePlugin.h"
#include "EntitySerialization.h"
#include "Logging.h"
#include "Version.h"

#include <boost/format.hpp>

#include <algorithm>
#include <filesystem>
#include <memory>

using namespace Forum::Extensibility;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;
using namespace Json;

using FilePtr = std::unique_ptr<FILE, decltype(&fclose)>;

ForumSearchUpdatePlugin::ForumSearchUpdatePlugin(PluginInput& input) :
    SeparateThreadConsumer<ForumSearchUpdatePlugin, SeparateThreadConsumerBlob>{ std::chrono::milliseconds(5000) },
    writeEvents_{ *input.writeEvents },
    destinationFileTemplate_{ input.configuration->get<std::string>("outputFileNameTemplate") },
    refreshEverySeconds_{ input.configuration->get<time_t>("createNewOutputFileEverySeconds") }
{
    registerEvents();
}

StringView ForumSearchUpdatePlugin::name() const noexcept
{
    return "Search Update";
}

StringView ForumSearchUpdatePlugin::version() const noexcept
{
    return Forum::VERSION;
}

void ForumSearchUpdatePlugin::stop()
{
    stopConsumer();

    writeEvents_.onAddNewDiscussionThread.disconnect(onAddNewDiscussionThreadConnection_);
    writeEvents_.onChangeDiscussionThread.disconnect(onChangeDiscussionThreadConnection_);
    writeEvents_.onDeleteDiscussionThread.disconnect(onDeleteDiscussionThreadConnection_);
    writeEvents_.onMergeDiscussionThreads.disconnect(onMergeDiscussionThreadsConnection_);

    writeEvents_.onAddNewDiscussionThreadMessage.disconnect(onAddNewDiscussionThreadMessageConnection_);
    writeEvents_.onChangeDiscussionThreadMessage.disconnect(onChangeDiscussionThreadMessageConnection_);
    writeEvents_.onDeleteDiscussionThreadMessage.disconnect(onDeleteDiscussionThreadMessageConnection_);
}

void ForumSearchUpdatePlugin::onThreadWaitNoValues()
{
    //send a reminder so that files are closed even if no other activity occurs
    enqueue({});
}

void ForumSearchUpdatePlugin::registerEvents()
{
    onAddNewDiscussionThreadConnection_ = writeEvents_.onAddNewDiscussionThread.connect([this](auto _, auto& thread)
    {
        this->onAddNewDiscussionThread(thread);
    });
    onChangeDiscussionThreadConnection_ = 
        writeEvents_.onChangeDiscussionThread.connect([this](auto _, auto& thread, auto changeType)
    {
        this->onChangeDiscussionThread(thread, changeType);
    });
    onDeleteDiscussionThreadConnection_ = writeEvents_.onDeleteDiscussionThread.connect([this](auto _, auto& thread)
    {
        this->onDeleteDiscussionThread(thread);
    });
    onMergeDiscussionThreadsConnection_ =
        writeEvents_.onMergeDiscussionThreads.connect([this](auto _, auto& fromThread, auto& toThread)
    {
        this->onDeleteDiscussionThread(fromThread);
    });

    onAddNewDiscussionThreadMessageConnection_ =
            writeEvents_.onAddNewDiscussionThreadMessage.connect([this](auto _, auto& message)
    {
        this->onAddNewDiscussionThreadMessage(message);
    });
    onChangeDiscussionThreadMessageConnection_ =
        writeEvents_.onChangeDiscussionThreadMessage.connect([this](auto _, auto& message, auto changeType)
    {
        this->onChangeDiscussionThreadMessage(message, changeType);
    });
    onDeleteDiscussionThreadMessageConnection_ =
        writeEvents_.onDeleteDiscussionThreadMessage.connect([this](auto _, auto& message)
    {
        this->onDeleteDiscussionThreadMessage(message);
    });
}

void ForumSearchUpdatePlugin::onAddNewDiscussionThread(const DiscussionThread& thread)
{
    enqueueJson([&thread](JsonWriter& writer)
    {
        writer
            << objStart
                << propertySafeName("type", "new thread")
                << propertySafeName("id", thread.id())
                << propertySafeName("name", thread.name())
            << objEnd;
    });
}

void ForumSearchUpdatePlugin::onChangeDiscussionThread(const DiscussionThread& thread,
                                                       const DiscussionThread::ChangeType changeType)
{
    if (DiscussionThread::ChangeType::Name != changeType) return;

    enqueueJson([&thread](JsonWriter& writer)
    {
        writer
            << objStart
                << propertySafeName("type", "change thread name")
                << propertySafeName("id", thread.id())
                << propertySafeName("name", thread.name())
            << objEnd;
    });
}

void ForumSearchUpdatePlugin::onDeleteDiscussionThread(const DiscussionThread& thread)
{
    enqueueJson([&thread](JsonWriter& writer)
    {
        writer
            << objStart
                << propertySafeName("type", "delete thread")
                << propertySafeName("id", thread.id())
            << objEnd;
    });

    for (auto message : thread.messages().byId())
    {
        assert(message);
        onDeleteDiscussionThreadMessage(*message);
    }
}

void ForumSearchUpdatePlugin::onAddNewDiscussionThreadMessage(const DiscussionThreadMessage& message)
{
    enqueueJson([&message](JsonWriter& writer)
    {
        writer
            << objStart
                << propertySafeName("type", "new thread message")
                << propertySafeName("id", message.id())
                << propertySafeName("content", message.content())
            << objEnd;
    });
}

void ForumSearchUpdatePlugin::onChangeDiscussionThreadMessage(const DiscussionThreadMessage& message,
                                                              const DiscussionThreadMessage::ChangeType changeType)
{
    if (DiscussionThreadMessage::ChangeType::Content != changeType) return;

    enqueueJson([&message](JsonWriter& writer)
    {
        writer
            << objStart
                << propertySafeName("type", "change thread message content")
                << propertySafeName("id", message.id())
                << propertySafeName("content", message.content())
            << objEnd;
    });
}

void ForumSearchUpdatePlugin::onDeleteDiscussionThreadMessage(const DiscussionThreadMessage& message)
{
    enqueueJson([&message](JsonWriter& writer)
    {
        writer
            << objStart
                << propertySafeName("type", "delete thread message")
                << propertySafeName("id", message.id())
            << objEnd;
    });
}

void Forum::Extensibility::loadPlugin(PluginInput* input, PluginPtr* output)
{
    Private::setGlobalEntityCollection(input->globalEntityCollection);

    *output = std::make_shared<ForumSearchUpdatePlugin>(*input);
}

void ForumSearchUpdatePlugin::onFail(const uint32_t failNr)
{
    if (0 == failNr)
    {
        FORUM_LOG_WARNING << "ForumSearchUpdatePlugin: persistence queue is full";
    }
    std::this_thread::sleep_for(std::chrono::seconds(2000));
}

void ForumSearchUpdatePlugin::onThreadFinish()
{
    closeFile();
}

template<typename T>
static void writeOrAbort(FilePtr& file, const T* elements, const size_t nrOfElements)
{
    fwrite(elements, sizeof(T), nrOfElements, file.get());
    if (ferror(file.get()))
    {
        FORUM_LOG_ERROR << "ForumSearchUpdatePlugin: Could not write to file";
        std::abort();
    }
}

template<typename T>
static void writeOrAbort(FilePtr& file, T element)
{
    writeOrAbort(file, &element, 1);
}

void ForumSearchUpdatePlugin::consumeValues(SeparateThreadConsumerBlob* values, const size_t nrOfValues)
{
    prepareFile();

    //don't write anything if the values are all reminders
    if (std::all_of(values, values + nrOfValues, [](const SeparateThreadConsumerBlob blob) { return 0 == blob.size; }))
    {
        return;
    }

    auto file = FilePtr(fopen(currentFileName_.c_str(), "ab"), fclose);
    if ( ! file)
    {
        FORUM_LOG_ERROR << "Could not open file for writing: " << currentFileName_;
        std::abort();
    }

    for (size_t i = 0; i < nrOfValues; ++i)
    {
        auto blob = values[i];
        if (0 == blob.size) continue;

        if (0 == elementsWritten_++)
        {
            writeOrAbort(file, '[');
        }
        else
        {
            writeOrAbort(file, ',');
        }
        writeOrAbort(file, blob.buffer, blob.size);

        SeparateThreadConsumerBlob::free(blob);
    }
}

void ForumSearchUpdatePlugin::prepareFile()
{
    const auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    if ((lastFileCreatedAt_ + refreshEverySeconds_) < now)
    {
        lastFileCreatedAt_ = now;

        closeFile();

        currentFileName_ = boost::str(boost::format(destinationFileTemplate_) % now);
        elementsWritten_ = 0;
    }
}

void ForumSearchUpdatePlugin::closeFile()
{
    if (currentFileName_.empty() || (0 == elementsWritten_)) return;

    auto file = FilePtr(fopen(currentFileName_.c_str(), "ab"), fclose);
    if ( ! file)
    {
        FORUM_LOG_ERROR << "Could not open file for writing: " << currentFileName_;
        std::abort();
    }
    writeOrAbort(file, ']');

    currentFileName_ = "";
}
