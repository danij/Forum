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

#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>

#include "IpAddress.h"

#include <boost/program_options/parsers.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/filesystem.hpp>

#include "PersistenceFormat.h"
#include "TypeHelpers.h"

using namespace Forum::Persistence;
using namespace Forum::Helpers;

class MessageExtractor final
{
public:
    MessageExtractor(const char* inputData, size_t inputSize, uint64_t currentOffset, bool skipLatest,
                     std::ostream& eventOutput, std::ostream& messagesOutput);
    int perform();

private:
    int iterateBlobs(std::function<int(const char*, size_t)>&& fn);
    int updateLatestMessages(const char* data, size_t size);
    int processBlob(const char* data, size_t size);
    int writeBlob(const char* data, size_t size);

    const char* inputData_;
    size_t inputSize_;
    uint64_t currentOffset_;
    bool skipLatest_;
    std::ostream& eventOutput_;
    std::ostream& messagesOutput_;
    std::map<boost::uuids::uuid, boost::uuids::uuid> latestThreadMessages_;
};

int startExtraction(const std::string& input, const std::string& output, const std::string& messages, bool skipLatest);

int main(int argc, const char* argv[])
{
    boost::program_options::options_description options("Available options");
    options.add_options()
        ("help,h", "Display available options")
        ("input,i", boost::program_options::value<std::string>(), "Input file")
        ("output,o", boost::program_options::value<std::string>(), "Output file")
        ("messages,m", boost::program_options::value<std::string>(), "File where to append messages")
        ("skip-latest,l", "Skip the latest message of each discussion thread");

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

    if (arguments.count("input") && arguments.count("output") && arguments.count("messages"))
    {
        return startExtraction(arguments["input"].as<std::string>(), arguments["output"].as<std::string>(),
                               arguments["messages"].as<std::string>(), arguments.count("skip-latest") > 0);
    }
    else
    {
        std::cout << options << '\n';
        return 1;
    }
}

int startExtraction(const std::string& input, const std::string& output, const std::string& messages, 
                    const bool skipLatest)
{
    std::ofstream outputStream(output, std::ios_base::binary);
    if ( ! outputStream)
    {
        std::cerr << "Could not open output file: " << output << '\n';
        return 1;
    }

    uint64_t messagesFileSize = 0;

    if (boost::filesystem::is_regular_file(messages))
    {
        messagesFileSize = static_cast<decltype(messagesFileSize)>(boost::filesystem::file_size(messages));
    }

    std::ofstream messageStream(messages, std::ios_base::app | std::ios_base::binary);
    if ( ! messageStream)
    {
        std::cerr << "Could not open message file: " << messages << '\n';
        return 1;
    }

    try
    {
        const auto mappingMode = boost::interprocess::read_only;
        boost::interprocess::file_mapping mapping(input.c_str(), mappingMode);
        boost::interprocess::mapped_region region(mapping, mappingMode);
        region.advise(boost::interprocess::mapped_region::advice_sequential);

        const auto ptr = reinterpret_cast<const char*>(region.get_address());
        const auto size = region.get_size();

        MessageExtractor extractor(ptr, size, messagesFileSize, skipLatest, outputStream, messageStream);
        return extractor.perform();
    }
    catch (boost::interprocess::interprocess_exception& ex)
    {
        std::cerr << "Error mapping input file: " << input << " (" << ex.what() << ')';
        return 1;
    }
}

static constexpr char Padding[8] = { 0 };

template<typename T>
T readAndIncrementBuffer(const char*& data, size_t& size)
{
    T result;
    memcpy(&result, data, sizeof(T));
    data += sizeof(T); size -= sizeof(T);

    return result;
}

MessageExtractor::MessageExtractor(const char* inputData, const size_t inputSize, const uint64_t currentOffset, 
                                   const bool skipLatest, std::ostream& eventOutput, std::ostream& messagesOutput)
    : inputData_(inputData), inputSize_(inputSize), currentOffset_(currentOffset), skipLatest_(skipLatest),
      eventOutput_(eventOutput), messagesOutput_(messagesOutput)
{
}

int MessageExtractor::perform()
{
    if (skipLatest_)
    {
        const auto result = iterateBlobs([this](const char* data, const size_t blobSize)
        {
            return this->updateLatestMessages(data, blobSize);
        });
        if (result) return result;
    }

    return iterateBlobs([this](const char* data, const size_t blobSize)
    {
        return this->processBlob(data, blobSize);
    });
}

int MessageExtractor::iterateBlobs(std::function<int(const char*, size_t)>&& fn)
{
    auto data = inputData_;
    auto size = inputSize_;
    int oldProcessedPercent = -1;

    while (size > 0)
    {
        if (size < MinBlobSize)
        {
            std::cerr << "Found bytes that are not enough to contain a persisted event blob";
            break;
        }

        const auto magic = readAndIncrementBuffer<MagicPrefixType>(data, size);
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
        if (result != 0)
        {
            return result;
        }

        data += blobSizeWithPadding;
        size -= blobSizeWithPadding;

        const auto processed = inputSize_ - size;
        const auto processedPercent = static_cast<int>((processed * 100.0) / inputSize_);
        if (processedPercent > oldProcessedPercent)
        {
            std::cout << processedPercent << "% " << std::flush;
            oldProcessedPercent = processedPercent;
        }
    }
    std::cout << std::endl;
    return 0;
}

boost::uuids::uuid parseUuid(const char* data)
{
    boost::uuids::uuid result{};
    std::copy(data, data + boost::uuids::uuid::static_size(), result.data);
    return result;
}

int MessageExtractor::updateLatestMessages(const char* data, size_t size)
{
    const auto blobStart = data;

    const auto eventType = readAndIncrementBuffer<EventType>(data, size);
    const auto version = readAndIncrementBuffer<EventVersionType>(data, size);
    const auto contextVersion = readAndIncrementBuffer<EventContextVersionType>(data, size);

    static constexpr auto uuidSize = boost::uuids::uuid::static_size();

    static constexpr auto eventHeaderSize = sizeof(EventType) + sizeof(EventVersionType) + sizeof(EventContextVersionType);
    static constexpr auto contextSize = sizeof(PersistentTimestampType) + uuidSize + IpAddress::dataSize();

    if ((ADD_NEW_DISCUSSION_THREAD_MESSAGE == eventType) && ((1 == version) || (3 == version)) && (1 == contextVersion))
    {
        if (size < contextSize)
        {
            std::cerr << "Unable to import context v1: expected " << contextSize << " bytes, found only " << size;
            return false;
        }

        auto messageId = parseUuid(blobStart + eventHeaderSize + contextSize);
        auto threadId = parseUuid(blobStart + eventHeaderSize + contextSize + uuidSize);

        auto it = latestThreadMessages_.find(threadId);
        if (it == latestThreadMessages_.end())
        {
            latestThreadMessages_.insert(std::make_pair(threadId, messageId));
        }
        else
        {
            it->second = messageId;
        }
    }

    return 0;
}

int MessageExtractor::processBlob(const char* data, size_t size)
{
    const auto blobStart = data;
    auto blobSize = size;

    const auto eventType = readAndIncrementBuffer<EventType>(data, size);
    const auto version = readAndIncrementBuffer<EventVersionType>(data, size);
    const auto contextVersion = readAndIncrementBuffer<EventContextVersionType>(data, size);

    auto blobToWrite = blobStart;

    static constexpr auto uuidSize = boost::uuids::uuid::static_size();

    static constexpr auto eventHeaderSize = sizeof(EventType) + sizeof(EventVersionType) + sizeof(EventContextVersionType);
    static constexpr auto contextSize = sizeof(PersistentTimestampType) + uuidSize + IpAddress::dataSize();
    static constexpr auto sameAsOldVersionSize = contextSize + uuidSize /*messageId*/ + uuidSize /*parentId*/;
    static constexpr auto newBlobSize = sizeof(EventType) + sizeof(EventVersionType) + sizeof(EventContextVersionType) +
                                        sameAsOldVersionSize + 2 * sizeof(uint32_t) + sizeof(uint64_t);
    char blobBuffer[newBlobSize];

    if ((ADD_NEW_DISCUSSION_THREAD_MESSAGE == eventType) && ((1 == version) || (3 == version)) && (1 == contextVersion))
    {
        if (size < contextSize)
        {
            std::cerr << "Unable to import context v1: expected " << contextSize << " bytes, found only " << size;
            return false;
        }

        const auto messageId = parseUuid(blobStart + eventHeaderSize + contextSize);
        const auto threadId = parseUuid(blobStart + eventHeaderSize + contextSize + uuidSize);

        const auto it = latestThreadMessages_.find(threadId);
        if ((it == latestThreadMessages_.end()) || (it->second != messageId))
        {
            char* blobData = blobBuffer;

            decltype(version) newVersion = 4;

            writeValue(blobData, eventType); blobData += sizeof(eventType);
            writeValue(blobData, newVersion); blobData += sizeof(newVersion);
            writeValue(blobData, contextVersion); blobData += sizeof(contextVersion);

            memmove(blobData, data, sameAsOldVersionSize); blobData += sameAsOldVersionSize;
            data += sameAsOldVersionSize; size -= sameAsOldVersionSize;

            uint32_t approved = 1;
            if (3 == version)
            {
                approved = readAndIncrementBuffer<uint32_t>(data, size);
            }

            auto messageSize = readAndIncrementBuffer<uint32_t>(data, size);

            if (size != messageSize)
            {
                std::cerr << "Remaining size (" << size << ") is different from the expected one (" << messageSize << ")\n";
                return 2;
            }
            const auto message = data;

            if ( ! messagesOutput_.write(message, messageSize))
            {
                std::cerr << "Could not append message";
                return 2;
            }

            writeValue(blobData, approved); blobData += sizeof(approved);
            writeValue(blobData, messageSize); blobData += sizeof(messageSize);
            writeValue(blobData, currentOffset_); blobData += sizeof(currentOffset_);

            currentOffset_ += messageSize;

            blobToWrite = blobBuffer;
            blobSize = newBlobSize;
        }
    }

    const auto result = writeBlob(blobToWrite, blobSize);
    if (result != 0)
    {
        return result;
    }

    return 0;
}

int MessageExtractor::writeBlob(const char* data, const size_t size)
{
    static constexpr size_t prefixSize = sizeof(MagicPrefix) + sizeof(uint32_t) + sizeof(uint32_t);
    char prefixBuffer[prefixSize];

    auto blobSize = static_cast<BlobSizeType>(size);
    const auto blobCRC32 = crc32(data, size);

    auto prefix = prefixBuffer;

    writeValue(prefix, MagicPrefix); prefix += sizeof(MagicPrefix);
    writeValue(prefix, blobSize); prefix += sizeof(blobSize);
    writeValue(prefix, blobCRC32);

    if ( ! eventOutput_.write(prefixBuffer, prefixSize))
    {
        std::cerr << "Could not write event prefix\n";
        return 2;
    }
    if ( ! eventOutput_.write(data, size))
    {
        std::cerr << "Could not write blob data\n";
        return 2;
    }

    const auto paddingNeeded = blobPaddingRequired(blobSize);
    if (paddingNeeded)
    {
        if ( ! eventOutput_.write(Padding, paddingNeeded))
        {
            std::cerr << "Could not write blob padding\n";
            return 2;
        }
    }
    return 0;
}
