#pragma once

#include "EntityCommonTypes.h"

#include <boost/noncopyable.hpp>

#include <memory>
#include <string>
#include "StringHelpers.h"

namespace Forum
{
    namespace Entities
    {
        struct User;
        struct DiscussionThreadMessage;

        /**
         * Stores a comment to a discussion thread message
         * Does not get deleted if the message is deleted
         */
        struct MessageComment final : public Identifiable,
                                      public CreatedMixin,
                                      private boost::noncopyable
        {
                  StringView                        content()       const { return content_; }
                  Helpers::ImmutableString&         content()             { return content_; }
            const User&                             createdBy()     const { return createdBy_; }
                  User&                             createdBy()           { return createdBy_; }
                  bool                              solved()        const { return solved_; }
                  bool&                             solved()              { return solved_; }

            std::weak_ptr<DiscussionThreadMessage>& parentMessage()       { return parentMessage_; }

            template<typename TAction>
            void executeActionWithParentMessageIfAvailable(TAction&& action) const
            {
                auto parentMessageShared = parentMessage_.lock();
                if (parentMessageShared)
                {
                    action(const_cast<const DiscussionThreadMessage&>(*parentMessageShared));
                }
            }

            template<typename TAction>
            void executeActionWithParentMessageIfAvailable(TAction&& action)
            {
                auto parentMessageShared = parentMessage_.lock();
                if (parentMessageShared)
                {
                    action(*parentMessageShared);
                }
            }

            explicit MessageComment(User& createdBy) : createdBy_(createdBy), solved_(false)
            {}

        private:
            Helpers::ImmutableString content_;
            User& createdBy_;
            std::weak_ptr<DiscussionThreadMessage> parentMessage_;
            bool solved_;
        };

        typedef std::shared_ptr<MessageComment> MessageCommentRef;
        typedef std::  weak_ptr<MessageComment> MessageCommentWeakRef;
    }
}
