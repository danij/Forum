#pragma once

#include "EntityCommonTypes.h"
#include "StringHelpers.h"

#include <string>

#include <boost/noncopyable.hpp>


namespace Forum
{
    namespace Entities
    {
        class User;
        class DiscussionThreadMessage;

        /**
         * Stores a comment to a discussion thread message
         * Does not get deleted if the message is deleted
         */
        class MessageComment final : private boost::noncopyable
        {
        public:
            const auto& id()              const { return id_; }
            
                   auto created()         const { return created_; }
            const auto& creationDetails() const { return creationDetails_; }

            const User& createdBy()       const { return createdBy_; }
            const auto& parentMessage()   const { return message_; }

             StringView content()         const { return content_; }
                   bool solved()          const { return solved_; }


            MessageComment(IdType id, DiscussionThreadMessage& message, User& createdBy, Timestamp created,
                           VisitDetails creationDetails)
                : id_(std::move(id)), created_(created), creationDetails_(std::move(creationDetails)), 
                  createdBy_(createdBy), message_(message)
            {}

            bool&                     solved()  { return solved_; }
            Helpers::ImmutableString& content() { return content_; }

        private:
            IdType id_;
            Timestamp created_ = 0;
            VisitDetails creationDetails_;
            
            User& createdBy_;
            DiscussionThreadMessage& message_;

            Helpers::ImmutableString content_;

            bool solved_ = false;
        };
        
        typedef EntityPointer<MessageComment> MessageCommentPtr;
        typedef EntityPointer<const MessageComment> MessageCommentConstPtr;
    }
}
