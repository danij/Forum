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

#include "ConstCollectionAdapter.h"
#include "EntityCommonTypes.h"
#include "StringHelpers.h"

#include <boost/noncopyable.hpp>
#include <boost/container/flat_set.hpp>

#include <atomic>

namespace Forum::Entities
{
    class User;
    class DiscussionThreadMessage;

    class Attachment final : boost::noncopyable
    {
    public:
        const auto& id()                  const { return id_; }

               auto created()             const { return created_; }
        const auto& creationDetails()     const { return creationDetails_; }

        const auto& createdBy()           const { return createdBy_; }
        const auto& name()                const { return name_; }
              auto  size()                const { return size_; }
              auto  approved()            const { return approved_; }

              auto  messages()            const { return Helpers::toConst(messages_); }

        typedef Json::JsonReadyString<16> NameType;

        enum ChangeType : uint32_t
        {
            None = 0,
            Name,
            Approval
        };

        Attachment(const IdType id, const Timestamp created, const VisitDetails creationDetails, 
                   User& createdBy, NameType&& name, const uint64_t size, const bool approved)
            : id_(id), created_(created), creationDetails_(creationDetails), createdBy_(createdBy), 
              name_(std::move(name)), size_(size), approved_(approved)
        {}

        auto& createdBy() { return createdBy_; }
        auto& name()      { return name_; }
        auto& approved()  { return approved_; }
        auto& messages()  { return messages_; }

        auto& nrOfGetRequests() const { return nrOfGetRequests_; }

        bool addMessage(const EntityPointer<DiscussionThreadMessage> messagePtr)
        {
            return messages_.insert(messagePtr).second;
        }

        bool removeMessage(const EntityPointer<DiscussionThreadMessage> messagePtr)
        {
            return messages_.erase(messagePtr) > 0;
        }
        
    private:
        IdType id_;
        Timestamp created_;
        VisitDetails creationDetails_;

        User& createdBy_;
        NameType name_;
        uint64_t size_;
        bool approved_;
        mutable std::atomic<uint32_t> nrOfGetRequests_{ 0 };

        boost::container::flat_set<EntityPointer<DiscussionThreadMessage>> messages_;
    };

    typedef EntityPointer<Attachment> AttachmentPtr;
    typedef EntityPointer<const Attachment> AttachmentConstPtr;
}
