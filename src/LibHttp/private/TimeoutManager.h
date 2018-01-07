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

#include <cassert>
#include <chrono>
#include <ctime>
#include <cstdint>
#include <functional>
#include <mutex>
#include <utility>

#include <boost/noncopyable.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

namespace Http
{
    template<typename T>
    class TimeoutManager final : private boost::noncopyable
    {
    public:

        typedef std::function<void(T)> ReleaseFn;
        typedef int64_t Timestamp;

        explicit TimeoutManager(ReleaseFn&& release, Timestamp defaultTimeout)
            : release_(std::move(release)), defaultTimeout_(defaultTimeout)
        {
            assert(nullptr != release_);
        }

        Timestamp defaultTimeout() const
        {
            return defaultTimeout_;
        }

        void addExpireIn(T element, Timestamp expiresIn)
        {
            addExpireAt(element, getTimeSinceEpoch() + expiresIn);
        }

        void addExpireAt(T element, Timestamp expiresAt)
        {
            std::lock_guard<decltype(mutex_)> lock(mutex_);

            collection_.insert(std::make_pair(element, expiresAt));
        }

        void remove(T element)
        {
            std::lock_guard<decltype(mutex_)> lock(mutex_);

            auto& index = collection_.template get<TimeoutManagerCollectionByElement>();

            auto it = index.find(element);
            if (it == index.end())
            {
                return;
            }
            index.erase(it);
        }

        void checkTimeout()
        {
            checkTimeout(getTimeSinceEpoch());
        }

        void checkTimeout(Timestamp at)
        {
            std::lock_guard<decltype(mutex_)> lock(mutex_);

            auto& index = collection_.template get<TimeoutManagerCollectionByExpirationTime>();

            auto it = index.begin();
            auto upperBound = index.upper_bound(at);

            while (it != upperBound)
            {
                release_(it++->first);
            }

            index.erase(index.begin(), upperBound);
        }

    private:

        typedef std::pair<T, time_t> EntryPair;

        struct TimeoutManagerCollectionByElement {};
        struct TimeoutManagerCollectionByExpirationTime {};

        struct TimeoutManagerCollectionIndices : boost::multi_index::indexed_by<

            boost::multi_index::hashed_unique<boost::multi_index::tag<TimeoutManagerCollectionByElement>,
                    const boost::multi_index::member<EntryPair, typename EntryPair::first_type, &EntryPair::first>>,

            boost::multi_index::ordered_non_unique<boost::multi_index::tag<TimeoutManagerCollectionByExpirationTime>,
                    const boost::multi_index::member<EntryPair, typename EntryPair::second_type, &EntryPair::second>>
        > {};

        typedef boost::multi_index_container<EntryPair, TimeoutManagerCollectionIndices> TimeoutManagerCollection;

        auto getTimeSinceEpoch()
        {
            return static_cast<Timestamp>(std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count());
        }

        TimeoutManagerCollection collection_;
        std::function<void(T)> release_;
        Timestamp defaultTimeout_;
        std::mutex mutex_;
    };
}
