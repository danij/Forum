#pragma once

#include <cstddef>
#include <string>

#include <boost/noncopyable.hpp>
#include <boost/filesystem.hpp>

namespace Forum
{
    namespace Persistence
    {
        struct ImportStatistic
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

        struct ImportResult
        {
            ImportStatistic statistic;
            bool success = true;
        };

        class EventImporter final : private boost::noncopyable
        {
        public:
            explicit EventImporter(bool verifyChecksum);
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
