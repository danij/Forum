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

#pragma once

#include <cstddef>
#include <cstdint>

#include <boost/crc.hpp>

namespace Forum::Persistence
{
    typedef uint64_t MagicPrefixType;

    typedef uint32_t BlobSizeType;
    typedef uint32_t BlobChecksumSizeType;

    typedef uint16_t EventVersionType;
    typedef uint16_t EventContextVersionType;

    typedef int64_t PersistentTimestampType;

    typedef uint16_t PersistentPrivilegeEnumType;
    typedef int16_t PersistentPrivilegeValueType;
    typedef int64_t PersistentPrivilegeDurationType;

    //Changing existing enum members breaks backwards compatibility

    enum EventType : uint32_t
    {
        UNKNOWN = 0,
        ADD_NEW_USER,
        CHANGE_USER_NAME,
        CHANGE_USER_INFO,
        CHANGE_USER_TITLE,
        CHANGE_USER_SIGNATURE,
        CHANGE_USER_LOGO,
        DELETE_USER,

        ADD_NEW_DISCUSSION_THREAD,
        CHANGE_DISCUSSION_THREAD_NAME,
        CHANGE_DISCUSSION_THREAD_PIN_DISPLAY_ORDER,
        DELETE_DISCUSSION_THREAD,
        MERGE_DISCUSSION_THREADS,
        SUBSCRIBE_TO_DISCUSSION_THREAD,
        UNSUBSCRIBE_FROM_DISCUSSION_THREAD,

        ADD_NEW_DISCUSSION_THREAD_MESSAGE,
        CHANGE_DISCUSSION_THREAD_MESSAGE_CONTENT,
        INCREMENT_DISCUSSION_THREAD_NUMBER_OF_VISITS,
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

        CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_THREAD_MESSAGE,
        CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_THREAD,
        CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FOR_TAG,
        CHANGE_DISCUSSION_THREAD_MESSAGE_REQUIRED_PRIVILEGE_FORUM_WIDE,
        CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE_FOR_THREAD,
        CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE_FOR_TAG,
        CHANGE_DISCUSSION_THREAD_REQUIRED_PRIVILEGE_FORUM_WIDE,
        CHANGE_DISCUSSION_TAG_REQUIRED_PRIVILEGE_FOR_TAG,
        CHANGE_DISCUSSION_TAG_REQUIRED_PRIVILEGE_FORUM_WIDE,
        CHANGE_DISCUSSION_CATEGORY_REQUIRED_PRIVILEGE_FOR_CATEGORY,
        CHANGE_DISCUSSION_CATEGORY_REQUIRED_PRIVILEGE_FORUM_WIDE,
        CHANGE_FORUM_WIDE_REQUIRED_PRIVILEGE,
        CHANGE_FORUM_WIDE_DEFAULT_PRIVILEGE_LEVEL,

        ASSIGN_DISCUSSION_THREAD_MESSAGE_PRIVILEGE,
        ASSIGN_DISCUSSION_THREAD_PRIVILEGE,
        ASSIGN_DISCUSSION_TAG_PRIVILEGE,
        ASSIGN_DISCUSSION_CATEGORY_PRIVILEGE,
        ASSIGN_FORUM_WIDE_PRIVILEGE,

        QUOTE_USER_IN_DISCUSSION_THREAD_MESSAGE,
        CHANGE_DISCUSSION_THREAD_MESSAGE_APPROVAL,
        INCREMENT_USER_LATEST_VISITED_PAGE,
        CHANGE_DISCUSSION_THREAD_APPROVAL,
        SEND_PRIVATE_MESSAGE,
        DELETE_PRIVATE_MESSAGE,

        CHANGE_USER_ATTACHMENT_QUOTA,
        ADD_NEW_ATTACHMENT,
        CHANGE_ATTACHMENT_NAME,
        CHANGE_ATTACHMENT_APPROVAL,
        ADD_ATTACHMENT_TO_DISCUSSION_THREAD_MESSAGE,
        REMOVE_ATTACHMENT_FROM_DISCUSSION_THREAD_MESSAGE,
        DELETE_ATTACHMENT,
        INCREMENT_ATTACHMENT_NUMBER_OF_GETS,
    };

    static constexpr MagicPrefixType MagicPrefix = 0xFFFFFFFFFFFFFFFF;
    static constexpr size_t BlobPaddingBytes = 8;

    static constexpr size_t MinBlobSize = sizeof(MagicPrefix) + sizeof(BlobSizeType) + sizeof(BlobChecksumSizeType);
    static constexpr size_t EventHeaderSize = sizeof(EventType) + sizeof(EventVersionType) + sizeof(EventContextVersionType);

    inline BlobChecksumSizeType crc32(const void* buffer, const size_t size)
    {
        boost::crc_32_type hash;
        hash.process_bytes(buffer, size);
        return hash.checksum();
    }

    inline size_t blobPaddingRequired(const size_t size)
    {
        const auto sizeMultiple = (size / BlobPaddingBytes) * BlobPaddingBytes;
        if (sizeMultiple < size)
        {
            return BlobPaddingBytes - (size - sizeMultiple);
        }
        return 0;
    }
}
