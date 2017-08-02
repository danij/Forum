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
using namespace Forum::Repository;
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

static constexpr auto uuidBinarySize = boost::uuids::uuid::static_size();
static constexpr auto ipAddressBinarySize = IpAddress::dataSize();

template<>
UuidString readAndIncrementBuffer<UuidString>(const uint8_t*& data, size_t& size)
{
    UuidString result(data);
    data += uuidBinarySize; size -= uuidBinarySize;

    return result;
}

template<>
IpAddress readAndIncrementBuffer<IpAddress>(const uint8_t*& data, size_t& size)
{
    IpAddress result(data);
    data += ipAddressBinarySize; size -= ipAddressBinarySize;

    return result;
}

template<>
StringView readAndIncrementBuffer<StringView>(const uint8_t*& data, size_t& size)
{
    StringView result;

    auto stringSize = readAndIncrementBuffer<BlobSizeType>(data, size);

    if (size < stringSize)
    {
        FORUM_LOG_ERROR << "Could not read string of " << stringSize << " bytes, only " << size << " remaining";
    }
    else
    {
        result = StringView(reinterpret_cast<const char*>(data), stringSize);
    }
    data += stringSize; size -= stringSize;

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

#define CHECK_SIZE(value, expected) \
    if ((value) < (expected)) \
    { \
        FORUM_LOG_ERROR << "Unable to import event of type " << currentEventType_ << ": expected " << expected << " bytes, found only " << value; \
        return false; \
    }

#define CHECK_NONEMPTY_STRING(value) \
    if (value.size() < 1) \
    { \
        FORUM_LOG_ERROR << "Unable to import event of type " << currentEventType_ << ": unexpected empty or incomplete string"; \
        return false; \
    }

#define CHECK_STATUS_CODE(value) \
    if ((StatusCode::OK) != value && (StatusCode::NO_EFFECT != value)) { \
        FORUM_LOG_ERROR << "Unable to import event of type " << currentEventType_ << ": unexpected status code: " << value; \
    }

#define CHECK_READ_ALL_DATA(value) \
    if (0 != value) \
    { \
        FORUM_LOG_ERROR << "Unable to import event of type " << currentEventType_ << ": unexpected " << value << " bytes at end of blob"; \
        return false; \
    }

#define READ_UUID(variable, data, size) \
    CHECK_SIZE(size, uuidBinarySize); \
    auto variable = readAndIncrementBuffer<UuidString>(data, size);

#define READ_STRING(variable, data, size) \
    CHECK_SIZE(size, sizeof(BlobSizeType)); \
    auto variable = readAndIncrementBuffer<StringView>(data, size); \

#define READ_NONEMPTY_STRING(variable, data, size) \
    READ_STRING(variable, data, size) \
    CHECK_NONEMPTY_STRING(variable);

#define READ_TYPE(type, variable, data, size) \
    CHECK_SIZE(size, sizeof(type)); \
    auto variable = readAndIncrementBuffer<type>(data, size);


struct EventImporter::EventImporterImpl final : private boost::noncopyable
{
    explicit EventImporterImpl(bool verifyChecksum, EntityCollection& entityCollection, 
                               DirectWriteRepositoryCollection&& repositories)
        : verifyChecksum_(verifyChecksum), entityCollection_(entityCollection), repositories_(std::move(repositories))
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
            { {/*v0*/}, [this](auto contextVersion, auto data, auto size) { return this->import_CHANGE_DISCUSSION_THREAD_PIN_DISPLAY_ORDER_v1  (contextVersion, data, size); } },
        };
    }

    bool import_ADD_NEW_USER_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(id, data, size);
        READ_NONEMPTY_STRING(auth, data, size);
        READ_NONEMPTY_STRING(userName, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.user->addNewUser(entityCollection_, id, userName, auth).status);
       
        return true;
    }

    bool import_CHANGE_USER_NAME_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(id, data, size);
        READ_NONEMPTY_STRING(newName, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.user->changeUserName(entityCollection_, id, newName));

        return true;
    }

    bool import_CHANGE_USER_INFO_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(id, data, size);
        READ_NONEMPTY_STRING(newInfo, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.user->changeUserInfo(entityCollection_, id, newInfo));
        
        return true;
    }

    bool import_DELETE_USER_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(id, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.user->deleteUser(entityCollection_, id));

        return true;
    }

    bool import_ADD_NEW_DISCUSSION_THREAD_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(id, data, size);
        READ_NONEMPTY_STRING(threadName, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionThread->addNewDiscussionThread(entityCollection_, id, threadName).status);

        return true;
    }

    bool import_CHANGE_DISCUSSION_THREAD_NAME_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(id, data, size);
        READ_NONEMPTY_STRING(threadName, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionThread->changeDiscussionThreadName(entityCollection_, id, threadName));
                        
        return true;
    }

    bool import_DELETE_DISCUSSION_THREAD_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(id, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionThread->deleteDiscussionThread(entityCollection_, id));

        return true;
    }

    bool import_MERGE_DISCUSSION_THREADS_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        
        READ_UUID(fromThreadId, data, size);
        READ_UUID(intoThreadId, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionThread->mergeDiscussionThreads(entityCollection_, fromThreadId, 
                                                                                 intoThreadId));

        return true;
    }

    bool import_SUBSCRIBE_TO_DISCUSSION_THREAD_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(id, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionThread->subscribeToDiscussionThread(entityCollection_, id));

        return true;
    }

    bool import_UNSUBSCRIBE_FROM_DISCUSSION_THREAD_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(id, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionThread->unsubscribeFromDiscussionThread(entityCollection_, id));

        return true;
    }

    bool import_ADD_NEW_DISCUSSION_THREAD_MESSAGE_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(messageId, data, size);
        READ_UUID(parentId, data, size);
        READ_NONEMPTY_STRING(message, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionThreadMessage->addNewDiscussionMessageInThread(entityCollection_, 
                                                                                                 messageId, parentId, 
                                                                                                 message).status);

        return true;
    }

    bool import_CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(messageId, data, size);
        READ_NONEMPTY_STRING(content, data, size);
        READ_STRING(lastUpdatedReason, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionThreadMessage->changeDiscussionThreadMessageContent(entityCollection_,
                                                                                                      messageId, content, 
                                                                                                      lastUpdatedReason));

        return true;
    }

    bool import_MOVE_DISCUSSION_THREAD_MESSAGE_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(messageId, data, size);
        READ_UUID(threadId, data, size);

        CHECK_STATUS_CODE(repositories_.discussionThreadMessage->moveDiscussionThreadMessage(entityCollection_,
                                                                                             messageId, 
                                                                                             threadId));

        CHECK_READ_ALL_DATA(size);
        return true;
    }

    bool import_DELETE_DISCUSSION_THREAD_MESSAGE_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(id, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionThreadMessage->deleteDiscussionMessage(entityCollection_, id));

        return true;
    }

    bool import_DISCUSSION_THREAD_MESSAGE_UP_VOTE_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(id, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionThreadMessage->upVoteDiscussionThreadMessage(entityCollection_, id));

        return true;
    }

    bool import_DISCUSSION_THREAD_MESSAGE_DOWN_VOTE_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(id, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionThreadMessage->downVoteDiscussionThreadMessage(entityCollection_, id));

        return true;
    }

    bool import_DISCUSSION_THREAD_MESSAGE_RESET_VOTE_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(id, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionThreadMessage->resetVoteDiscussionThreadMessage(entityCollection_, id));
        
        return true;
    }

    bool import_ADD_COMMENT_TO_DISCUSSION_THREAD_MESSAGE_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(commentId, data, size);
        READ_UUID(messageId, data, size);
        READ_NONEMPTY_STRING(comment, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionThreadMessage->addCommentToDiscussionThreadMessage(entityCollection_,
                                                                                                     commentId, 
                                                                                                     messageId, 
                                                                                                     comment)
                                                                                                     .status);
        return true;
    }

    bool import_SOLVE_DISCUSSION_THREAD_MESSAGE_COMMENT_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;
        
        READ_UUID(id, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionThreadMessage->setMessageCommentToSolved(entityCollection_, id));
        
        return true;
    }

    bool import_ADD_NEW_DISCUSSION_TAG_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(id, data, size);
        READ_NONEMPTY_STRING(tagName, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionTag->addNewDiscussionTag(entityCollection_, id, tagName).status);

        return true;
    }

    bool import_CHANGE_DISCUSSION_TAG_NAME_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(id, data, size);
        READ_NONEMPTY_STRING(tagName, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionTag->changeDiscussionTagName(entityCollection_, id, tagName));

        return true;
    }

    bool import_CHANGE_DISCUSSION_TAG_UI_BLOB_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(id, data, size);
        READ_NONEMPTY_STRING(uiBlob, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionTag->changeDiscussionTagUiBlob(entityCollection_, id, uiBlob));

        return true;
    }

    bool import_DELETE_DISCUSSION_TAG_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(id, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionTag->deleteDiscussionTag(entityCollection_, id));

        return true;
    }

    bool import_ADD_DISCUSSION_TAG_TO_THREAD_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(tagId, data, size);
        READ_UUID(threadId, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionTag->addDiscussionTagToThread(entityCollection_, tagId, threadId));
        
        return true;
    }

    bool import_REMOVE_DISCUSSION_TAG_FROM_THREAD_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(tagId, data, size);
        READ_UUID(threadId, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionTag->removeDiscussionTagFromThread(entityCollection_, tagId, threadId));

        return true;
    }

    bool import_MERGE_DISCUSSION_TAGS_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(fromTagId, data, size);
        READ_UUID(intoTagId, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionTag->mergeDiscussionTags(entityCollection_, fromTagId, intoTagId));

        return true;
    }

    bool import_ADD_NEW_DISCUSSION_CATEGORY_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(categoryId, data, size);
        READ_UUID(parentId, data, size);
        READ_NONEMPTY_STRING(categoryName, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionCategory->addNewDiscussionCategory(entityCollection_, categoryId,
                                                                                     categoryName, parentId).status);
        return true;
    }

    bool import_CHANGE_DISCUSSION_CATEGORY_NAME_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(id, data, size);
        READ_NONEMPTY_STRING(categoryName, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionCategory->changeDiscussionCategoryName(entityCollection_, id, categoryName));

        return true;
    }

    bool import_CHANGE_DISCUSSION_CATEGORY_DESCRIPTION_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(id, data, size);
        READ_NONEMPTY_STRING(description, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionCategory->changeDiscussionCategoryDescription(entityCollection_, id, 
                                                                                                description));
        return true;
    }

    bool import_CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(id, data, size);
        READ_TYPE(int16_t, displayOrder, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionCategory->changeDiscussionCategoryDisplayOrder(entityCollection_, id, 
                                                                                                 displayOrder));
        return true;
    }

    bool import_CHANGE_DISCUSSION_CATEGORY_PARENT_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(categoryId, data, size);
        READ_UUID(parentId, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionCategory->changeDiscussionCategoryParent(entityCollection_, categoryId,
                                                                                           parentId));
        return true;
    }

    bool import_DELETE_DISCUSSION_CATEGORY_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(id, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionCategory->deleteDiscussionCategory(entityCollection_, id));
        
        return true;
    }

    bool import_ADD_DISCUSSION_TAG_TO_CATEGORY_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(tagId, data, size);
        READ_UUID(categoryId, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionCategory->addDiscussionTagToCategory(entityCollection_, tagId, 
                                                                                       categoryId));
        return true;
    }

    bool import_REMOVE_DISCUSSION_TAG_FROM_CATEGORY_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(tagId, data, size);
        READ_UUID(categoryId, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionCategory->removeDiscussionTagFromCategory(entityCollection_, tagId, 
                                                                                            categoryId));
        return true;
    }

    bool import_INCREMENT_DISCUSSION_THREAD_NUMBER_OF_VISITS_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(threadId, data, size);
        READ_TYPE(uint32_t, nrOfVisits, data, size);
        CHECK_READ_ALL_DATA(size);

        auto it = cachedNrOfThreadVisits_.find(threadId);
        if (it != cachedNrOfThreadVisits_.end())
        {
            it->second += nrOfVisits;
        }
        else
        {
            cachedNrOfThreadVisits_.insert(std::make_pair(threadId, nrOfVisits));
        }

        return true;
    }

    bool import_CHANGE_DISCUSSION_THREAD_PIN_DISPLAY_ORDER_v1(uint16_t contextVersion, const uint8_t* data, size_t size)
    {
        if ( ! processContext(contextVersion, data, size)) return false;

        READ_UUID(id, data, size);
        READ_TYPE(uint16_t, pinDisplayOrder, data, size);
        CHECK_READ_ALL_DATA(size);

        CHECK_STATUS_CODE(repositories_.discussionThread->changeDiscussionThreadPinDisplayOrder(entityCollection_, id, pinDisplayOrder));
        
        return true;
    }

    bool processContext_v1(const uint8_t*& data, size_t& size)
    {
        static constexpr auto expected = sizeof(PersistentTimestampType) + uuidBinarySize + ipAddressBinarySize;
        if (size < expected)
        {
            FORUM_LOG_ERROR << "Unable to import context v1: expected " << expected << " bytes, found only " << size;
            return false;
        }

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

        currentEventType_ = readAndIncrementBuffer<EventType>(data, size);
        auto version = readAndIncrementBuffer<EventVersionType>(data, size);
        auto contextVersion = readAndIncrementBuffer<EventContextVersionType>(data, size);

        if (currentEventType_ >= importFunctions_.size())
        {
            FORUM_LOG_WARNING << "Import for unknown type " << currentEventType_;
            return false;
        }

        auto& importerVersions = importFunctions_[currentEventType_];
        if (version >= importerVersions.size())
        {
            FORUM_LOG_WARNING << "Import for unsupported version " << version << " for event " << currentEventType_;
            return false;
        }

        auto& fn = importerVersions[version];
        if ( ! fn)
        {
            FORUM_LOG_WARNING << "Missing import function for version " << version << " for event " << currentEventType_;
            return false;
        }

        return fn(contextVersion, data, size);
    }

    ImportResult import(const boost::filesystem::path& sourcePath)
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
            auto currentResult = importFile(pair.second);
            if ( ! currentResult.success)
            {
                break;
            }
            result.statistic = result.statistic + currentResult.statistic;
        }

        updateDiscussionThreadVisitCount();

        return result;
    }

    void updateDiscussionThreadVisitCount()
    {
        auto& threads = entityCollection_.threads().byId();
        for (auto& pair : cachedNrOfThreadVisits_)
        {
            auto& id = pair.first;
            auto nrOfVisits = static_cast<int_fast64_t>(pair.second);

            auto it = threads.find(id);
            if (it != threads.end())
            {
                const DiscussionThread& thread = **it;
                thread.visited() += nrOfVisits;
            }
        }
    }

    Timestamp getCurrentTimestamp()
    {
        return currentTimestamp_;
    }

private:
    bool verifyChecksum_;
    EntityCollection& entityCollection_;
    DirectWriteRepositoryCollection repositories_;
    std::vector<std::vector<std::function<bool(uint16_t, const uint8_t*, size_t)>>> importFunctions_;
    Timestamp currentTimestamp_{};
    EventType currentEventType_{};
    std::map<UuidString, uint32_t> cachedNrOfThreadVisits_;
};

EventImporter::EventImporter(bool verifyChecksum, EntityCollection& entityCollection,
                             DirectWriteRepositoryCollection repositories)
    : impl_(new EventImporterImpl(verifyChecksum, entityCollection, std::move(repositories)))
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
    return impl_->import(sourcePath);
}
