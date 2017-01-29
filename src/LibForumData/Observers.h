#pragma once

#include "Entities.h"
#include "TypeHelpers.h"
#include "ContextProviders.h"

#include <boost/signals2.hpp>

namespace Forum
{
    namespace Repository
    {
        typedef const Entities::User& PerformedByType;

        struct ObserverContext_
        {
            PerformedByType performedBy;
            const Entities::Timestamp timestamp;
            const Context::DisplayContext displayContext;

            ObserverContext_(PerformedByType performedBy, Entities::Timestamp timestamp, 
                            Context::DisplayContext displayContext) :
                    performedBy(performedBy), timestamp(timestamp), displayContext(displayContext)
            {
            }
        };

        //Do not polute all observer methods with const Struct&
        typedef const ObserverContext_& ObserverContext;

        struct ReadEvents : private boost::noncopyable
        {            
            boost::signals2::signal<void(ObserverContext)> onGetEntitiesCount;

            boost::signals2::signal<void(ObserverContext)> onGetUsers;
            boost::signals2::signal<void(ObserverContext, const Entities::IdType&)> onGetUserById;
            boost::signals2::signal<void(ObserverContext, const std::string&)> onGetUserByName;

            boost::signals2::signal<void(ObserverContext)> onGetDiscussionThreads;
            boost::signals2::signal<void(ObserverContext, const Entities::IdType&)> onGetDiscussionThreadById;
            boost::signals2::signal<void(ObserverContext, const Entities::User&)> onGetDiscussionThreadsOfUser;

            boost::signals2::signal<void(ObserverContext, const Entities::User&)> onGetDiscussionThreadMessagesOfUser;

            boost::signals2::signal<void(ObserverContext)> onGetDiscussionTags;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionTag&)> onGetDiscussionThreadsWithTag;

            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionCategory&)> onGetDiscussionCategory;
            boost::signals2::signal<void(ObserverContext)> onGetDiscussionCategories;
            boost::signals2::signal<void(ObserverContext)> onGetRootDiscussionCategories;
            boost::signals2::signal<void(ObserverContext,
                                         const Entities::DiscussionCategory&)> onGetDiscussionThreadsOfCategory;
        };
        
        struct WriteEvents : private boost::noncopyable
        {
            boost::signals2::signal<void(ObserverContext, const Entities::User&)> onAddNewUser;
            boost::signals2::signal<void(ObserverContext, const Entities::User&, Entities::User::ChangeType)> onChangeUser;
            boost::signals2::signal<void(ObserverContext, const Entities::User&)> onDeleteUser;

            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionThread&)> onAddNewDiscussionThread;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionThread&, 
                                         Entities::DiscussionThread::ChangeType)> onChangeDiscussionThread;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionThread&)> onDeleteDiscussionThread;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionThread& fromThread, 
                                         const Entities::DiscussionThread& toThread)> onMergeDiscussionThreads;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionThreadMessage& message, 
                                         const Entities::DiscussionThread& intoThread)> onMoveDiscussionThreadMessage;

            boost::signals2::signal<void(ObserverContext, 
                                         const Entities::DiscussionThreadMessage&)> onAddNewDiscussionThreadMessage;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionThreadMessage&, 
                                         Entities::DiscussionThreadMessage::ChangeType)> onChangeDiscussionThreadMessage;
            boost::signals2::signal<void(ObserverContext, 
                                         const Entities::DiscussionThreadMessage&)> onDeleteDiscussionThreadMessage;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionThreadMessage&)> 
                                         onDiscussionThreadMessageUpVote;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionThreadMessage&)>
                                         onDiscussionThreadMessageDownVote;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionThreadMessage&)>
                                         onDiscussionThreadMessageResetVote;

            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionTag&)> onAddNewDiscussionTag;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionTag&, 
                                         Entities::DiscussionTag::ChangeType)> onChangeDiscussionTag;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionTag&)> onDeleteDiscussionTag;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionTag& tag, 
                                         const Entities::DiscussionThread& thread)> onAddDiscussionTagToThread;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionTag& tag, 
                                         const Entities::DiscussionThread& thread)> onRemoveDiscussionTagFromThread;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionTag& fromTag, 
                                         const Entities::DiscussionTag& toTag)> onMergeDiscussionTags;

            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionCategory&)> onAddNewDiscussionCategory;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionCategory&, 
                                         Entities::DiscussionCategory::ChangeType)> onChangeDiscussionCategory;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionCategory&)> onDeleteDiscussionCategory;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionTag& tag, 
                                         const Entities::DiscussionCategory& category)> onAddDiscussionTagToCategory;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionTag& tag, 
                                         const Entities::DiscussionCategory& category)> onRemoveDiscussionTagFromCategory;
        };
    }
}
