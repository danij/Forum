#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <cstring>

#include <boost/program_options/parsers.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/filesystem.hpp>

#include "PersistenceFormat.h"
#include "TypeHelpers.h"
#include "IpAddress.h"

using namespace Forum::Persistence;
using namespace Forum::Helpers;

class MessageExtractor final
{
public:
    MessageExtractor(const char* inputData, size_t inputSize, uint64_t currentOffset,
                     std::ostream& eventOutput, std::ostream& messagesOutput);
    int perform();

private:
    int processBlob(const char* data, size_t size);
    int writeBlob(const char* data, size_t size);

    const char* inputData_;
    size_t inputSize_;
    uint64_t currentOffset_;
    std::ostream& eventOutput_;
    std::ostream& messagesOutput_;
};

int startExtraction(const std::string& input, const std::string& output, const std::string& messages);

int main(int argc, const char* argv[])
{
    boost::program_options::options_description options("Available options");
    options.add_options()
        ("help,h", "Display available options")
        ("input,i", boost::program_options::value<std::string>(), "Input file")
        ("output,o", boost::program_options::value<std::string>(), "Output file")
        ("messages,m", boost::program_options::value<std::string>(), "File where to append messages");

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
                               arguments["messages"].as<std::string>());
    }
    else
    {
        std::cout << options << '\n';
        return 1;
    }
}

int startExtraction(const std::string& input, const std::string& output, const std::string& messages)
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
        auto mappingMode = boost::interprocess::read_only;
        boost::interprocess::file_mapping mapping(input.c_str(), mappingMode);
        boost::interprocess::mapped_region region(mapping, mappingMode);
        region.advise(boost::interprocess::mapped_region::advice_sequential);

        auto ptr = reinterpret_cast<const char*>(region.get_address());
        auto size = region.get_size();

        MessageExtractor extractor(ptr, size, messagesFileSize, outputStream, messageStream);
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

MessageExtractor::MessageExtractor(const char* inputData, size_t inputSize, uint64_t currentOffset,
                                   std::ostream& eventOutput, std::ostream& messagesOutput)
    : inputData_(inputData), inputSize_(inputSize), currentOffset_(currentOffset), eventOutput_(eventOutput),
      messagesOutput_(messagesOutput)
{
}

int MessageExtractor::perform()
{
    auto data = inputData_;
    auto size = inputSize_;

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

        auto blobSize = readAndIncrementBuffer<BlobSizeType>(data, size);
        auto blobSizeWithPadding = blobSize + blobPaddingRequired(blobSize);

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

        auto result = processBlob(data, blobSize);
        if (result != 0)
        {
            return result;
        }

        data += blobSizeWithPadding;
        size -= blobSizeWithPadding;
    }
    return 0;
}

int MessageExtractor::processBlob(const char* data, size_t size)
{
    auto blobStart = data;
    auto blobSize = size;

    auto eventType = readAndIncrementBuffer<EventType>(data, size);
    auto version = readAndIncrementBuffer<EventVersionType>(data, size);
    auto contextVersion = readAndIncrementBuffer<EventContextVersionType>(data, size);
    (void)contextVersion;

    auto blobToWrite = blobStart;

    static constexpr auto uuidSize = boost::uuids::uuid::static_size();

    static constexpr auto contextSize = sizeof(PersistentTimestampType) + uuidSize + IpAddress::dataSize();
    static constexpr auto sameAsOldVersionSize = contextSize + uuidSize /*messageId*/ + uuidSize /*parentId*/;
    static constexpr auto newBlobSize = sizeof(EventType) + sizeof(EventVersionType) + sizeof(EventContextVersionType) +
                                        sameAsOldVersionSize + sizeof(uint32_t) + sizeof(uint64_t);
    char blobBuffer[newBlobSize];

    if ((ADD_NEW_DISCUSSION_THREAD_MESSAGE == eventType) && (1 == version) && (1 == contextVersion))
    {
        if (size < contextSize)
        {
            std::cerr << "Unable to import context v1: expected " << contextSize << " bytes, found only " << size;
            return false;
        }
        char* blobData = blobBuffer;

        version = 2;

        writeValue(blobData, eventType); blobData += sizeof(eventType);
        writeValue(blobData, version); blobData += sizeof(version);
        writeValue(blobData, contextVersion); blobData += sizeof(contextVersion);

        memcpy(blobData, data, sameAsOldVersionSize); blobData += sameAsOldVersionSize;
        data += sameAsOldVersionSize; size -= sameAsOldVersionSize;

        auto messageSize = readAndIncrementBuffer<uint32_t>(data, size);

        if (size != messageSize)
        {
            std::cerr << "Remaining size (" << size << ") is different from the expected one (" << messageSize << ")\n";
            return 2;
        }
        auto message = data;

        if ( ! messagesOutput_.write(message, messageSize))
        {
            std::cerr << "Could not append message";
            return 2;
        }

        writeValue(blobData, messageSize); blobData += sizeof(messageSize);
        writeValue(blobData, currentOffset_); blobData += sizeof(currentOffset_);

        currentOffset_ += messageSize;

        blobToWrite = blobBuffer;
        blobSize = newBlobSize;
    }

    auto result = writeBlob(blobToWrite, blobSize);
    if (result != 0)
    {
        return result;
    }

    return 0;
}

int MessageExtractor::writeBlob(const char* data, size_t size)
{
    static constexpr size_t prefixSize = sizeof(MagicPrefix) + sizeof(uint32_t) + sizeof(uint32_t);
    char prefixBuffer[prefixSize];

    auto blobSize = static_cast<BlobSizeType>(size);
    auto blobCRC32 = crc32(data, size);

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

    auto paddingNeeded = blobPaddingRequired(blobSize);
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
