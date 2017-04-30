#pragma once

#include "Observers.h"

#include <ctime>
#include <boost/filesystem.hpp>

#include <boost/noncopyable.hpp>

namespace Forum
{
    namespace Persistence
    {
        class EventObserver final : private boost::noncopyable
        {
        public:
            EventObserver(Repository::ReadEvents& readEvents, Repository::WriteEvents& writeEvents,
                          const boost::filesystem::path& destinationFolder, time_t refreshEverySeconds);
            ~EventObserver();

        private:
            struct EventObserverImpl;
            EventObserverImpl* impl_;
        };
    }
}
