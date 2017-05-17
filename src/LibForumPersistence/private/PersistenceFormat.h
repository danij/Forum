#pragma once

#include <cstddef>
#include <cstdint>

#include <boost/crc.hpp>

namespace Forum
{
    namespace Persistence
    {
        typedef uint64_t MagicPrefixType;

        typedef uint32_t BlobSizeType;
        typedef uint32_t BlobChecksumSizeType;

        typedef uint16_t EventVersionType;
        typedef uint16_t EventContextVersionType;

        typedef int64_t PersistentTimestampType;

        enum EventType : uint32_t
        {
            UNKNOWN = 0,
            ADD_NEW_USER,
            CHANGE_USER_NAME,
            CHANGE_USER_INFO,
            DELETE_USER,
            ADD_NEW_DISCUSSION_THREAD,
            CHANGE_DISCUSSION_THREAD_NAME,
            DELETE_DISCUSSION_THREAD,
            MERGE_DISCUSSION_THREADS,
            SUBSCRIBE_TO_DISCUSSION_THREAD,
            UNSUBSCRIBE_FROM_DISCUSSION_THREAD,
            ADD_NEW_DISCUSSION_THREAD_MESSAGE,
            CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT,
            MOVE_DISCUSSION_THREAD_MESSAGE,
            DELETE_DISCUSSION_THREAD_MESSAGE,
            DISCUSSION_THREAD_MESSAGE_UP_VOTE,
            DISCUSSION_THREAD_MESSAGE_DOWN_VOTE,
            DISCUSSION_THREAD_MESSAGE_RESET_VOTE,
            ADD_COMMENT_TO_DISCUSSION_THREAD_MESSAGE,
            SOLVE_DISCUSSION_THREAD_MESSAGE_COMMENT,
            ADD_NEW_DISCUSSION_TAG,
            CHANGE_DISCUSSION_TAG_NAME,
            CHANGE_DISCUSSION_TAG_UI_BLOB,
            DELETE_DISCUSSION_TAG,
            ADD_DISCUSSION_TAG_TO_THREAD,
            REMOVE_DISCUSSION_TAG_FROM_THREAD,
            MERGE_DISCUSSION_TAGS,
            ADD_NEW_DISCUSSION_CATEGORY,
            CHANGE_DISCUSSION_CATEGORY_NAME,
            CHANGE_DISCUSSION_CATEGORY_DESCRIPTION,
            CHANGE_DISCUSSION_CATEGORY_DISPLAY_ORDER,
            CHANGE_DISCUSSION_CATEGORY_PARENT,
            DELETE_DISCUSSION_CATEGORY,
            ADD_DISCUSSION_TAG_TO_CATEGORY,
            REMOVE_DISCUSSION_TAG_FROM_CATEGORY,

            INCREMENT_DISCUSSION_THREAD_NUMBER_OF_VISITS,
            CHANGE_DISCUSSION_THREAD_PIN_DISPLAY_ORDER
        };

        static constexpr MagicPrefixType MagicPrefix = 0xFFFFFFFFFFFFFFFF;
        static constexpr size_t BlobPaddingBytes = 8;

        static constexpr size_t MinBlobSize = sizeof(MagicPrefix) + sizeof(BlobSizeType) + sizeof(BlobChecksumSizeType);
        static constexpr size_t EventHeaderSize = sizeof(EventType) + sizeof(EventVersionType) + sizeof(EventContextVersionType);

        inline BlobChecksumSizeType crc32(const char* buffer, size_t size)
        {
            boost::crc_32_type hash;
            hash.process_bytes(buffer, size);
            return hash.checksum();
        }

        inline size_t blobPaddingRequired(size_t size)
        {
            auto sizeMultiple = (size / BlobPaddingBytes) * BlobPaddingBytes;
            if (sizeMultiple < size)
            {
                return BlobPaddingBytes - (size - sizeMultiple);
            }
            return 0;
        }
    }
}
