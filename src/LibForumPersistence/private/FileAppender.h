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

#include "SeparateThreadConsumer.h"

#include <ctime>
#include <string>

#include <boost/noncopyable.hpp>
#include <boost/filesystem.hpp>

namespace Forum::Persistence
{
    class FileAppender final : boost::noncopyable
    {
    public:
        FileAppender(const boost::filesystem::path& destinationFolder, time_t refreshEverySeconds);

        void append(const Helpers::SeparateThreadConsumerBlob* blobs, size_t nrOfBlobs);

    private:
        void updateCurrentFileIfNeeded();

        boost::filesystem::path destinationFolder_;
        std::string currentFileName_;
        time_t refreshEverySeconds_;
        time_t lastFileNameCreatedAt_;
    };
}
