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
#include "EntityMessageComment.h"

#include <boost/noncopyable.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ranked_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

namespace Forum
{
    namespace Entities
    {
        class MessageCommentCollection final : private boost::noncopyable
        {
        public:
            bool add(MessageCommentPtr comment);
            bool remove(MessageCommentPtr comment);

            auto count()          const { return byId_.size(); }

            auto byId()           const { return Helpers::toConst(byId_); }
            auto byCreated()      const { return Helpers::toConst(byCreated_); }

            auto& byId()      { return byId_; }
            auto& byCreated() { return byCreated_; }

        private:
            HASHED_UNIQUE_COLLECTION(MessageComment, id) byId_;

            RANKED_COLLECTION(MessageComment, created) byCreated_;
        };

        class MessageCommentCollectionLowMemory final : private boost::noncopyable
        {
        public:
            bool add(MessageCommentPtr comment);
            bool remove(MessageCommentPtr comment);

            auto count()          const { return byId_.size(); }

            auto byId()           const { return Helpers::toConst(byId_); }
            auto byCreated()      const { return Helpers::toConst(byCreated_); }

            auto& byId()      { return byId_; }
            auto& byCreated() { return byCreated_; }

        private:
            SORTED_VECTOR_UNIQUE_COLLECTION(MessageComment, id) byId_;

            SORTED_VECTOR_COLLECTION(MessageComment, created) byCreated_;
        };
    }
}
