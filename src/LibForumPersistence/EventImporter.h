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

#include "EntityCollection.h"
#include "Repository.h"

#include <cstddef>
#include <string>

#include <boost/noncopyable.hpp>
#include <boost/filesystem.hpp>

namespace Forum
{
    namespace Persistence
    {
        struct ImportStatistic final
        {
            size_t readBlobs = 0;
            size_t importedBlobs = 0;

            ImportStatistic operator+(ImportStatistic other) const
            {
                other.readBlobs += readBlobs;
                other.importedBlobs += importedBlobs;

                return other;
            }
        };

        struct ImportResult final
        {
            ImportStatistic statistic;
            bool success = true;
        };

        class EventImporter final : private boost::noncopyable
        {
        public:
            explicit EventImporter(bool verifyChecksum, Entities::EntityCollection& entityCollection,
                                   Repository::DirectWriteRepositoryCollection repositories);
            ~EventImporter();

            /**
             * Imports eventsin chronological order from files found after recursively searching the provided path
             * Files are sorted based on timestamp before import
             *
             * @return Number of events imported
             */
            ImportResult import(const boost::filesystem::path& sourcePath);

        private:
            struct EventImporterImpl;
            EventImporterImpl* impl_ = nullptr;
        };
    }
}
