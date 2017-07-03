#include "EventImporter.h"
#include "PersistenceFormat.h"
#include "Logging.h"
#include "ContextProviders.h"
#include "ContextProviderMocks.h"
#include "UuidString.h"
#include "IpAddress.h"

#include <cstdint>
#include <ctime>
#include <functional>
#include <map>
#include <numeric>
#include <regex>
#include <string>
#include <type_traits>
#include <vector>

#include <boost/noncopyable.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/iterator_range.hpp>

using namespace Forum;
using namespace Forum::Persistence;
using namespace Forum::Entities;
using namespace Forum::Helpers;

template<typename Fn>
static void iteratePathRecursively(const boost::filesystem::path& sourcePath, Fn&& action)
{
    if (is_directory(sourcePath))
    {
        for (auto& entry : boost::make_iterator_range(boost::filesystem::directory_iterator(sourcePath), {}))
        {
            iteratePathRecursively(entry, action);
        }
    }
    else if (is_regular_file(sourcePath))
    {
        action(sourcePath);
    }
}

template<typename T>
T readAndIncrementBuffer(const uint8_t*& data, size_t& size)
{
    auto result = *reinterpret_cast<typename std::add_pointer<typename std::add_const<T>::type>::type>(data);
    data += sizeof(T); size -= sizeof(T);

    return result;
}

template<>
UuidString readAndIncrementBuffer<UuidString>(const uint8_t*& data, size_t& size)
{
    UuidString result(data);

    static constexpr auto dataSize = boost::uuids::uuid::static_size();
    data += dataSize; size -= dataSize;

    return result;
}

template<>
IpAddress readAndIncrementBuffer<IpAddress>(const uint8_t*& data, size_t& size)
{
    IpAddress result(data);

    static constexpr auto dataSize = IpAddress::dataSize();
    data += dataSize; size -= dataSize;

    return result;
}

struct CurrentTimeChanger final : private boost::noncopyable
{
    explicit CurrentTimeChanger(std::function<Timestamp()>&& fn)
    {
        Context::setCurrentTimeMockForCurrentThread(std::move(fn));
    }

    ~CurrentTimeChanger()
    {
        Context::resetCurrentTimeMock();
    }
};

struct EventImporter::EventImporterImpl final : private boost::noncopyable
{
    explicit EventImporterImpl(bool verifyChecksum) : verifyChecksum_(verifyChecksum)
    {
        importFunctions_ =
        {
            {}, //UNKNOWN
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_ADD_NEW_USER_v1                                (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_CHANGE_USER_NAME_v1                            (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_CHANGE_USER_INFO_v1                            (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_DELETE_USER_v1                                 (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_ADD_NEW_DISCUSSION_THREAD_v1                   (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_CHANGE_DISCUSSION_THREAD_NAME_v1               (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_DELETE_DISCUSSION_THREAD_v1                    (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_MERGE_DISCUSSION_THREADS_v1                    (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_SUBSCRIBE_TO_DISCUSSION_THREAD_v1              (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_UNSUBSCRIBE_FROM_DISCUSSION_THREAD_v1          (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_ADD_NEW_DISCUSSION_THREAD_MESSAGE_v1           (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT_v1    (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_MOVE_DISCUSSION_THREAD_MESSAGE_v1              (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_DELETE_DISCUSSION_THREAD_MESSAGE_v1            (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_DISCUSSION_THREAD_MESSAGE_UP_VOTE_v1           (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_DISCUSSION_THREAD_MESSAGE_DOWN_VOTE_v1         (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_DISCUSSION_THREAD_MESSAGE_RESET_VOTE_v1        (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_ADD_COMMENT_TO_DISCUSSION_THREAD_MESSAGE_v1    (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_SOLVE_DISCUSSION_THREAD_MESSAGE_COMMENT_v1     (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_ADD_NEW_DISCUSSION_TAG_v1                      (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_CHANGE_DISCUSSION_TAG_NAME_v1                  (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_CHANGE_DISCUSSION_TAG_UI_BLOB_v1               (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_DELETE_DISCUSSION_TAG_v1                       (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_ADD_DISCUSSION_TAG_TO_THREAD_v1                (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_REMOVE_DISCUSSION_TAG_FROM_THREAD_v1           (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_MERGE_DISCUSSION_TAGS_v1                       (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_ADD_NEW_DISCUSSION_CATEGORY_v1                 (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_CHANGE_DISCUSSION_CATEGORY_NAME_v1             (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_CHANGE_DISCUSSION_CATEGORY_DESCRIPTION_v1      (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER_v1    (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_CHANGE_DISCUSSION_CATEGORY_PARENT_v1           (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_DELETE_DISCUSSION_CATEGORY_v1                  (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_ADD_DISCUSSION_TAG_TO_CATEGORY_v1              (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_REMOVE_DISCUSSION_TAG_FROM_CATEGORY_v1         (contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_INCREMENT_DISCUSSION_THREAD_NUMBER_OF_VISITS_v1(contextVersion, data, size); } },
        };
    }

    bool import_ADD_NEW_USER_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_CHANGE_USER_NAME_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_CHANGE_USER_INFO_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_DELETE_USER_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_ADD_NEW_DISCUSSION_THREAD_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_CHANGE_DISCUSSION_THREAD_NAME_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_DELETE_DISCUSSION_THREAD_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_MERGE_DISCUSSION_THREADS_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_SUBSCRIBE_TO_DISCUSSION_THREAD_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_UNSUBSCRIBE_FROM_DISCUSSION_THREAD_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_ADD_NEW_DISCUSSION_THREAD_MESSAGE_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_MOVE_DISCUSSION_THREAD_MESSAGE_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_DELETE_DISCUSSION_THREAD_MESSAGE_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_DISCUSSION_THREAD_MESSAGE_UP_VOTE_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_DISCUSSION_THREAD_MESSAGE_DOWN_VOTE_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_DISCUSSION_THREAD_MESSAGE_RESET_VOTE_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_ADD_COMMENT_TO_DISCUSSION_THREAD_MESSAGE_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_SOLVE_DISCUSSION_THREAD_MESSAGE_COMMENT_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_ADD_NEW_DISCUSSION_TAG_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_CHANGE_DISCUSSION_TAG_NAME_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_CHANGE_DISCUSSION_TAG_UI_BLOB_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_DELETE_DISCUSSION_TAG_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_ADD_DISCUSSION_TAG_TO_THREAD_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_REMOVE_DISCUSSION_TAG_FROM_THREAD_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_MERGE_DISCUSSION_TAGS_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_ADD_NEW_DISCUSSION_CATEGORY_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_CHANGE_DISCUSSION_CATEGORY_NAME_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_CHANGE_DISCUSSION_CATEGORY_DESCRIPTION_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_CHANGE_DISCUSSION_CATEGORY_PARENT_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_DELETE_DISCUSSION_CATEGORY_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_ADD_DISCUSSION_TAG_TO_CATEGORY_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_REMOVE_DISCUSSION_TAG_FROM_CATEGORY_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool import_INCREMENT_DISCUSSION_THREAD_NUMBER_OF_VISITS_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        return true;
    }

    bool processContext_v1(const uint8_t*& data, size_t& size)
    {
        currentTimestamp_ = readAndIncrementBuffer<PersistentTimestampType>(data, size);
        Context::setCurrentUserId(readAndIncrementBuffer<UuidString>(data, size));
        Context::setCurrentUserIpAddress(readAndIncrementBuffer<IpAddress>(data, size));
        return true;
    }

    bool processContext(uint16_t contextVersion, const uint8_t*& data, size_t& size)
    {
        if (1 != contextVersion)
        {
            FORUM_LOG_ERROR << "Unimplemented context version: " << contextVersion;
            return false;

        }
        return processContext_v1(data, size);
    }

    ImportResult importFile(const std::string& fileName)
    {
        FORUM_LOG_INFO << "Imporing events from: " << fileName;

        try
        {
            auto mappingMode = boost::interprocess::read_only;
            boost::interprocess::file_mapping mapping(fileName.c_str(), mappingMode);
            boost::interprocess::mapped_region region(mapping, mappingMode);
            region.advise(boost::interprocess::mapped_region::advice_sequential);

            return iterateBlobsInFile(reinterpret_cast<const uint8_t*>(region.get_address()), region.get_size());
        }
        catch(boost::interprocess::interprocess_exception& ex)
        {
            FORUM_LOG_ERROR << "Error mapping file: " << fileName << '(' << ex.what() << ')';
            return {};
        }
    }

    ImportResult iterateBlobsInFile(const uint8_t* data, size_t size)
    {
        ImportResult result{};
        CurrentTimeChanger _([this]() { return this->getCurrentTimestamp(); });

        while (size > 0)
        {
            if (size < MinBlobSize)
            {
                FORUM_LOG_ERROR << "Found bytes that are not enough to contain a persisted event blob";
                break;
            }

            auto magic = readAndIncrementBuffer<MagicPrefixType>(data, size);
            if (magic != MagicPrefix)
            {
                FORUM_LOG_ERROR << "Invalid prefix in current blob";
                break;
            }

            auto blobSize = readAndIncrementBuffer<BlobSizeType>(data, size);
            auto blobSizeWithPadding = blobSize + blobPaddingRequired(blobSize);

            auto storedChecksum = readAndIncrementBuffer<BlobChecksumSizeType>(data, size);

            if (size < blobSizeWithPadding)
            {
                FORUM_LOG_ERROR << "Not enough bytes remaining in file for a full event blob";
                result.success = false;
                break;
            }

            if (verifyChecksum_)
            {
                auto calculatedChecksum = crc32(data, blobSize);
                if (calculatedChecksum != storedChecksum)
                {
                    FORUM_LOG_ERROR << "Checksum mismatch in event blob: " << calculatedChecksum << " != " << storedChecksum;
                    result.success = false;
                    break;
                }
            }

            result.statistic.readBlobs += 1;
            if (processEvent(data, blobSize))
            {
                result.statistic.importedBlobs += 1;
            }

            data += blobSizeWithPadding;
            size -= blobSizeWithPadding;
        }

        return result;
    }

    bool processEvent(const uint8_t* data, size_t size)
    {
        if (size < EventHeaderSize)
        {
            FORUM_LOG_WARNING << "Blob too small";
            return false;
        }

        auto eventType = readAndIncrementBuffer<EventType>(data, size);
        auto version = readAndIncrementBuffer<EventVersionType>(data, size);
        auto contextVersion = readAndIncrementBuffer<EventContextVersionType>(data, size);

        if (eventType >= importFunctions_.size())
        {
            FORUM_LOG_WARNING << "Import for unknown type " << eventType;
            return false;
        }

        auto& importerVersions = importFunctions_[eventType];
        if (version >= importerVersions.size())
        {
            FORUM_LOG_WARNING << "Import for unsupported version " << version << " for event " << eventType;
            return false;
        }

        auto& fn = importerVersions[version];
        if ( ! fn)
        {
            FORUM_LOG_WARNING << "Missing import function for version " << version << " for event " << eventType;
            return false;
        }

        return fn(contextVersion, data, size);
    }

    Timestamp getCurrentTimestamp()
    {
        return currentTimestamp_;
    }

private:
    bool verifyChecksum_;
    std::vector<std::vector<std::function<bool(uint16_t, const uint8_t*, size_t)>>> importFunctions_;
    Timestamp currentTimestamp_;
};

EventImporter::EventImporter(bool verifyChecksum) : impl_(new EventImporterImpl(verifyChecksum))
{
}

EventImporter::~EventImporter()
{
    if (impl_)
    {
        delete impl_;
    }
}

ImportResult EventImporter::import(const boost::filesystem::path& sourcePath)
{
    std::map<time_t, std::string> eventFileNames;
    std::regex eventFileMatcher("^forum-(\\d+).events$", std::regex_constants::icase);

    iteratePathRecursively(sourcePath, [&](auto& path)
    {
        std::smatch match;
        auto fileName = path.filename().string();
        if (std::regex_match(fileName, match, eventFileMatcher))
        {
            auto timestampString = match[1].str();
            time_t timestamp{};

            if ( ! boost::conversion::try_lexical_convert(timestampString, timestamp))
            {
                FORUM_LOG_ERROR << "Cannot convert timestamp from " << fileName;
            }
            else
            {
                auto fullName = path.string();
                eventFileNames.insert(std::make_pair(timestamp, fullName));
            }
        }
    });


    ImportResult result{};
    for (auto& pair : eventFileNames)
    {
        auto currentResult = impl_->importFile(pair.second);
        if ( ! currentResult.success)
        {
            break;
        }
        result.statistic = result.statistic + currentResult.statistic;
    }

    return result;
}
