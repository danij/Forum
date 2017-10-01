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

        struct ObserverContext_ final
        {
            PerformedByType performedBy;
            const Entities::Timestamp timestamp;
            const Context::DisplayContext displayContext;
            const Helpers::IpAddress ipAddress;

            ObserverContext_(PerformedByType performedBy, Entities::Timestamp timestamp,
                            Context::DisplayContext displayContext, Helpers::IpAddress ipAddress) :
                    performedBy(performedBy), timestamp(timestamp), displayContext(displayContext), ipAddress(ipAddress)
            {
            }
        };

        //Do not polute all observer methods with const Struct&
        typedef const ObserverContext_& ObserverContext;

        struct ReadEvents final : private boost::noncopyable
        {
            boost::signals2::signal<void(ObserverContext)> onGetEntitiesCount;

            boost::signals2::signal<void(ObserverContext)> onGetUsers;
            boost::signals2::signal<void(ObserverContext)> onGetUsersOnline;
            boost::signals2::signal<void(ObserverContext, const Entities::User&)> onGetUserById;
            boost::signals2::signal<void(ObserverContext, StringView)> onGetUserByName;
            boost::signals2::signal<void(ObserverContext, const Entities::User&)> onGetUserLogo;
            boost::signals2::signal<void(ObserverContext, const Entities::User&)> onGetUserVoteHistory;

            boost::signals2::signal<void(ObserverContext)> onGetDiscussionThreads;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionThread&)> onGetDiscussionThreadById;
            boost::signals2::signal<void(ObserverContext, const Entities::User&)> onGetDiscussionThreadsOfUser;

            boost::signals2::signal<void(ObserverContext, const Entities::User&)> onGetDiscussionThreadMessagesOfUser;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionThreadMessage&)>
                                                          onGetDiscussionThreadMessageRank;

            boost::signals2::signal<void(ObserverContext)> onGetMessageComments;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionThreadMessage&)>
                                                          onGetMessageCommentsOfMessage;
            boost::signals2::signal<void(ObserverContext, const Entities::User&)> onGetMessageCommentsOfUser;

            boost::signals2::signal<void(ObserverContext)> onGetDiscussionTags;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionTag&)> onGetDiscussionThreadsWithTag;

            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionCategory&)> onGetDiscussionCategory;
            boost::signals2::signal<void(ObserverContext)> onGetDiscussionCategories;
            boost::signals2::signal<void(ObserverContext)> onGetRootDiscussionCategories;
            boost::signals2::signal<void(ObserverContext,
                                         const Entities::DiscussionCategory&)> onGetDiscussionThreadsOfCategory;

            boost::signals2::signal<void(ObserverContext)> onGetForumWideCurrentUserPrivileges;
            boost::signals2::signal<void(ObserverContext)> onGetForumWideRequiredPrivileges;
            boost::signals2::signal<void(ObserverContext,
                                         const Entities::DiscussionThreadMessage&)>
                                                onGetRequiredPrivilegesFromThreadMessage;
            boost::signals2::signal<void(ObserverContext,
                                         const Entities::DiscussionThread&)>
                                                onGetRequiredPrivilegesFromThread;
            boost::signals2::signal<void(ObserverContext,
                                         const Entities::DiscussionTag&)>
                                                onGetRequiredPrivilegesFromTag;
            boost::signals2::signal<void(ObserverContext,
                                         const Entities::DiscussionCategory&)>
                                                onGetRequiredPrivilegesFromCategory;

            boost::signals2::signal<void(ObserverContext)> onGetForumWideDefaultPrivilegeDurations;
            boost::signals2::signal<void(ObserverContext,
                                         const Entities::DiscussionThread&)>
                                                onGetDefaultPrivilegeDurationsFromThread;
            boost::signals2::signal<void(ObserverContext,
                                         const Entities::DiscussionTag&)>
                                                onGetDefaultPrivilegeDurationsFromTag;

            boost::signals2::signal<void(ObserverContext)> onGetForumWideAssignedPrivileges;
            boost::signals2::signal<void(ObserverContext,
                                         const Entities::User&)> onGetForumWideAssignedPrivilegesForUser;
            boost::signals2::signal<void(ObserverContext,
                                         const Entities::DiscussionThreadMessage&)>
                                                onGetAssignedPrivilegesFromThreadMessage;
            boost::signals2::signal<void(ObserverContext,
                                         const Entities::DiscussionThread&)>
                                                onGetAssignedPrivilegesFromThread;
            boost::signals2::signal<void(ObserverContext,
                                         const Entities::DiscussionTag&)>
                                                onGetAssignedPrivilegesFromTag;
            boost::signals2::signal<void(ObserverContext,
                                         const Entities::DiscussionCategory&)>
                                                onGetAssignedPrivilegesFromCategory;
        };

        struct WriteEvents final : private boost::noncopyable
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
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionThread&)> onSubscribeToDiscussionThread;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionThread&)> onUnsubscribeFromDiscussionThread;

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

            boost::signals2::signal<void(ObserverContext, const Entities::MessageComment&)>
                                         onAddCommentToDiscussionThreadMessage;
            boost::signals2::signal<void(ObserverContext, const Entities::MessageComment&)>
                                         onSolveDiscussionThreadMessageComment;

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

            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionThreadMessage&,
                                         Authorization::DiscussionThreadMessagePrivilege,
                                         Authorization::PrivilegeValueIntType)> changeDiscussionThreadMessageRequiredPrivilegeForThreadMessage;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionThread&,
                                         Authorization::DiscussionThreadMessagePrivilege,
                                         Authorization::PrivilegeValueIntType)> changeDiscussionThreadMessageRequiredPrivilegeForThread;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionTag&,
                                         Authorization::DiscussionThreadMessagePrivilege,
                                         Authorization::PrivilegeValueIntType)> changeDiscussionThreadMessageRequiredPrivilegeForTag;
            boost::signals2::signal<void(ObserverContext,
                                         Authorization::DiscussionThreadMessagePrivilege,
                                         Authorization::PrivilegeValueIntType)> changeDiscussionThreadMessageRequiredPrivilegeForumWide;

            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionThread&,
                                         Authorization::DiscussionThreadPrivilege,
                                         Authorization::PrivilegeValueIntType)> changeDiscussionThreadRequiredPrivilegeForThread;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionTag&,
                                         Authorization::DiscussionThreadPrivilege,
                                         Authorization::PrivilegeValueIntType)> changeDiscussionThreadRequiredPrivilegeForTag;
            boost::signals2::signal<void(ObserverContext,
                                         Authorization::DiscussionThreadPrivilege,
                                         Authorization::PrivilegeValueIntType)> changeDiscussionThreadRequiredPrivilegeForumWide;

            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionTag&,
                                         Authorization::DiscussionTagPrivilege,
                                         Authorization::PrivilegeValueIntType)> changeDiscussionTagRequiredPrivilegeForTag;
            boost::signals2::signal<void(ObserverContext,
                                         Authorization::DiscussionTagPrivilege,
                                         Authorization::PrivilegeValueIntType)> changeDiscussionTagRequiredPrivilegeForumWide;

            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionCategory&,
                                         Authorization::DiscussionCategoryPrivilege,
                                         Authorization::PrivilegeValueIntType)> changeDiscussionCategoryRequiredPrivilegeForCategory;
            boost::signals2::signal<void(ObserverContext,
                                         Authorization::DiscussionCategoryPrivilege,
                                         Authorization::PrivilegeValueIntType)> changeDiscussionCategoryRequiredPrivilegeForumWide;

            boost::signals2::signal<void(ObserverContext,
                                         Authorization::ForumWidePrivilege,
                                         Authorization::PrivilegeValueIntType)> changeForumWideRequiredPrivilege;

            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionThread&,
                                         Authorization::DiscussionThreadMessageDefaultPrivilegeDuration,
                                         Authorization::PrivilegeDefaultDurationIntType)> changeDiscussionThreadMessageDefaultPrivilegeDurationForThread;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionTag&,
                                         Authorization::DiscussionThreadMessageDefaultPrivilegeDuration,
                                         Authorization::PrivilegeDefaultDurationIntType)> changeDiscussionThreadMessageDefaultPrivilegeDurationForTag;
            boost::signals2::signal<void(ObserverContext,
                                         Authorization::DiscussionThreadMessageDefaultPrivilegeDuration,
                                         Authorization::PrivilegeDefaultDurationIntType)> changeDiscussionThreadMessageDefaultPrivilegeDurationForumWide;

            boost::signals2::signal<void(ObserverContext,
                                         Authorization::ForumWideDefaultPrivilegeDuration,
                                         Authorization::PrivilegeDefaultDurationIntType)> changeForumWideDefaultPrivilegeDuration;

            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionThreadMessage&,
                                         const Entities::User&, Authorization::DiscussionThreadMessagePrivilege,
                                         Authorization::PrivilegeValueIntType,
                                         Authorization::PrivilegeDefaultDurationIntType)> assignDiscussionThreadMessagePrivilegeForThreadMessage;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionThread&,
                                         const Entities::User&, Authorization::DiscussionThreadMessagePrivilege,
                                         Authorization::PrivilegeValueIntType,
                                         Authorization::PrivilegeDefaultDurationIntType)> assignDiscussionThreadMessagePrivilegeForThread;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionTag&,
                                         const Entities::User&, Authorization::DiscussionThreadMessagePrivilege,
                                         Authorization::PrivilegeValueIntType,
                                         Authorization::PrivilegeDefaultDurationIntType)> assignDiscussionThreadMessagePrivilegeForTag;
            boost::signals2::signal<void(ObserverContext,
                                         const Entities::User&, Authorization::DiscussionThreadMessagePrivilege,
                                         Authorization::PrivilegeValueIntType,
                                         Authorization::PrivilegeDefaultDurationIntType)> assignDiscussionThreadMessagePrivilegeForumWide;

            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionThread&,
                                         const Entities::User&, Authorization::DiscussionThreadPrivilege,
                                         Authorization::PrivilegeValueIntType,
                                         Authorization::PrivilegeDefaultDurationIntType)> assignDiscussionThreadPrivilegeForThread;
            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionTag&,
                                         const Entities::User&, Authorization::DiscussionThreadPrivilege,
                                         Authorization::PrivilegeValueIntType,
                                         Authorization::PrivilegeDefaultDurationIntType)> assignDiscussionThreadPrivilegeForTag;
            boost::signals2::signal<void(ObserverContext,
                                         const Entities::User&, Authorization::DiscussionThreadPrivilege,
                                         Authorization::PrivilegeValueIntType,
                                         Authorization::PrivilegeDefaultDurationIntType)> assignDiscussionThreadPrivilegeForumWide;

            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionTag&,
                                         const Entities::User&, Authorization::DiscussionTagPrivilege,
                                         Authorization::PrivilegeValueIntType,
                                         Authorization::PrivilegeDefaultDurationIntType)> assignDiscussionTagPrivilegeForTag;
            boost::signals2::signal<void(ObserverContext,
                                         const Entities::User&, Authorization::DiscussionTagPrivilege,
                                         Authorization::PrivilegeValueIntType,
                                         Authorization::PrivilegeDefaultDurationIntType)> assignDiscussionTagPrivilegeForumWide;

            boost::signals2::signal<void(ObserverContext, const Entities::DiscussionCategory&,
                                         const Entities::User&, Authorization::DiscussionCategoryPrivilege,
                                         Authorization::PrivilegeValueIntType,
                                         Authorization::PrivilegeDefaultDurationIntType)> assignDiscussionCategoryPrivilegeForCategory;
            boost::signals2::signal<void(ObserverContext,
                                         const Entities::User&, Authorization::DiscussionCategoryPrivilege,
                                         Authorization::PrivilegeValueIntType,
                                         Authorization::PrivilegeDefaultDurationIntType)> assignDiscussionCategoryPrivilegeForumWide;

            boost::signals2::signal<void(ObserverContext,
                                         const Entities::User&, Authorization::ForumWidePrivilege,
                                         Authorization::PrivilegeValueIntType,
                                         Authorization::PrivilegeDefaultDurationIntType)> assignForumWidePrivilege;
        };
    }
}
