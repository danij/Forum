/*
Fast Forum Backend
Copyright (C) 2016-2017 Daniel Jurcau

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

void FileAppender::append(const Blob* blobs, size_t nrOfBlobs)
{
    if (nrOfBlobs < 1)
    {
        return;
    }

    updateCurrentFileIfNeeded();

    auto file = fopen(currentFileName_.c_str(), "ab");
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
        auto blobSize = static_cast<BlobSizeType>(blob.size);
        auto blobCRC32 = crc32(blob.buffer, blob.size);

        auto prefix = prefixBuffer;

        writeValue(prefix, MagicPrefix); prefix += sizeof(MagicPrefix);
        writeValue(prefix, blobSize); prefix += sizeof(blobSize);
        writeValue(prefix, blobCRC32);

        fwrite(prefixBuffer, 1, prefixSize, file);

        fwrite(blob.buffer, 1, blobSize, file);

        auto paddingNeeded = blobPaddingRequired(blobSize);
        if (paddingNeeded)
        {
            fwrite(Padding, 1, paddingNeeded, file);
        }

        if (ferror(file))
        {
            FORUM_LOG_ERROR << "Could not persist blob to file";
            std::abort();
        }
    }
    fclose(file);
}

void FileAppender::updateCurrentFileIfNeeded()
{
    auto now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    if ((lastFileNameCreatedAt_ + refreshEverySeconds_) < now)
    {
        auto newFile = "forum-" + std::to_string(now) + ".events";
        currentFileName_ = (destinationFolder_ / newFile).string();
        lastFileNameCreatedAt_ = now;
    }
}
