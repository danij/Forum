#pragma once

#include "PersistenceBlob.h"

#include <ctime>
#include <string>

#include <boost/noncopyable.hpp>
#include <boost/filesystem.hpp>

namespace Forum
{
    namespace Persistence
    {
        class FileAppender final : private boost::noncopyable
        {
        public:
            FileAppender(boost::filesystem::path destinationFolder, time_t refreshEverySeconds);
            
            void append(const Blob* blobs, size_t nrOfBlobs);

        private:
            void updateCurrentFileIfNeeded();

            boost::filesystem::path destinationFolder_;
            std::string currentFileName_;
            time_t refreshEverySeconds_;
            time_t lastFileNameCreatedAt_;
        };
    }
}