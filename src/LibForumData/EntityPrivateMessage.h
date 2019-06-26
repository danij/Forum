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

#include "EntityCommonTypes.h"
#include "StringHelpers.h"

#include <boost/noncopyable.hpp>

namespace Forum::Entities
{
    class User;

    /**
     * Stores a message between two users
     * Repositories are responsible for updating the relationships between this message and other entities
     * When cloning a message, the repository needs to reintroduce it in all collections it was part of
     */
    class PrivateMessage final : boost::noncopyable
    {
    public:
        const auto& id()                  const { return id_; }

               auto created()             const { return created_; }
        const auto& creationDetails()     const { return creationDetails_; }

        const auto& source()              const { return source_; }
        const auto& destination()         const { return destination_; }

        const auto& content()             const { return content_; }

        typedef Json::JsonReadyString<> ContentType;

        PrivateMessage(const IdType id, User& source, User& destination, const Timestamp created, 
                       const VisitDetails creationDetails, ContentType&& content)
            : id_(id), created_(created), creationDetails_(creationDetails), source_(source), destination_(destination),
              content_(std::move(content))
        {}

        auto& source()      { return source_; }
        auto& destination() { return destination_; }
        
    private:
        IdType id_;
        Timestamp created_{0};
        VisitDetails creationDetails_;

        User& source_;
        User& destination_;
        ContentType content_;
    };

    typedef PrivateMessage* PrivateMessagePtr;
    typedef const PrivateMessage* PrivateMessageConstPtr;
}
