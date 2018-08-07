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

#include "FileAppender.h"
#include "PersistenceFormat.h"
#include "TypeHelpers.h"
#include "Logging.h"

#include <cstdio>
#include <cstddef>
#include <chrono>
#include <stdexcept>
#include <string>

using namespace Forum;
using namespace Forum::Persistence;
using namespace Forum::Helpers;

FileAppender::FileAppender(const boost::filesystem::path& destinationFolder, time_t refreshEverySeconds)
    : destinationFolder_(destinationFolder), refreshEverySeconds_(refreshEverySeconds), lastFileNameCreatedAt_(0)
{
    if ( ! boost::filesystem::is_directory(destinationFolder))
    {
        throw std::runtime_error("The destination folder does not exist or is not a folder");
    }
}

static constexpr uint8_t Padding[8] = { 0 };

template<typename T>
static void writeOrAbort(FILE* file, const T* elements, const size_t nrOfElements)
{
    fwrite(elements, sizeof(T), nrOfElements, file);
    if (ferror(file))
    {
        FORUM_LOG_ERROR << "Could not persist blob to file";
        std::abort();
    }
}

void FileAppender::append(const SeparateThreadConsumerBlob* blobs, const size_t nrOfBlobs)
{
    if (nrOfBlobs < 1)
    {
        return;
    }

    updateCurrentFileIfNeeded();

    const auto file = fopen(currentFileName_.c_str(), "ab");
    if ( ! file)
    {
        FORUM_LOG_ERROR << "Could not open file for writing: " << currentFileName_;
        std::abort();
        return;
    }

    static constexpr size_t prefixSize = sizeof(MagicPrefix) + sizeof(uint32_t) + sizeof(uint32_t);
    static thread_local char prefixBuffer[prefixSize];

    for (size_t i = 0; i < nrOfBlobs; ++i)
    {
        auto& blob = blobs[i];
        const auto blobSize = static_cast<BlobSizeType>(blob.size);
        const auto blobCRC32 = crc32(blob.buffer, blob.size);

        auto prefix = prefixBuffer;

        writeValue(prefix, MagicPrefix); prefix += sizeof(MagicPrefix);
        writeValue(prefix, blobSize); prefix += sizeof(blobSize);
        writeValue(prefix, blobCRC32);

        writeOrAbort(file, prefixBuffer, prefixSize);

        writeOrAbort(file, blob.buffer, blobSize);

        const auto paddingNeeded = blobPaddingRequired(blobSize);
        if (paddingNeeded)
        {
            writeOrAbort(file, Padding, paddingNeeded);
        }
    }
    fclose(file);
}

void FileAppender::updateCurrentFileIfNeeded()
{
    const auto now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    if ((lastFileNameCreatedAt_ + refreshEverySeconds_) < now)
    {
        const auto newFile = "forum-" + std::to_string(now) + ".events";
        currentFileName_ = (destinationFolder_ / newFile).string();
        lastFileNameCreatedAt_ = now;
    }
}
