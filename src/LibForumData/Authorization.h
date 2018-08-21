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

#include "TypeHelpers.h"
#include "Entities.h"
#include "AuthorizationPrivileges.h"

#include <memory>

namespace Forum::Authorization
{
    enum class AuthorizationStatus : uint_fast32_t
    {
        OK = 0,
        NOT_ALLOWED,
        THROTTLED
    };

    class IUserAuthorization
    {
    public:
        DECLARE_INTERFACE_MANDATORY(IUserAuthorization)

        virtual AuthorizationStatus login(Entities::IdType userId) const = 0;

        virtual AuthorizationStatus getUsers(const Entities::User& currentUser) const = 0;

        virtual AuthorizationStatus getUserById(const Entities::User& currentUser,
                                                const Entities::User& user) const = 0;
        virtual AuthorizationStatus getUserByName(const Entities::User& currentUser,
                                                  const Entities::User& user) const = 0;

        virtual AuthorizationStatus addNewUser(const Entities::User& currentUser, StringView name) const = 0;
        virtual AuthorizationStatus changeUserName(const Entities::User& currentUser,
                                                   const Entities::User& user, StringView newName) const = 0;
        virtual AuthorizationStatus changeUserInfo(const Entities::User& currentUser,
                                                   const Entities::User& user, StringView newInfo) const = 0;
        virtual AuthorizationStatus changeUserTitle(const Entities::User& currentUser,
                                                    const Entities::User& user, StringView newTitle) const = 0;
        virtual AuthorizationStatus changeUserSignature(const Entities::User& currentUser,
                                                        const Entities::User& user, StringView newSignature) const = 0;
        virtual AuthorizationStatus changeUserLogo(const Entities::User& currentUser,
                                                   const Entities::User& user, StringView newLogo) const = 0;
        virtual AuthorizationStatus deleteUserLogo(const Entities::User& currentUser,
                                                   const Entities::User& user) const = 0;
        virtual AuthorizationStatus deleteUser(const Entities::User& currentUser,
                                               const Entities::User& user) const = 0;
    };
    typedef std::shared_ptr<IUserAuthorization> UserAuthorizationRef;


    class IDiscussionThreadAuthorization
    {
    public:
        DECLARE_INTERFACE_MANDATORY(IDiscussionThreadAuthorization)

        virtual AuthorizationStatus getDiscussionThreadRequiredPrivileges(const Entities::User& currentUser,
                                                                          const Entities::DiscussionThread& thread) const = 0;
        virtual AuthorizationStatus getDiscussionThreadAssignedPrivileges(const Entities::User& currentUser,
                                                                          const Entities::DiscussionThread& thread) const = 0;
        virtual AuthorizationStatus getDiscussionThreads(const Entities::User& currentUser) const = 0;
        virtual AuthorizationStatus getDiscussionThreadById(const Entities::User& currentUser,
                                                            const Entities::DiscussionThread& thread) const = 0;
        virtual AuthorizationStatus getDiscussionThreadSubscribedUsers(const Entities::User& currentUser,
                                                                       const Entities::DiscussionThread& thread) const = 0;

        virtual AuthorizationStatus getDiscussionThreadsOfUser(const Entities::User& currentUser,
                                                               const Entities::User& user) const = 0;
        virtual AuthorizationStatus getSubscribedDiscussionThreadsOfUser(const Entities::User& currentUser,
                                                                         const Entities::User& user) const = 0;

        virtual AuthorizationStatus getDiscussionThreadsWithTag(const Entities::User& currentUser,
                                                                const Entities::DiscussionTag& tag) const = 0;

        virtual AuthorizationStatus getDiscussionThreadsOfCategory(const Entities::User& currentUser,
                                                                   const Entities::DiscussionCategory& category) const = 0;
        virtual AuthorizationStatus getAllowDiscussionThreadPrivilegeChange(const Entities::User& currentUser,
                                                                            const Entities::DiscussionThread& thead) const = 0;

        virtual AuthorizationStatus addNewDiscussionThread(const Entities::User& currentUser,
                                                           StringView name) const = 0;
        virtual AuthorizationStatus changeDiscussionThreadName(const Entities::User& currentUser,
                                                               const Entities::DiscussionThread& thread,
                                                               StringView newName) const = 0;
        virtual AuthorizationStatus changeDiscussionThreadPinDisplayOrder(const Entities::User& currentUser,
                                                                          const Entities::DiscussionThread& thread,
                                                                          uint16_t newValue) const = 0;
        virtual AuthorizationStatus deleteDiscussionThread(const Entities::User& currentUser,
                                                           const Entities::DiscussionThread& thread) const = 0;
        virtual AuthorizationStatus mergeDiscussionThreads(const Entities::User& currentUser,
                                                           const Entities::DiscussionThread& from,
                                                           const Entities::DiscussionThread& into) const = 0;
        virtual AuthorizationStatus subscribeToDiscussionThread(const Entities::User& currentUser,
                                                                const Entities::DiscussionThread& thread) const = 0;
        virtual AuthorizationStatus unsubscribeFromDiscussionThread(const Entities::User& currentUser,
                                                                    const Entities::DiscussionThread& thread) const = 0;

        virtual AuthorizationStatus updateDiscussionThreadMessagePrivilege(const Entities::User& currentUser,
                                                                           const Entities::DiscussionThread& thread,
                                                                           DiscussionThreadMessagePrivilege privilege,
                                                                           PrivilegeValueType oldValue,
                                                                           PrivilegeValueIntType newValue) const = 0;
        virtual AuthorizationStatus updateDiscussionThreadPrivilege(const Entities::User& currentUser,
                                                                    const Entities::DiscussionThread& thread,
                                                                    DiscussionThreadPrivilege privilege,
                                                                    PrivilegeValueType oldValue,
                                                                    PrivilegeValueIntType newValue) const = 0;
        virtual AuthorizationStatus assignDiscussionThreadPrivilege(const Entities::User& currentUser,
                                                                    const Entities::DiscussionThread& thread,
                                                                    const Entities::User& targetUser,
                                                                    PrivilegeValueIntType newValue) const = 0;
    };
    typedef std::shared_ptr<IDiscussionThreadAuthorization> DiscussionThreadAuthorizationRef;


    class IDiscussionThreadMessageAuthorization
    {
    public:
        DECLARE_INTERFACE_MANDATORY(IDiscussionThreadMessageAuthorization)

        virtual AuthorizationStatus getDiscussionThreadMessageRequiredPrivileges(const Entities::User& currentUser,
                                                                                 const Entities::DiscussionThreadMessage& message) const = 0;
        virtual AuthorizationStatus getDiscussionThreadMessageAssignedPrivileges(const Entities::User& currentUser,
                                                                                 const Entities::DiscussionThreadMessage& message) const = 0;
        virtual AuthorizationStatus getDiscussionThreadMessageById(const Entities::User& currentUser,
                                                                   const Entities::DiscussionThreadMessage& message) const = 0;
        virtual AuthorizationStatus getDiscussionThreadMessagesOfUserByCreated(const Entities::User& currentUser,
                                                                               const Entities::User& user) const = 0;
        virtual AuthorizationStatus getDiscussionThreadMessageRank(const Entities::User& currentUser,
                                                                   const Entities::DiscussionThreadMessage& message) const = 0;

        virtual AuthorizationStatus getMessageComments(const Entities::User& currentUser) const = 0;
        virtual AuthorizationStatus getMessageCommentsOfDiscussionThreadMessage(const Entities::User& currentUser,
                                                                                const Entities::DiscussionThreadMessage& message) const = 0;
        virtual AuthorizationStatus getMessageCommentsOfUser(const Entities::User& currentUser,
                                                             const Entities::User& user) const = 0;
        virtual AuthorizationStatus getAllowDiscussionThreadMessagePrivilegeChange(const Entities::User& currentUser,
                                                                                   const Entities::DiscussionThreadMessage& threadMessage) const = 0;

        virtual AuthorizationStatus addNewDiscussionMessageInThread(const Entities::User& currentUser,
                                                                    const Entities::DiscussionThread& thread,
                                                                    StringView content) const = 0;
        virtual AuthorizationStatus deleteDiscussionMessage(const Entities::User& currentUser,
                                                            const Entities::DiscussionThreadMessage& message) const = 0;
        virtual AuthorizationStatus changeDiscussionThreadMessageContent(const Entities::User& currentUser,
                                                                         const Entities::DiscussionThreadMessage& message,
                                                                         StringView newContent,
                                                                         StringView changeReason) const = 0;
        virtual AuthorizationStatus moveDiscussionThreadMessage(const Entities::User& currentUser,
                                                                const Entities::DiscussionThreadMessage& message,
                                                                const Entities::DiscussionThread& intoThread) const = 0;
        virtual AuthorizationStatus upVoteDiscussionThreadMessage(const Entities::User& currentUser,
                                                                  const Entities::DiscussionThreadMessage& message) const = 0;
        virtual AuthorizationStatus downVoteDiscussionThreadMessage(const Entities::User& currentUser,
                                                                    const Entities::DiscussionThreadMessage& message) const = 0;
        virtual AuthorizationStatus resetVoteDiscussionThreadMessage(const Entities::User& currentUser,
                                                                     const Entities::DiscussionThreadMessage& message) const = 0;

        virtual AuthorizationStatus addCommentToDiscussionThreadMessage(const Entities::User& currentUser,
                                                                        const Entities::DiscussionThreadMessage& message,
                                                                        StringView content) const = 0;
        virtual AuthorizationStatus setMessageCommentToSolved(const Entities::User& currentUser,
                                                              const Entities::MessageComment& comment) const = 0;

        virtual AuthorizationStatus updateDiscussionThreadMessagePrivilege(const Entities::User& currentUser,
                                                                           const Entities::DiscussionThreadMessage& message,
                                                                           DiscussionThreadMessagePrivilege privilege,
                                                                           PrivilegeValueType oldValue,
                                                                           PrivilegeValueIntType newValue) const = 0;
        virtual AuthorizationStatus assignDiscussionThreadMessagePrivilege(const Entities::User& currentUser,
                                                                           const Entities::DiscussionThreadMessage& message,
                                                                           const Entities::User& targetUser,
                                                                           PrivilegeValueIntType newValue) const = 0;
    };
    typedef std::shared_ptr<IDiscussionThreadMessageAuthorization> DiscussionThreadMessageAuthorizationRef;


    class IDiscussionTagAuthorization
    {
    public:
        DECLARE_INTERFACE_MANDATORY(IDiscussionTagAuthorization)

        virtual AuthorizationStatus getDiscussionTagRequiredPrivileges(const Entities::User& currentUser,
                                                                       const Entities::DiscussionTag& tag) const = 0;
        virtual AuthorizationStatus getDiscussionTagAssignedPrivileges(const Entities::User& currentUser,
                                                                       const Entities::DiscussionTag& tag) const = 0;
        virtual AuthorizationStatus getDiscussionTagById(const Entities::User& currentUser,
                                                         const Entities::DiscussionTag& tag) const = 0;
        virtual AuthorizationStatus getDiscussionTags(const Entities::User& currentUser) const = 0;
        virtual AuthorizationStatus getAllowDiscussionTagPrivilegeChange(const Entities::User& currentUser,
                                                                         const Entities::DiscussionTag& tag) const = 0;

        virtual AuthorizationStatus addNewDiscussionTag(const Entities::User& currentUser,
                                                        StringView name) const = 0;
        virtual AuthorizationStatus changeDiscussionTagName(const Entities::User& currentUser,
                                                            const Entities::DiscussionTag& tag,
                                                            StringView newName) const = 0;
        virtual AuthorizationStatus changeDiscussionTagUiBlob(const Entities::User& currentUser,
                                                              const Entities::DiscussionTag& tag,
                                                              StringView blob) const = 0;
        virtual AuthorizationStatus deleteDiscussionTag(const Entities::User& currentUser,
                                                        const Entities::DiscussionTag& tag) const = 0;
        virtual AuthorizationStatus addDiscussionTagToThread(const Entities::User& currentUser,
                                                             const Entities::DiscussionTag& tag,
                                                             const Entities::DiscussionThread& thread) const = 0;
        virtual AuthorizationStatus removeDiscussionTagFromThread(const Entities::User& currentUser,
                                                                  const Entities::DiscussionTag& tag,
                                                                  const Entities::DiscussionThread& thread) const = 0;
        virtual AuthorizationStatus mergeDiscussionTags(const Entities::User& currentUser,
                                                        const Entities::DiscussionTag& from,
                                                        const Entities::DiscussionTag& into) const = 0;

        virtual AuthorizationStatus updateDiscussionThreadMessagePrivilege(const Entities::User& currentUser,
                                                                           const Entities::DiscussionTag& tag,
                                                                           DiscussionThreadMessagePrivilege privilege,
                                                                           PrivilegeValueType oldValue,
                                                                           PrivilegeValueIntType newValue) const = 0;
        virtual AuthorizationStatus updateDiscussionThreadPrivilege(const Entities::User& currentUser,
                                                                    const Entities::DiscussionTag& tag,
                                                                    DiscussionThreadPrivilege privilege,
                                                                    PrivilegeValueType oldValue,
                                                                    PrivilegeValueIntType newValue) const = 0;
        virtual AuthorizationStatus updateDiscussionTagPrivilege(const Entities::User& currentUser,
                                                                 const Entities::DiscussionTag& tag,
                                                                 DiscussionTagPrivilege privilege,
                                                                 PrivilegeValueType oldValue,
                                                                 PrivilegeValueIntType newValue) const = 0;
        virtual AuthorizationStatus assignDiscussionTagPrivilege(const Entities::User& currentUser,
                                                                 const Entities::DiscussionTag& tag,
                                                                 const Entities::User& targetUser,
                                                                 PrivilegeValueIntType newValue) const = 0;
    };
    typedef std::shared_ptr<IDiscussionTagAuthorization> DiscussionTagAuthorizationRef;


    class IDiscussionCategoryAuthorization
    {
    public:
        DECLARE_INTERFACE_MANDATORY(IDiscussionCategoryAuthorization)

        virtual AuthorizationStatus getDiscussionCategoryRequiredPrivileges(const Entities::User& currentUser,
                                                                            const Entities::DiscussionCategory& category) const = 0;
        virtual AuthorizationStatus getDiscussionCategoryAssignedPrivileges(const Entities::User& currentUser,
                                                                            const Entities::DiscussionCategory& category) const = 0;
        virtual AuthorizationStatus getDiscussionCategoryById(const Entities::User& currentUser,
                                                              const Entities::DiscussionCategory& category) const = 0;
        virtual AuthorizationStatus getDiscussionCategories(const Entities::User& currentUser) const = 0;
        virtual AuthorizationStatus getDiscussionCategoriesFromRoot(const Entities::User& currentUser) const = 0;
        virtual AuthorizationStatus getAllowDiscussionCategoryPrivilegeChange(const Entities::User& currentUser,
                                                                              const Entities::DiscussionCategory& category) const = 0;

        virtual AuthorizationStatus addNewDiscussionCategory(const Entities::User& currentUser,
                                                             StringView name,
                                                             const Entities::DiscussionCategory* parent) const = 0;
        virtual AuthorizationStatus changeDiscussionCategoryName(const Entities::User& currentUser,
                                                                 const Entities::DiscussionCategory& category,
                                                                 StringView newName) const = 0;
        virtual AuthorizationStatus changeDiscussionCategoryDescription(const Entities::User& currentUser,
                                                                        const Entities::DiscussionCategory& category,
                                                                        StringView newDescription) const = 0;
        virtual AuthorizationStatus changeDiscussionCategoryParent(const Entities::User& currentUser,
                                                                   const Entities::DiscussionCategory& category,
                                                                   const Entities::DiscussionCategory* newParent) const = 0;
        virtual AuthorizationStatus changeDiscussionCategoryDisplayOrder(const Entities::User& currentUser,
                                                                         const Entities::DiscussionCategory& category,
                                                                         int_fast16_t newDisplayOrder) const = 0;
        virtual AuthorizationStatus deleteDiscussionCategory(const Entities::User& currentUser,
                                                             const Entities::DiscussionCategory& category) const = 0;
        virtual AuthorizationStatus addDiscussionTagToCategory(const Entities::User& currentUser,
                                                               const Entities::DiscussionTag& tag,
                                                               const Entities::DiscussionCategory& category) const = 0;
        virtual AuthorizationStatus removeDiscussionTagFromCategory(const Entities::User& currentUser,
                                                                    const Entities::DiscussionTag& tag,
                                                                    const Entities::DiscussionCategory& category) const = 0;

        virtual AuthorizationStatus updateDiscussionCategoryPrivilege(const Entities::User& currentUser,
                                                                      const Entities::DiscussionCategory& category,
                                                                      DiscussionCategoryPrivilege privilege,
                                                                      PrivilegeValueType oldValue,
                                                                      PrivilegeValueIntType newValue) const = 0;
        virtual AuthorizationStatus assignDiscussionCategoryPrivilege(const Entities::User& currentUser,
                                                                      const Entities::DiscussionCategory& category,
                                                                      const Entities::User& targetUser,
                                                                      PrivilegeValueIntType newValue) const = 0;
    };
    typedef std::shared_ptr<IDiscussionCategoryAuthorization> DiscussionCategoryAuthorizationRef;

    class IForumWideAuthorization
    {
    public:
        DECLARE_INTERFACE_MANDATORY(IForumWideAuthorization)

        virtual AuthorizationStatus updateDiscussionThreadMessagePrivilege(const Entities::User& currentUser,
                                                                           DiscussionThreadMessagePrivilege privilege,
                                                                           PrivilegeValueType oldValue,
                                                                           PrivilegeValueIntType newValue) const = 0;
        virtual AuthorizationStatus updateDiscussionThreadPrivilege(const Entities::User& currentUser,
                                                                    DiscussionThreadPrivilege privilege,
                                                                    PrivilegeValueType oldValue,
                                                                    PrivilegeValueIntType newValue) const = 0;
        virtual AuthorizationStatus updateDiscussionTagPrivilege(const Entities::User& currentUser,
                                                                 DiscussionTagPrivilege privilege,
                                                                 PrivilegeValueType oldValue,
                                                                 PrivilegeValueIntType newValue) const = 0;
        virtual AuthorizationStatus updateDiscussionCategoryPrivilege(const Entities::User& currentUser,
                                                                      DiscussionCategoryPrivilege privilege,
                                                                      PrivilegeValueType oldValue,
                                                                      PrivilegeValueIntType newValue) const = 0;
        virtual AuthorizationStatus updateForumWidePrivilege(const Entities::User& currentUser,
                                                             ForumWidePrivilege privilege,
                                                             PrivilegeValueType oldValue,
                                                             PrivilegeValueIntType newValue) const = 0;
        virtual AuthorizationStatus updateForumWideDefaultPrivilegeLevel(
                const Entities::User& currentUser, ForumWideDefaultPrivilegeDuration privilege,
                PrivilegeValueIntType newValue, PrivilegeDurationIntType newDuration) const = 0;

        virtual AuthorizationStatus getForumWideRequiredPrivileges(const Entities::User& currentUser) const = 0;
        virtual AuthorizationStatus getForumWideAssignedPrivileges(const Entities::User& currentUser) const = 0;
        virtual AuthorizationStatus getUserAssignedPrivileges(const Entities::User& currentUser,
                                                              const Entities::User& targetUser) const = 0;
        virtual AuthorizationStatus getAllowForumWidePrivilegeChange(const Entities::User& currentUser) const = 0;

        virtual AuthorizationStatus assignForumWidePrivilege(const Entities::User& currentUser,
                                                             const Entities::User& targetUser,
                                                             PrivilegeValueIntType newValue) const = 0;
    };
    typedef std::shared_ptr<IForumWideAuthorization> ForumWideAuthorizationRef;

    class IStatisticsAuthorization
    {
    public:
        DECLARE_INTERFACE_MANDATORY(IStatisticsAuthorization)

        virtual AuthorizationStatus getEntitiesCount(const Entities::User& currentUser) const = 0;
    };
    typedef std::shared_ptr<IStatisticsAuthorization> StatisticsAuthorizationRef;


    class IMetricsAuthorization
    {
    public:
        DECLARE_INTERFACE_MANDATORY(IMetricsAuthorization)

        virtual AuthorizationStatus getVersion(const Entities::User& currentUser) const = 0;
    };
    typedef std::shared_ptr<IMetricsAuthorization> MetricsAuthorizationRef;
}
