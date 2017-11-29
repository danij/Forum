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

#pragma once

#include "ConstCollectionAdapter.h"
#include "EntityUser.h"

#include <functional>

#include <boost/noncopyable.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ranked_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

namespace Forum
{
    namespace Entities
    {
        class UserCollection final : private boost::noncopyable
        {
        public:
            bool add(UserPtr user);
            bool remove(UserPtr user);

            void stopBatchInsert();

            void prepareUpdateAuth(UserPtr user);
            void updateAuth(UserPtr user);

            void prepareUpdateName(UserPtr user);
            void updateName(UserPtr user);

            void prepareUpdateLastSeen(UserPtr user);
            void updateLastSeen(UserPtr user);

            void prepareUpdateThreadCount(UserPtr user);
            void updateThreadCount(UserPtr user);

            void prepareUpdateMessageCount(UserPtr user);
            void updateMessageCount(UserPtr user);

            auto count()          const { return byId_.size(); }

            auto byId()           const { return Helpers::toConst(byId_); }
            auto byAuth()         const { return Helpers::toConst(byAuth_); }
            auto byName()         const { return Helpers::toConst(byName_); }
            auto byCreated()      const { return Helpers::toConst(byCreated_); }
            auto byLastSeen()     const { return Helpers::toConst(byLastSeen_); }
            auto byThreadCount()  const { return Helpers::toConst(byThreadCount_); }
            auto byMessageCount() const { return Helpers::toConst(byMessageCount_); }

            auto& byId()           { return byId_; }
            auto& byAuth()         { return byAuth_; }
            auto& byName()         { return byName_; }
            auto& byCreated()      { return byCreated_; }
            auto& byLastSeen()     { return byLastSeen_; }
            auto& byThreadCount()  { return byThreadCount_; }
            auto& byMessageCount() { return byMessageCount_; }

        private:
            HASHED_UNIQUE_COLLECTION(User, id) byId_;
            HASHED_UNIQUE_COLLECTION(User, auth) byAuth_;
            decltype(byAuth_)::nth_index<0>::type::iterator byAuthUpdateIt_;

            RANKED_UNIQUE_COLLECTION(User, name) byName_;
            decltype(byName_)::nth_index<0>::type::iterator byNameUpdateIt_;

            RANKED_COLLECTION(User, created) byCreated_;

            RANKED_COLLECTION(User, lastSeen) byLastSeen_;
            decltype(byLastSeen_)::nth_index<0>::type::iterator byLastSeenUpdateIt_;

            RANKED_COLLECTION(User, threadCount) byThreadCount_;
            decltype(byThreadCount_)::nth_index<0>::type::iterator byThreadCountUpdateIt_;

            RANKED_COLLECTION(User, messageCount) byMessageCount_;
            decltype(byMessageCount_)::nth_index<0>::type::iterator byMessageCountUpdateIt_;
        };
    }
}
