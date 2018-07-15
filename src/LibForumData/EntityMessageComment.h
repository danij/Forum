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
    class DiscussionThreadMessage;

    /**
     * Stores a comment to a discussion thread message
     * Does not get deleted if the message is deleted
     */
    class MessageComment final : boost::noncopyable
    {
    public:
        const auto& id()              const { return id_; }

               auto created()         const { return created_; }
        const auto& creationDetails() const { return creationDetails_; }

        const User& createdBy()       const { return createdBy_; }
        const auto& parentMessage()   const { return message_; }
              auto& parentMessage()         { return message_; }

         StringView content()         const { return content_; }
               bool solved()          const { return solved_; }


        MessageComment(const IdType id, DiscussionThreadMessage& message, User& createdBy, const Timestamp created,
                       const VisitDetails creationDetails)
            : id_(id), created_(created), creationDetails_(creationDetails), createdBy_(createdBy), message_(message)
        {}

        auto& solved()    { return solved_; }
        auto& content()   { return content_; }
        auto& createdBy() { return createdBy_; }

    private:
        IdType id_;
        Timestamp created_{0};
        VisitDetails creationDetails_;

        User& createdBy_;
        DiscussionThreadMessage& message_;

        Helpers::WholeChangeableString content_;

        bool solved_ = false;
    };

    typedef EntityPointer<MessageComment> MessageCommentPtr;
    typedef EntityPointer<const MessageComment> MessageCommentConstPtr;
}
