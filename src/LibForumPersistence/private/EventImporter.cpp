#include "EventImporter.h"
#include "PersistenceFormat.h"
#include "Logging.h"

#include <cstdint>
#include <ctime>
#include <functional>
#include <map>
#include <numeric>
#include <regex>
#include <string>
#include <type_traits>
#include <vector>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/iterator_range.hpp>

using namespace Forum;
using namespace Forum::Persistence;

template<typename Fn>
static void iteratePathRecursively(const boost::filesystem::path& sourcePath, Fn& action)
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
T readAndIncrementBuffer(const char*& data, size_t& size)
{
    auto result = *reinterpret_cast<typename std::add_pointer<typename std::add_const<T>::type>::type>(data);
    data += sizeof(T); size -= sizeof(T);

    return result;
}

struct PersistedContext
{
    
};

struct EventImporter::EventImporterImpl final : private boost::noncopyable
{
    explicit EventImporterImpl(bool verifyChecksum) : verifyChecksum_(verifyChecksum)
    {
        importFunctions_ = 
        {
            {}, //UNKNOWN
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_ADD_NEW_USER_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_CHANGE_USER_NAME_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_CHANGE_USER_INFO_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_DELETE_USER_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_ADD_NEW_DISCUSSION_THREAD_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_CHANGE_DISCUSSION_THREAD_NAME_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_DELETE_DISCUSSION_THREAD_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_MERGE_DISCUSSION_THREADS_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_SUBSCRIBE_TO_DISCUSSION_THREAD_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_UNSUBSCRIBE_FROM_DISCUSSION_THREAD_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_ADD_NEW_DISCUSSION_THREAD_MESSAGE_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_MOVE_DISCUSSION_THREAD_MESSAGE_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_DELETE_DISCUSSION_THREAD_MESSAGE_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_DISCUSSION_THREAD_MESSAGE_UP_VOTE_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_DISCUSSION_THREAD_MESSAGE_DOWN_VOTE_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_DISCUSSION_THREAD_MESSAGE_RESET_VOTE_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_ADD_COMMENT_TO_DISCUSSION_THREAD_MESSAGE_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_SOLVE_DISCUSSION_THREAD_MESSAGE_COMMENT_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_ADD_NEW_DISCUSSION_TAG_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_CHANGE_DISCUSSION_TAG_NAME_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_CHANGE_DISCUSSION_TAG_UI_BLOB_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_DELETE_DISCUSSION_TAG_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_ADD_DISCUSSION_TAG_TO_THREAD_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_REMOVE_DISCUSSION_TAG_FROM_THREAD_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_MERGE_DISCUSSION_TAGS_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_ADD_NEW_DISCUSSION_CATEGORY_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_CHANGE_DISCUSSION_CATEGORY_NAME_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_CHANGE_DISCUSSION_CATEGORY_DESCRIPTION_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_CHANGE_DISCUSSION_CATEGORY_PARENT_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_DELETE_DISCUSSION_CATEGORY_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_ADD_DISCUSSION_TAG_TO_CATEGORY_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_REMOVE_DISCUSSION_TAG_FROM_CATEGORY_v1(contextVersion, data, size); } },
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { this->import_INCREMENT_DISCUSSION_THREAD_NUMBER_OF_VISITS_v1(contextVersion, data, size); } },
        };
    }

    void import_ADD_NEW_USER_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_CHANGE_USER_NAME_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_CHANGE_USER_INFO_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_DELETE_USER_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_ADD_NEW_DISCUSSION_THREAD_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_CHANGE_DISCUSSION_THREAD_NAME_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_DELETE_DISCUSSION_THREAD_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_MERGE_DISCUSSION_THREADS_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_SUBSCRIBE_TO_DISCUSSION_THREAD_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_UNSUBSCRIBE_FROM_DISCUSSION_THREAD_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_ADD_NEW_DISCUSSION_THREAD_MESSAGE_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_MOVE_DISCUSSION_THREAD_MESSAGE_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_DELETE_DISCUSSION_THREAD_MESSAGE_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_DISCUSSION_THREAD_MESSAGE_UP_VOTE_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_DISCUSSION_THREAD_MESSAGE_DOWN_VOTE_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_DISCUSSION_THREAD_MESSAGE_RESET_VOTE_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_ADD_COMMENT_TO_DISCUSSION_THREAD_MESSAGE_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_SOLVE_DISCUSSION_THREAD_MESSAGE_COMMENT_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_ADD_NEW_DISCUSSION_TAG_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_CHANGE_DISCUSSION_TAG_NAME_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_CHANGE_DISCUSSION_TAG_UI_BLOB_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_DELETE_DISCUSSION_TAG_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_ADD_DISCUSSION_TAG_TO_THREAD_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_REMOVE_DISCUSSION_TAG_FROM_THREAD_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }
    
    void import_MERGE_DISCUSSION_TAGS_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }
    
    void import_ADD_NEW_DISCUSSION_CATEGORY_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }
    
    void import_CHANGE_DISCUSSION_CATEGORY_NAME_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }
    
    void import_CHANGE_DISCUSSION_CATEGORY_DESCRIPTION_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }
    
    void import_CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }
    
    void import_CHANGE_DISCUSSION_CATEGORY_PARENT_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }
    
    void import_DELETE_DISCUSSION_CATEGORY_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_ADD_DISCUSSION_TAG_TO_CATEGORY_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_REMOVE_DISCUSSION_TAG_FROM_CATEGORY_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    void import_INCREMENT_DISCUSSION_THREAD_NUMBER_OF_VISITS_v1(uint16_t contextVersion, const char* data, size_t size)
    {
        auto context = processContext(contextVersion, data, size);
        (void)context;
    }

    PersistedContext processContext(uint16_t contextVersion, const char*& data, size_t& size)
    {
        //return pointers to context information, increment data and size pointers
        return {};
    }

    ImportStatistic importFile(const std::string& fileName)
    {
        FORUM_LOG_INFO << "Imporing events from: " << fileName;

        try
        {
            auto mappingMode = boost::interprocess::read_only;
            boost::interprocess::file_mapping mapping(fileName.c_str(), mappingMode);
            boost::interprocess::mapped_region region(mapping, mappingMode);
            region.advise(boost::interprocess::mapped_region::advice_sequential);

            return iterateBlobsInFile(reinterpret_cast<const char*>(region.get_address()), region.get_size());
        }
        catch(boost::interprocess::interprocess_exception& ex)
        {
            FORUM_LOG_ERROR << "Error mapping file: " << fileName << '(' << ex.what() << ')';
            return {};
        }
    }

    ImportStatistic iterateBlobsInFile(const char* data, size_t size)
    {
        ImportStatistic statistic{};

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
                break;
            }

            if (verifyChecksum_)
            {
                auto calculatedChecksum = crc32(data, blobSize);
                if (calculatedChecksum != storedChecksum)
                {
                    FORUM_LOG_ERROR << "Checksum mismatch in event blob: " << calculatedChecksum << " != " << storedChecksum;
                    break;
                }
            }

            statistic.readBlobs += 1;            
            if (processEvent(data, blobSize))
            {
                statistic.importedBlobs += 1;
            }

            data += blobSizeWithPadding;
            size -= blobSizeWithPadding;
        }
        
        return statistic;
    }

    bool processEvent(const char* data, size_t size)
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

        fn(contextVersion, data, size);

        return true;
    }

private:
    bool verifyChecksum_;
    std::vector<std::vector<std::function<void(uint16_t, const char*, size_t)>>> importFunctions_;
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

ImportStatistic EventImporter::import(const boost::filesystem::path& sourcePath)
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

    return std::accumulate(eventFileNames.begin(), eventFileNames.end(), ImportStatistic{}, [this](auto& previous, auto& pair)
    {
        return previous + impl_->importFile(pair.second);
    });
}
