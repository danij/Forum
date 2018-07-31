/*
Fast Forum Backend
Copyright (C) Daniel Jurcau

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

#include "PersistenceFormat.h"
#include "IpAddress.h"
#include "TypeHelpers.h"
#include "JsonWriter.h"

#include <boost/format.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/uuid/uuid.hpp>
#include <utility>

#include <iostream>
#include <filesystem>
#include <string>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <regex>
#include <set>
#include <string_view>

using namespace Forum::Persistence;
using namespace Forum::Helpers;
using namespace Json;

using IdType = boost::uuids::uuid;

class SearchDataExtractor final
{
public:
    SearchDataExtractor(std::string outputTemplate, size_t eventsPerFile);
    ~SearchDataExtractor();
    bool perform(const char* data, size_t size);

private:
    bool iterateBlobs(const char* data, size_t size, std::function<bool(const char*, size_t)>&& fn);
    bool processBlob(const char* data, size_t size);

    void writeJson(std::function<void(JsonWriter&)>&& callback);
    void closeFile();

    void onAddNewDiscussionThread(const IdType& id, std::string_view name);
    void onChangeDiscussionThreadName(const IdType& id, std::string_view newName);
    void onDeleteDiscussionThread(const IdType& id);
    void onMergeDiscussionThread(const IdType& fromId, const IdType& intoId);

    void onAddNewDiscussionThreadMessage(const IdType& id, const IdType& threadId, std::string_view content);
    void onChangeDiscussionThreadMessageContent(const IdType& id, std::string_view newContent);
    void onDeleteDiscussionThreadMessage(const IdType& id);
    
    std::string outputTemplate_;
    const size_t eventsPerFile_;
    size_t currentEventsPerFile_{};
    FILE* currentFile_{};
    size_t currentFileNr_{};
    StringBuffer outputBuffer_{ 1 << 20 };

    std::map<IdType, std::set<IdType>> threadMessages_{};
    std::set<IdType> deletedMessages_{};
};

int startExtraction(const std::string& inputFolder, const std::string& outputTemplate, size_t eventsPerFile);

int main(int argc, const char* argv[])
{
    boost::program_options::options_description options("Available options");
    options.add_options()
        ("help,h", "Display available options")
        ("input,i", boost::program_options::value<std::string>(), "Input folder")
        ("output,o", boost::program_options::value<std::string>(), "Output file template")
        ("events-per-file,e", boost::program_options::value<size_t>(), "Max number of events/file to write");

    boost::program_options::variables_map arguments;

    try
    {
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, options), arguments);
        boost::program_options::notify(arguments);
    }
    catch (std::exception& ex)
    {
        std::cerr << "Invalid command line: " << ex.what() << '\n';
        return 1;
    }

    if (arguments.count("help"))
    {
        std::cout << options << '\n';
        return 1;
    }

    if (arguments.count("input") && arguments.count("output"))
    {
        return startExtraction(arguments["input"].as<std::string>(), arguments["output"].as<std::string>(),
                               arguments["events-per-file"].as<size_t>());
    }
    else
    {
        std::cout << options << '\n';
        return 1;
    }
}

template<typename Fn>
static void iteratePathRecursively(const std::filesystem::path& sourcePath, Fn&& action)
{
    if (is_directory(sourcePath))
    {
        for (auto& entry : boost::make_iterator_range(std::filesystem::directory_iterator(sourcePath), {}))
        {
            iteratePathRecursively(entry, action);
        }
    }
    else if (is_regular_file(sourcePath))
    {
        action(sourcePath);
    }
}

std::vector<std::string> getSortedEventFileNames(const std::filesystem::path& sourcePath)
{
    std::map<time_t, std::string> eventFileNames;
    std::regex eventFileMatcher("^forum-(\\d+).events$", std::regex_constants::icase);

    iteratePathRecursively(sourcePath, [&](auto& path)
    {
        std::smatch match;
        auto fileName = path.filename().string();
        if (std::regex_match(fileName, match, eventFileMatcher))
        {
            const auto timestampString = match[1].str();
            time_t timestamp{};

            if ( ! boost::conversion::try_lexical_convert(timestampString, timestamp))
            {
                std::cerr << "Cannot convert timestamp from " << fileName;
            }
            else
            {
                auto fullName = path.string();
                eventFileNames.insert(std::make_pair(timestamp, fullName));
            }
        }
    });

    std::vector<std::string> result;

    for (auto& [_, fileName] : eventFileNames)
    {
        result.emplace_back(std::move(fileName));
    }

    return result;
}

int startExtraction(const std::string& inputFolder, const std::string& outputTemplate, const size_t eventsPerFile)
{
    SearchDataExtractor extractor(outputTemplate, eventsPerFile);

    for (const auto& fileName : getSortedEventFileNames(inputFolder))
    {
        try
        {
            const auto mappingMode = boost::interprocess::read_only;
            const boost::interprocess::file_mapping mapping(fileName.c_str(), mappingMode);
            boost::interprocess::mapped_region region(mapping, mappingMode);
            region.advise(boost::interprocess::mapped_region::advice_sequential);

            const auto ptr = reinterpret_cast<const char*>(region.get_address());
            const auto size = region.get_size();

            const auto result = extractor.perform(ptr, size);
            if ( ! result)
            {
                return 1;
            }
        }
        catch (boost::interprocess::interprocess_exception& ex)
        {
            std::cerr << "Error mapping input file: " << fileName << " (" << ex.what() << ')';
            return 1;
        }
    }
    return 0;
}

template<typename T>
T readAndIncrementBuffer(const char*& data, size_t& size)
{
    T result;
    memcpy(&result, data, sizeof(T));
    data += sizeof(T); size -= sizeof(T);

    return result;
}

SearchDataExtractor::SearchDataExtractor(std::string outputTemplate, const size_t eventsPerFile)
    : outputTemplate_(std::move(outputTemplate)), 
      eventsPerFile_(0 < eventsPerFile ? eventsPerFile : std::numeric_limits<size_t>::max())
{
}

SearchDataExtractor::~SearchDataExtractor()
{
    closeFile();
}

bool SearchDataExtractor::perform(const char* data, const size_t size)
{
    return iterateBlobs(data, size, [this](const char* data, const size_t blobSize)
    {
        return this->processBlob(data, blobSize);
    });
}

bool SearchDataExtractor::iterateBlobs(const char* data, size_t size, std::function<bool(const char*, size_t)>&& fn)
{
    while (size > 0)
    {
        if (size < MinBlobSize)
        {
            std::cerr << "Found bytes that are not enough to contain a persisted event blob";
            break;
        }

        auto magic = readAndIncrementBuffer<MagicPrefixType>(data, size);
        if (magic != MagicPrefix)
        {
            std::cerr << "Invalid prefix in current blob";
            break;
        }

        const auto blobSize = readAndIncrementBuffer<BlobSizeType>(data, size);
        const auto blobSizeWithPadding = blobSize + blobPaddingRequired(blobSize);

        auto storedChecksum = readAndIncrementBuffer<BlobChecksumSizeType>(data, size);
        (void)storedChecksum;

        if (size < blobSizeWithPadding)
        {
            std::cerr << "Not enough bytes remaining in file for a full event blob";
            break;
        }

        if (size < EventHeaderSize)
        {
            std::cerr << "Blob too small";
            return false;
        }

        const auto result = fn(data, blobSize);
        if ( ! result)
        {
            return result;
        }

        data += blobSizeWithPadding;
        size -= blobSizeWithPadding;
    }
    return true;
}

IdType parseUuid(const char* data)
{
    boost::uuids::uuid result{};
    std::copy(data, data + boost::uuids::uuid::static_size(), result.data);
    return result;
}

#define CHECK_SIZE \
    if (size < contextSize)                                                                                    \
    {                                                                                                          \
        std::cerr << "Unable to import context v1: expected " << contextSize << " bytes, found only " << size; \
        return false;                                                                                          \
    }

#define CHECK_VERSION_1 \
    if (1 != version)                                                \
    {                                                                \
        std::cerr << "Version " << version << " is not supported\n"; \
        return false;                                                \
    }

std::string_view readStringViewWithPrefix(const char* data)
{
    uint32_t size;
    readValue(data, size);
    return std::string_view(data + sizeof(size), size);
}

bool SearchDataExtractor::processBlob(const char* data, size_t size)
{
    const auto blobStart = data;
    const auto blobSize = size;

    const auto eventType = readAndIncrementBuffer<EventType>(data, size);
    const auto version = readAndIncrementBuffer<EventVersionType>(data, size);
    const auto contextVersion = readAndIncrementBuffer<EventContextVersionType>(data, size);

    static constexpr auto uuidSize = boost::uuids::uuid::static_size();

    static constexpr auto eventHeaderSize = sizeof(EventType) + sizeof(EventVersionType) + sizeof(EventContextVersionType);
    static constexpr auto contextSize = sizeof(PersistentTimestampType) + uuidSize + IpAddress::dataSize();

    const auto dataStart = blobStart + eventHeaderSize + contextSize;

    if (ADD_NEW_DISCUSSION_THREAD == eventType)
    {
        CHECK_SIZE;
        CHECK_VERSION_1;

        const auto threadId = parseUuid(dataStart);
        const auto name = readStringViewWithPrefix(dataStart + uuidSize);

        onAddNewDiscussionThread(threadId, name);
    }
    else if (CHANGE_DISCUSSION_THREAD_NAME == eventType)
    {
        CHECK_SIZE;
        CHECK_VERSION_1;

        const auto threadId = parseUuid(dataStart);
        const auto newName = readStringViewWithPrefix(dataStart + uuidSize);

        onChangeDiscussionThreadName(threadId, newName);
    }
    else if (DELETE_DISCUSSION_THREAD == eventType)
    {
        CHECK_SIZE;
        CHECK_VERSION_1;

        const auto threadId = parseUuid(dataStart);

        onDeleteDiscussionThread(threadId);
    }
    else if (MERGE_DISCUSSION_THREADS == eventType)
    {
        CHECK_SIZE;
        CHECK_VERSION_1;

        const auto fromThreadId = parseUuid(dataStart);
        const auto intoThreadId = parseUuid(dataStart + uuidSize);

        onMergeDiscussionThread(fromThreadId, intoThreadId);
    }
    else if (ADD_NEW_DISCUSSION_THREAD_MESSAGE == eventType)
    {
        CHECK_SIZE;
        CHECK_VERSION_1;

        const auto messageId = parseUuid(dataStart);
        const auto threadId = parseUuid(dataStart + uuidSize);
        const auto content = readStringViewWithPrefix(dataStart + uuidSize + uuidSize);

        onAddNewDiscussionThreadMessage(messageId, threadId, content);
    }
    else if (CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT == eventType)
    {
        CHECK_SIZE;
        CHECK_VERSION_1;

        const auto messageId = parseUuid(dataStart);
        const auto newContent = readStringViewWithPrefix(dataStart + uuidSize);

        onChangeDiscussionThreadMessageContent(messageId, newContent);
    }
    else if (DELETE_DISCUSSION_THREAD_MESSAGE == eventType)
    {
        CHECK_SIZE;
        CHECK_VERSION_1;

        const auto messageId = parseUuid(dataStart);

        onDeleteDiscussionThreadMessage(messageId);
    }

    return true;
}

template<typename T>
static void writeOrAbort(FILE* file, const T* elements, const size_t nrOfElements)
{
    fwrite(elements, sizeof(T), nrOfElements, file);
    if (ferror(file))
    {
        std::cerr << "Could not write to file\n";
        std::abort();
    }
}

template<typename T>
static void writeOrAbort(FILE* file, T element)
{
    writeOrAbort(file, &element, 1);
}

static void writeOrAbort(FILE* file, std::string_view stringView)
{
    writeOrAbort(file, stringView.data(), stringView.size());
}

namespace Json
{
    JsonWriter& operator<<(JsonWriter& writer, const IdType& id)
    {
        char buffer[boost::uuids::uuid::static_size() * 2 + 4];
        static const char* hexCharsLowercase = "0123456789abcdef";

        const auto& data = id.data;

        for (size_t source = 0, destination = 0; source < boost::uuids::uuid::static_size(); ++source)
        {
            const auto value = data[source];

            buffer[destination++] = hexCharsLowercase[(value / 16) & 0xF];
            buffer[destination++] = hexCharsLowercase[(value % 16) & 0xF];

            if (8 == destination || 13 == destination || 18 == destination || 23 == destination)
            {
                buffer[destination++] = '-';
            }
        }

        return writer.writeSafeString(buffer, std::size(buffer));
    }
}

void SearchDataExtractor::writeJson(std::function<void(JsonWriter&)>&& callback)
{
    outputBuffer_.clear();
    JsonWriter writer{ outputBuffer_ };
    callback(writer);

    if (outputBuffer_.view().empty()) return;

    if (currentEventsPerFile_ >= eventsPerFile_)
    {
        closeFile();
    }
    if (nullptr == currentFile_)
    {
        auto fileName = boost::str(boost::format(outputTemplate_) % ++currentFileNr_);
        currentFile_ = fopen(fileName.c_str(), "wb");
        if (nullptr == currentFile_)
        {
            std::cerr << "Could not open file for writing: " << fileName << '\n';
            std::abort();
        }
        writeOrAbort(currentFile_, '[');
    }
    if (currentEventsPerFile_++ > 0)
    {
        writeOrAbort(currentFile_, ',');
    }
    writeOrAbort(currentFile_, outputBuffer_.view());
}

void SearchDataExtractor::closeFile()
{
    if (nullptr == currentFile_) return;

    if (currentEventsPerFile_)
    {
        writeOrAbort(currentFile_, ']');
    }
    fclose(currentFile_);
    currentFile_ = nullptr;
    currentEventsPerFile_ = 0;
}

void SearchDataExtractor::onAddNewDiscussionThread(const IdType& id, std::string_view name)
{
    if (threadMessages_.insert(std::make_pair(id, std::set<IdType>{})).second)
    {
        writeJson([&id, name](JsonWriter& writer)
        {
            writer
                << objStart
                    << propertySafeName("type", "new thread")
                    << propertySafeName("id", id)
                    << propertySafeName("name", name)
                << objEnd;
        });
    }
}

void SearchDataExtractor::onChangeDiscussionThreadName(const IdType& id, std::string_view newName)
{
    writeJson([&id, newName](JsonWriter& writer)
    {
        writer
            << objStart
                << propertySafeName("type", "change thread name")
                << propertySafeName("id", id)
                << propertySafeName("name", newName)
            << objEnd;
    });
}

void SearchDataExtractor::onDeleteDiscussionThread(const IdType& id)
{
    auto findMessagesOfThread = threadMessages_.find(id);
    if (findMessagesOfThread == threadMessages_.end()) return;

    writeJson([&id](JsonWriter& writer)
    {
        writer
            << objStart
                << propertySafeName("type", "delete thread")
                << propertySafeName("id", id)
            << objEnd;
    });

    for (const auto& messageId : findMessagesOfThread->second)
    {
        onDeleteDiscussionThreadMessage(id);
    }

    threadMessages_.erase(findMessagesOfThread);
}

void SearchDataExtractor::onMergeDiscussionThread(const IdType& fromId, const IdType& intoId)
{
    for (const auto& messageId : threadMessages_[fromId])
    {
        threadMessages_[intoId].insert(messageId);
    }
    //do not delete messages of from thread
    threadMessages_[fromId] = {};
    onDeleteDiscussionThread(fromId);
}

void SearchDataExtractor::onAddNewDiscussionThreadMessage(const IdType& id, const IdType& threadId,
                                                          std::string_view content)
{
    if ( ! threadMessages_[threadId].insert(id).second) return;

    writeJson([&id, content](JsonWriter& writer)
    {
        writer
            << objStart
                << propertySafeName("type", "new thread message")
                << propertySafeName("id", id)
                << propertySafeName("content", content)
            << objEnd;
    });
}

void SearchDataExtractor::onChangeDiscussionThreadMessageContent(const IdType& id, std::string_view newContent)
{
    writeJson([&id, newContent](JsonWriter& writer)
    {
        writer
            << objStart
            << propertySafeName("type", "change thread message content")
            << propertySafeName("id", id)
            << propertySafeName("content", newContent)
            << objEnd;
    });
}

void SearchDataExtractor::onDeleteDiscussionThreadMessage(const IdType& id)
{
    if (deletedMessages_.insert(id).second)
    {
        writeJson([&id](JsonWriter& writer)
        {
            writer
                << objStart
                << propertySafeName("type", "delete thread message")
                << propertySafeName("id", id)
                << objEnd;
        });
    }
}
