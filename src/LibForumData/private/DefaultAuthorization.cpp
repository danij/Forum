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

#include "DefaultAuthorization.h"

#include "ContextProviders.h"
#include "EntityCollection.h"
#include "TypeHelpers.h"

using namespace Forum;
using namespace Forum::Authorization;
using namespace Forum::Entities;

DefaultAuthorization::DefaultAuthorization(GrantedPrivilegeStore& grantedPrivilegeStore,
                                           ForumWidePrivilegeStore& forumWidePrivilegeStore)
    : DefaultAuthorization(grantedPrivilegeStore, forumWidePrivilegeStore, false)
{
}

DefaultAuthorization::DefaultAuthorization(GrantedPrivilegeStore& grantedPrivilegeStore,
                                           ForumWidePrivilegeStore& forumWidePrivilegeStore, bool disableThrottling)
    : grantedPrivilegeStore_(grantedPrivilegeStore), forumWidePrivilegeStore_(forumWidePrivilegeStore),
      disableThrottling_(disableThrottling)
{
}

AuthorizationStatus DefaultAuthorization::getUsers(const User& currentUser) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::GET_ALL_USERS, with);
}

AuthorizationStatus DefaultAuthorization::getUserById(const User& currentUser, const User& user) const
{
    return AuthorizationStatus::OK;
}

AuthorizationStatus DefaultAuthorization::getUserByName(const User& currentUser, const User& user) const
{
    return AuthorizationStatus::OK;
}

AuthorizationStatus DefaultAuthorization::addNewUser(const User& currentUser, StringView /*name*/) const
{
    if (isThrottled(UserActionThrottling::NEW_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::ADD_USER, with);
}

AuthorizationStatus DefaultAuthorization::changeUserName(const User& currentUser, const User& user, StringView /*newName*/) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    if (currentUser.id() == user.id())
    {
        return isAllowed(&currentUser, ForumWidePrivilege::CHANGE_OWN_USER_NAME, with);
    }
    return isAllowed(&currentUser, ForumWidePrivilege::CHANGE_ANY_USER_NAME, with, &user);
}

AuthorizationStatus DefaultAuthorization::changeUserInfo(const User& currentUser, const User& user, StringView /*newInfo*/) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    if (currentUser.id() == user.id())
    {
        return isAllowed(&currentUser, ForumWidePrivilege::CHANGE_OWN_USER_INFO, with);
    }
    return isAllowed(&currentUser, ForumWidePrivilege::CHANGE_ANY_USER_INFO, with, &user);
}

AuthorizationStatus DefaultAuthorization::changeUserTitle(const User& currentUser, const User& user, StringView /*newTitle*/) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    if (currentUser.id() == user.id())
    {
        return isAllowed(&currentUser, ForumWidePrivilege::CHANGE_OWN_USER_TITLE, with);
    }
    return isAllowed(&currentUser, ForumWidePrivilege::CHANGE_ANY_USER_TITLE, with, &user);
}

AuthorizationStatus DefaultAuthorization::changeUserSignature(const User& currentUser, const User& user,
                                                              StringView /*newSignature*/) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    if (currentUser.id() == user.id())
    {
        return isAllowed(&currentUser, ForumWidePrivilege::CHANGE_OWN_USER_SIGNATURE, with);
    }
    return isAllowed(&currentUser, ForumWidePrivilege::CHANGE_ANY_USER_SIGNATURE, with, &user);
}

AuthorizationStatus DefaultAuthorization::changeUserAttachmentQuota(const User& currentUser, const User& user,
                                                                    uint64_t /*newQuota*/) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::CHANGE_USER_ATTACHMENT_QUOTA, with, &user);
}

AuthorizationStatus DefaultAuthorization::changeUserLogo(const User& currentUser, const User& user,
                                                         StringView /*newLogo*/) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    if (currentUser.id() == user.id())
    {
        return isAllowed(&currentUser, ForumWidePrivilege::CHANGE_OWN_USER_LOGO, with);
    }
    return isAllowed(&currentUser, ForumWidePrivilege::CHANGE_ANY_USER_LOGO, with, &user);
}

AuthorizationStatus DefaultAuthorization::deleteUserLogo(const User& currentUser, const User& user) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    if (currentUser.id() == user.id())
    {
        return isAllowed(&currentUser, ForumWidePrivilege::DELETE_OWN_USER_LOGO, with);
    }
    return isAllowed(&currentUser, ForumWidePrivilege::DELETE_ANY_USER_LOGO, with, &user);
}

AuthorizationStatus DefaultAuthorization::deleteUser(const User& currentUser, const User& user) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::DELETE_ANY_USER, with, &user);
}

AuthorizationStatus DefaultAuthorization::sendPrivateMessage(const User& currentUser, 
                                                             const User& destinationUser,
                                                             StringView /*content*/) const
{
    if (isThrottled(UserActionThrottling::NEW_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    if (currentUser.id() == destinationUser.id()) return AuthorizationStatus::NOT_ALLOWED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::SEND_PRIVATE_MESSAGE, with);
}

AuthorizationStatus DefaultAuthorization::deletePrivateMessage(const User& currentUser, 
                                                               const PrivateMessage& message) const
{
    return (currentUser.id() == message.source().id()) || (currentUser.id() == message.destination().id())
        ? AuthorizationStatus::OK
        : AuthorizationStatus::THROTTLED;
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadRequiredPrivileges(const User& currentUser,
                                                                                const DiscussionThread& thread) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, thread, DiscussionThreadPrivilege::VIEW_REQUIRED_PRIVILEGES, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadAssignedPrivileges(const User& currentUser,
                                                                                const DiscussionThread& thread) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, thread, DiscussionThreadPrivilege::VIEW_ASSIGNED_PRIVILEGES, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreads(const User& currentUser) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::GET_ALL_DISCUSSION_THREADS, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadById(const User& currentUser, const DiscussionThread& thread) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, thread, DiscussionThreadPrivilege::VIEW, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadSubscribedUsers(const User& currentUser,
                                                                             const DiscussionThread& thread) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, thread, DiscussionThreadPrivilege::GET_SUBSCRIBED_USERS, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadsOfUser(const User& currentUser, const User& user) const
{
    if (currentUser.id() == user.id())
    {
        return AuthorizationStatus::OK;
    }
    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::GET_DISCUSSION_THREADS_OF_USER, with);
}

AuthorizationStatus DefaultAuthorization::getSubscribedDiscussionThreadsOfUser(const User& currentUser, const User& user) const
{
    if (currentUser.id() == user.id())
    {
        return AuthorizationStatus::OK;
    }
    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadsWithTag(const User& currentUser, const DiscussionTag& tag) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, tag, DiscussionTagPrivilege::GET_DISCUSSION_THREADS, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadsOfCategory(const User& currentUser,
                                                                         const DiscussionCategory& category) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, category, DiscussionCategoryPrivilege::GET_DISCUSSION_THREADS, with);
}

AuthorizationStatus DefaultAuthorization::getAllowDiscussionThreadPrivilegeChange(const User& currentUser,
                                                                                  const DiscussionThread& thread) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, thread, DiscussionThreadPrivilege::ADJUST_PRIVILEGE, with);
}

AuthorizationStatus DefaultAuthorization::addNewDiscussionThread(const User& currentUser, StringView /*name*/) const
{
    if (isThrottled(UserActionThrottling::NEW_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::ADD_DISCUSSION_THREAD, with);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionThreadName(const User& currentUser,
                                                                     const DiscussionThread& thread,
                                                                     StringView /*newName*/) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, thread, DiscussionThreadPrivilege::CHANGE_NAME, with);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionThreadPinDisplayOrder(const User& currentUser,
                                                                                const DiscussionThread& thread,
                                                                                uint16_t /*newValue*/) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, thread, DiscussionThreadPrivilege::CHANGE_PIN_DISPLAY_ORDER, with);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionThreadApproval(const User& currentUser,
                                                                         const DiscussionThread& thread,
                                                                         bool /*newApproval*/) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, thread, DiscussionThreadPrivilege::CHANGE_APPROVAL, with);
}

AuthorizationStatus DefaultAuthorization::autoApproveDiscussionThread(const User& currentUser) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::AUTO_APPROVE_DISCUSSION_THREAD, with);
}

AuthorizationStatus DefaultAuthorization::deleteDiscussionThread(const User& currentUser, const DiscussionThread& thread) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, thread, DiscussionThreadPrivilege::DELETE, with);
}

AuthorizationStatus DefaultAuthorization::mergeDiscussionThreads(const User& currentUser, const DiscussionThread& from,
                                                                 const DiscussionThread& into) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    return isAllowed(&currentUser, from, into, DiscussionThreadPrivilege::MERGE);
}

AuthorizationStatus DefaultAuthorization::subscribeToDiscussionThread(const User& currentUser,
                                                                      const DiscussionThread& thread) const
{
    if (isAnonymousUser(currentUser)) return AuthorizationStatus::NOT_ALLOWED;

    if (isThrottled(UserActionThrottling::SUBSCRIBE, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, thread, DiscussionThreadPrivilege::SUBSCRIBE, with);
}

AuthorizationStatus DefaultAuthorization::unsubscribeFromDiscussionThread(const User& currentUser,
                                                                          const DiscussionThread& thread) const
{
    if (isAnonymousUser(currentUser)) return AuthorizationStatus::NOT_ALLOWED;

    if (isThrottled(UserActionThrottling::SUBSCRIBE, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, thread, DiscussionThreadPrivilege::UNSUBSCRIBE, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadMessageRequiredPrivileges(const User& currentUser,
                                                                                       const DiscussionThreadMessage& message) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, message, DiscussionThreadMessagePrivilege::VIEW_REQUIRED_PRIVILEGES, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadMessageAssignedPrivileges(const User& currentUser,
                                                                                       const DiscussionThreadMessage& message) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, message, DiscussionThreadMessagePrivilege::VIEW_ASSIGNED_PRIVILEGES, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadMessageById(const User& currentUser,
                                                                         const DiscussionThreadMessage& message) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, message, DiscussionThreadMessagePrivilege::VIEW, with);
}

AuthorizationStatus DefaultAuthorization::getAllowDiscussionThreadMessagePrivilegeChange(const User& currentUser, 
                                                                                         const DiscussionThreadMessage& message) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, message, DiscussionThreadMessagePrivilege::ADJUST_PRIVILEGE, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadMessagesOfUserByCreated(const User& currentUser,
                                                                                     const User& /*user*/) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::GET_DISCUSSION_THREAD_MESSAGES_OF_USER, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadMessageRank(const User& currentUser,
                                                                         const DiscussionThreadMessage& message) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, message, DiscussionThreadMessagePrivilege::VIEW, with);
}

AuthorizationStatus DefaultAuthorization::getMessageComments(const User& currentUser) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::GET_ALL_MESSAGE_COMMENTS, with);
}

AuthorizationStatus DefaultAuthorization::getMessageCommentsOfDiscussionThreadMessage(const User& currentUser,
                                                                                      const DiscussionThreadMessage& message) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, message, DiscussionThreadMessagePrivilege::GET_MESSAGE_COMMENTS, with);
}

AuthorizationStatus DefaultAuthorization::getMessageCommentsOfUser(const User& currentUser, const User& /*user*/) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::GET_MESSAGE_COMMENTS_OF_USER, with);
}

AuthorizationStatus DefaultAuthorization::addNewDiscussionMessageInThread(const User& currentUser,
                                                                          const DiscussionThread& thread,
                                                                          StringView /*content*/) const
{
    if ( ! thread.empty() && isThrottled(UserActionThrottling::NEW_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, thread, DiscussionThreadPrivilege::ADD_MESSAGE, with);
}

AuthorizationStatus DefaultAuthorization::autoApproveDiscussionMessageInThread(const User& currentUser,
                                                                               const DiscussionThread& thread) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, thread, DiscussionThreadPrivilege::AUTO_APPROVE_MESSAGE, with);
}

AuthorizationStatus DefaultAuthorization::deleteDiscussionMessage(const User& currentUser,
                                                                  const DiscussionThreadMessage& message) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, message, DiscussionThreadMessagePrivilege::DELETE, with);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionThreadMessageContent(const User& currentUser,
                                                                               const DiscussionThreadMessage& message,
                                                                               StringView /*newContent*/,
                                                                               StringView /*changeReason*/) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, message, DiscussionThreadMessagePrivilege::CHANGE_CONTENT, with);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionThreadMessageApproval(const User& currentUser,
                                                                                const DiscussionThreadMessage& message,
                                                                                bool /*newApproval*/) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, message, DiscussionThreadMessagePrivilege::CHANGE_APPROVAL, with);
}

AuthorizationStatus DefaultAuthorization::moveDiscussionThreadMessage(const User& currentUser,
                                                                      const DiscussionThreadMessage& message,
                                                                      const DiscussionThread& /*intoThread*/) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, message, DiscussionThreadMessagePrivilege::MOVE, with);
}

AuthorizationStatus DefaultAuthorization::upVoteDiscussionThreadMessage(const User& currentUser,
                                                                        const DiscussionThreadMessage& message) const
{
    if (currentUser.id() == message.createdBy().id()) return AuthorizationStatus::NOT_ALLOWED;
    if (isAnonymousUser(currentUser)) return AuthorizationStatus::NOT_ALLOWED;

    if (isThrottled(UserActionThrottling::VOTE, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, message, DiscussionThreadMessagePrivilege::UP_VOTE, with);
}

AuthorizationStatus DefaultAuthorization::downVoteDiscussionThreadMessage(const User& currentUser,
                                                                          const DiscussionThreadMessage& message) const
{
    if (currentUser.id() == message.createdBy().id()) return AuthorizationStatus::NOT_ALLOWED;
    if (isAnonymousUser(currentUser)) return AuthorizationStatus::NOT_ALLOWED;

    if (isThrottled(UserActionThrottling::VOTE, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, message, DiscussionThreadMessagePrivilege::DOWN_VOTE, with);
}

AuthorizationStatus DefaultAuthorization::resetVoteDiscussionThreadMessage(const User& currentUser,
                                                                           const DiscussionThreadMessage& message) const
{
    if (currentUser.id() == message.createdBy().id()) return AuthorizationStatus::NOT_ALLOWED;
    if (isAnonymousUser(currentUser)) return AuthorizationStatus::NOT_ALLOWED;

    if (isThrottled(UserActionThrottling::VOTE, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, message, DiscussionThreadMessagePrivilege::RESET_VOTE, with);
}

AuthorizationStatus DefaultAuthorization::addCommentToDiscussionThreadMessage(const User& currentUser,
                                                                              const DiscussionThreadMessage& message,
                                                                              StringView /*content*/) const
{
    if (isThrottled(UserActionThrottling::NEW_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, message, DiscussionThreadMessagePrivilege::ADD_COMMENT, with);
}

AuthorizationStatus DefaultAuthorization::setMessageCommentToSolved(const User& currentUser,
                                                                    const MessageComment& comment) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, comment.parentMessage(),
                     DiscussionThreadMessagePrivilege::SET_COMMENT_TO_SOLVED, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionTagRequiredPrivileges(const User& currentUser,
                                                                             const DiscussionTag& tag) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, tag, DiscussionTagPrivilege::VIEW_REQUIRED_PRIVILEGES, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionTagAssignedPrivileges(const User& currentUser,
                                                                             const DiscussionTag& tag) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, tag, DiscussionTagPrivilege::VIEW_ASSIGNED_PRIVILEGES, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionTagById(const User& currentUser,
                                                               const DiscussionTag& tag) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, tag, DiscussionTagPrivilege::VIEW, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionTags(const User& currentUser) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::GET_ALL_DISCUSSION_TAGS, with);
}

AuthorizationStatus DefaultAuthorization::getAllowDiscussionTagPrivilegeChange(const User& currentUser,
                                                                               const DiscussionTag& tag) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, tag, DiscussionTagPrivilege::ADJUST_PRIVILEGE, with);
}

AuthorizationStatus DefaultAuthorization::addNewDiscussionTag(const User& currentUser, StringView /*name*/) const
{
    if (isThrottled(UserActionThrottling::NEW_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::ADD_DISCUSSION_TAG, with);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionTagName(const User& currentUser, const DiscussionTag& tag,
                                                                  StringView /*newName*/) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, tag, DiscussionTagPrivilege::CHANGE_NAME, with);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionTagUiBlob(const User& currentUser, const DiscussionTag& tag,
                                                                    StringView /*blob*/) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, tag, DiscussionTagPrivilege::CHANGE_UIBLOB, with);
}

AuthorizationStatus DefaultAuthorization::deleteDiscussionTag(const User& currentUser, const DiscussionTag& tag) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, tag, DiscussionTagPrivilege::DELETE, with);
}

AuthorizationStatus DefaultAuthorization::addDiscussionTagToThread(const User& currentUser, const DiscussionTag& tag,
                                                                   const DiscussionThread& thread) const
{
    if ( ! thread.empty() && isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;

    const auto allowViewTag = isAllowed(&currentUser, tag, DiscussionTagPrivilege::VIEW, with);
    if (AuthorizationStatus::OK != allowViewTag) return allowViewTag;

    return isAllowed(&currentUser, thread, DiscussionThreadPrivilege::ADD_TAG, with);
}

AuthorizationStatus DefaultAuthorization::removeDiscussionTagFromThread(const User& currentUser, 
                                                                        const DiscussionTag& tag,
                                                                        const DiscussionThread& thread) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;

    const auto allowViewTag = isAllowed(&currentUser, tag, DiscussionTagPrivilege::VIEW, with);
    if (AuthorizationStatus::OK != allowViewTag) return allowViewTag;

    return isAllowed(&currentUser, thread, DiscussionThreadPrivilege::REMOVE_TAG, with);
}

AuthorizationStatus DefaultAuthorization::mergeDiscussionTags(const User& currentUser, const DiscussionTag& from,
                                                              const DiscussionTag& into) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    return isAllowed(&currentUser, from, into, DiscussionTagPrivilege::MERGE);
}

AuthorizationStatus DefaultAuthorization::getDiscussionCategoryRequiredPrivileges(const User& currentUser,
                                                                                  const DiscussionCategory& category) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, category, DiscussionCategoryPrivilege::VIEW_REQUIRED_PRIVILEGES, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionCategoryAssignedPrivileges(const User& currentUser,
                                                                                  const DiscussionCategory& category) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, category, DiscussionCategoryPrivilege::VIEW_ASSIGNED_PRIVILEGES, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionCategoryById(const User& currentUser,
                                                                    const DiscussionCategory& category) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, category, DiscussionCategoryPrivilege::VIEW, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionCategories(const User& currentUser) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::GET_ALL_DISCUSSION_CATEGORIES, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionCategoriesFromRoot(const User& currentUser) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::GET_DISCUSSION_CATEGORIES_FROM_ROOT, with);
}

AuthorizationStatus DefaultAuthorization::getAllowDiscussionCategoryPrivilegeChange(const User& currentUser,
                                                                                    const DiscussionCategory& category) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, category, DiscussionCategoryPrivilege::ADJUST_PRIVILEGE, with);
}

AuthorizationStatus DefaultAuthorization::addNewDiscussionCategory(const User& currentUser, StringView /*name*/,
                                                                   const DiscussionCategory* /*parent*/) const
{
    if (isThrottled(UserActionThrottling::NEW_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::ADD_DISCUSSION_CATEGORY, with);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionCategoryName(const User& currentUser,
                                                                       const DiscussionCategory& category,
                                                                       StringView /*newName*/) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, category, DiscussionCategoryPrivilege::CHANGE_NAME, with);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionCategoryDescription(const User& currentUser,
                                                                              const DiscussionCategory& category,
                                                                              StringView /*newDescription*/) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, category, DiscussionCategoryPrivilege::CHANGE_DESCRIPTION, with);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionCategoryParent(const User& currentUser,
                                                                         const DiscussionCategory& category,
                                                                         const DiscussionCategory* /*newParent*/) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, category, DiscussionCategoryPrivilege::CHANGE_PARENT, with);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionCategoryDisplayOrder(const User& currentUser,
                                                                               const DiscussionCategory& category,
                                                                               int_fast16_t /*newDisplayOrder*/) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, category, DiscussionCategoryPrivilege::CHANGE_DISPLAYORDER, with);
}

AuthorizationStatus DefaultAuthorization::deleteDiscussionCategory(const User& currentUser,
                                                                   const DiscussionCategory& category) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, category, DiscussionCategoryPrivilege::DELETE, with);
}

AuthorizationStatus DefaultAuthorization::addDiscussionTagToCategory(const User& currentUser, const DiscussionTag& tag,
                                                                     const DiscussionCategory& category) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    
    const auto allowViewTag = isAllowed(&currentUser, tag, DiscussionTagPrivilege::VIEW, with);
    if (AuthorizationStatus::OK != allowViewTag) return allowViewTag;

    return isAllowed(&currentUser, category, DiscussionCategoryPrivilege::ADD_TAG, with);
}

AuthorizationStatus DefaultAuthorization::removeDiscussionTagFromCategory(const User& currentUser,
                                                                          const DiscussionTag& tag,
                                                                          const DiscussionCategory& category) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;

    const auto allowViewTag = isAllowed(&currentUser, tag, DiscussionTagPrivilege::VIEW, with);
    if (AuthorizationStatus::OK != allowViewTag) return allowViewTag;


    return isAllowed(&currentUser, category, DiscussionCategoryPrivilege::REMOVE_TAG, with);
}

AuthorizationStatus DefaultAuthorization::getAttachments(const User& currentUser) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::GET_ALL_ATTACHMENTS, with);
}

AuthorizationStatus DefaultAuthorization::getAttachmentsOfUser(const User& currentUser, const User& user) const
{
    if (currentUser.id() == user.id())
    {
        return AuthorizationStatus::OK;
    }
    
    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::GET_ATTACHMENTS_OF_USER, with);
}

bool DefaultAuthorization::checkMessageAllowViewApproval(const User& currentUser,
                                                         const DiscussionThreadMessage& message) const
{
    if (message.approved())
    {
        return true;
    }
    PrivilegeValueType with;
    return (message.createdBy().id() == currentUser.id())
        || (AuthorizationStatus::OK == isAllowed(&currentUser, message, DiscussionThreadMessagePrivilege::VIEW_UNAPPROVED, with));
}

bool DefaultAuthorization::isAllowedToViewMessage(const User& currentUser, const DiscussionThreadMessage& message) const
{
    PrivilegeValueType with;
    return (AuthorizationStatus::OK == isAllowed(&currentUser, message, DiscussionThreadMessagePrivilege::VIEW, with))
        && checkMessageAllowViewApproval(currentUser, message)
        && isAllowedToViewThread(currentUser, *message.parentThread());
}

bool DefaultAuthorization::checkThreadAllowViewApproval(const User& currentUser, const DiscussionThread& thread) const
{
    if (thread.approved())
    {
        return true;
    }
    PrivilegeValueType with;
    return (thread.createdBy().id() == currentUser.id())
        || (AuthorizationStatus::OK == isAllowed(&currentUser, thread, DiscussionThreadPrivilege::VIEW_UNAPPROVED, with));
}

bool DefaultAuthorization::isAllowedToViewThread(const User& currentUser, const DiscussionThread& thread) const
{
    PrivilegeValueType with;
    return (AuthorizationStatus::OK == isAllowed(&currentUser, thread, DiscussionThreadPrivilege::VIEW, with))
        && checkThreadAllowViewApproval(currentUser, thread);
}

AuthorizationStatus DefaultAuthorization::canGetAttachment(const User& currentUser,
                                                           const Attachment& attachment) const
{
    if (currentUser.id() == attachment.createdBy().id())
    {
        return AuthorizationStatus::OK;
    }
    
    PrivilegeValueType with;
    if (AuthorizationStatus::OK == isAllowed(&currentUser, ForumWidePrivilege::GET_ALL_ATTACHMENTS, with))
    {
        return AuthorizationStatus::OK;
    }
    if (AuthorizationStatus::OK == isAllowed(&currentUser, ForumWidePrivilege::GET_ATTACHMENTS_OF_USER, with))
    {
        return AuthorizationStatus::OK;
    }

    for (const DiscussionThreadMessage* messagePtr : attachment.messages())
    {
        const DiscussionThreadMessage& message = *messagePtr;

        if (currentUser.id() == message.createdBy().id())
        {
            return AuthorizationStatus::OK;
        }
        if ((AuthorizationStatus::OK == isAllowed(&currentUser, message, DiscussionThreadMessagePrivilege::VIEW_ATTACHMENT, with))
            && (attachment.approved()
                || (AuthorizationStatus::OK == isAllowed(&currentUser, message, DiscussionThreadMessagePrivilege::VIEW_UNAPPROVED_ATTACHMENT, with)))
            && isAllowedToViewMessage(currentUser, message))
        {
            return AuthorizationStatus::OK;
        }
    }
    return AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::canAddAttachment(const User& currentUser) const
{
    if (isThrottled(UserActionThrottling::NEW_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::CREATE_ATTACHMENT, with);
}

AuthorizationStatus DefaultAuthorization::addNewAttachment(const User& currentUser,
                                                           StringView /*name*/, uint64_t /*size*/) const
{
    return canAddAttachment(currentUser);
}

AuthorizationStatus DefaultAuthorization::autoApproveAttachment(const User& currentUser) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::AUTO_APPROVE_ATTACHMENT, with);
}

AuthorizationStatus DefaultAuthorization::changeAttachmentName(const User& currentUser, const Attachment& attachment,
                                                               StringView /*newName*/) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    if (currentUser.id() == attachment.createdBy().id())
    {
        return isAllowed(&currentUser, ForumWidePrivilege::CHANGE_OWN_ATTACHMENT_NAME, with);
    }
    return isAllowed(&currentUser, ForumWidePrivilege::CHANGE_ANY_ATTACHMENT_NAME, with);
}

AuthorizationStatus DefaultAuthorization::changeAttachmentApproval(const User& currentUser, const Attachment& attachment,
                                                                   bool /*newApproval*/) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::CHANGE_ANY_ATTACHMENT_APPROVAL, with);
}

AuthorizationStatus DefaultAuthorization::deleteAttachment(const User& currentUser, const Attachment& attachment) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    if (currentUser.id() == attachment.createdBy().id())
    {
        return isAllowed(&currentUser, ForumWidePrivilege::DELETE_OWN_ATTACHMENT, with);
    }
    return isAllowed(&currentUser, ForumWidePrivilege::DELETE_ANY_ATTACHMENT, with);
}

AuthorizationStatus DefaultAuthorization::addAttachmentToDiscussionThreadMessage(const User& currentUser,
                                                                                 const Attachment& attachment,
                                                                                 const DiscussionThreadMessage& message) const
{
    if (currentUser.id() != attachment.createdBy().id())
    {
        return AuthorizationStatus::NOT_ALLOWED;
    }

    PrivilegeValueType with;
    return isAllowed(&currentUser, message, DiscussionThreadMessagePrivilege::ADD_ATTACHMENT, with);
}

AuthorizationStatus DefaultAuthorization::removeAttachmentFromDiscussionThreadMessage(const User& currentUser,
                                                                                      const Attachment& attachment,
                                                                                      const DiscussionThreadMessage& message) const
{
    if (isThrottled(UserActionThrottling::EDIT_CONTENT, currentUser)) return AuthorizationStatus::THROTTLED;
    
    PrivilegeValueType with;
    return ((AuthorizationStatus::OK == isAllowed(&currentUser, message, DiscussionThreadMessagePrivilege::REMOVE_ATTACHMENT, with))
        || (AuthorizationStatus::OK == isAllowed(&currentUser, ForumWidePrivilege::DELETE_ANY_ATTACHMENT, with))
        || ((currentUser.id() == attachment.createdBy().id())
            && (AuthorizationStatus::OK == isAllowed(&currentUser, ForumWidePrivilege::DELETE_OWN_ATTACHMENT, with))))
        ? AuthorizationStatus::OK
        : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::getEntitiesCount(const User& currentUser) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::GET_ENTITIES_COUNT, with);
}

AuthorizationStatus DefaultAuthorization::getVersion(const User& currentUser) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::GET_VERSION, with);
}

AuthorizationStatus DefaultAuthorization::isAllowed(UserConstPtr user, const DiscussionThreadMessage& message,
                                                    DiscussionThreadMessagePrivilege privilege, PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(user, message, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(UserConstPtr user, const DiscussionThread& thread,
                                                    DiscussionThreadMessagePrivilege privilege, PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(user, thread, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(UserConstPtr user, const DiscussionTag& tag,
                                                    DiscussionThreadMessagePrivilege privilege, PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(user, tag, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}


AuthorizationStatus DefaultAuthorization::isAllowed(UserConstPtr user, const DiscussionThread& thread,
                                                    DiscussionThreadPrivilege privilege, PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(user, thread, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(UserConstPtr user, const DiscussionTag& tag,
                                                    DiscussionThreadPrivilege privilege, PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(user, tag, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(UserConstPtr user, const DiscussionThread& from,
                                                    const DiscussionThread& into, DiscussionThreadPrivilege privilege) const
{
    return static_cast<bool>(grantedPrivilegeStore_.isAllowed(user, from, privilege, Context::getCurrentTime()))
            && static_cast<bool>(grantedPrivilegeStore_.isAllowed(user, into, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(UserConstPtr user, const DiscussionTag& tag,
                                                    DiscussionTagPrivilege privilege, PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(user, tag, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(UserConstPtr user, const DiscussionTag& from,
                                                    const DiscussionTag& into, DiscussionTagPrivilege privilege) const
{
    return static_cast<bool>(grantedPrivilegeStore_.isAllowed(user, from, privilege, Context::getCurrentTime()))
            && static_cast<bool>(grantedPrivilegeStore_.isAllowed(user, into, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(UserConstPtr user, const DiscussionCategory& category,
                                                    DiscussionCategoryPrivilege privilege, PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(user, category, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(UserConstPtr user, DiscussionThreadMessagePrivilege privilege,
                                                    PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(user, forumWidePrivilegeStore_, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(UserConstPtr user, DiscussionThreadPrivilege privilege,
                                                    PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(user, forumWidePrivilegeStore_, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(UserConstPtr user, DiscussionTagPrivilege privilege,
                                                    PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(user, forumWidePrivilegeStore_, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(UserConstPtr user, DiscussionCategoryPrivilege privilege,
                                                    PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(user, forumWidePrivilegeStore_, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(UserConstPtr user, ForumWidePrivilege privilege,
                                                    PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(user, forumWidePrivilegeStore_, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(UserConstPtr user, ForumWidePrivilege privilege,
                                                    PrivilegeValueType& with, UserConstPtr targetUser) const
{
    with = grantedPrivilegeStore_.isAllowed(user, forumWidePrivilegeStore_, privilege, Context::getCurrentTime());
    if ( ! with) return AuthorizationStatus::NOT_ALLOWED;

    PrivilegeValueType targetUserValuePositive, targetUserValueNegative;
    grantedPrivilegeStore_.calculateForumWidePrivilege(targetUser, Context::getCurrentTime(),
                                                       targetUserValuePositive, targetUserValueNegative);

    return *with > (targetUserValuePositive ? *targetUserValuePositive : 0)
        ? AuthorizationStatus::OK
        : AuthorizationStatus::NOT_ALLOWED;
}

bool DefaultAuthorization::isThrottled(UserActionThrottling action, const User& currentUser) const
{
    return ! disableThrottling_
          && ! grantedPrivilegeStore_.isAllowed(&currentUser, forumWidePrivilegeStore_,
                                                ForumWidePrivilege::NO_THROTTLING, Context::getCurrentTime())
          && throttling_.check(action, Context::getCurrentTime(), currentUser.id(), Context::getCurrentUserIpAddress());
}

static bool allowPrivilegeUpdate(PrivilegeValueType oldValue, PrivilegeValueIntType newValue,
                                 PrivilegeValueType currentPermissions)
{
    const PrivilegeValueIntType currentPermissionsValue = currentPermissions ? *currentPermissions : 0;
    return ((! oldValue) || (std::abs(*oldValue) <= currentPermissionsValue))
        && (std::abs(newValue) <= currentPermissionsValue);
}

static bool allowPrivilegeAssignment(PrivilegeValueType oldValue, PrivilegeValueIntType newValue,
    PrivilegeValueType currentPermissions)
{
    const PrivilegeValueIntType currentPermissionsValue = currentPermissions ? *currentPermissions : 0;
    return (( ! oldValue) || (std::abs(*oldValue) < currentPermissionsValue))
        && (std::abs(newValue) < currentPermissionsValue);
}

AuthorizationStatus DefaultAuthorization::updateDiscussionThreadMessagePrivilege(const User& currentUser,
                                                                                 const DiscussionThreadMessage& message,
                                                                                 DiscussionThreadMessagePrivilege /*privilege*/,
                                                                                 PrivilegeValueType oldValue,
                                                                                 PrivilegeValueIntType newValue) const
{
    if (isThrottled(UserActionThrottling::EDIT_PRIVILEGES, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    const auto status = isAllowed(&currentUser, message, DiscussionThreadMessagePrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

static PrivilegeValueType computeOldValueForAssigningPrivileges(PrivilegeValueType oldValuePositive,
    PrivilegeValueType oldValueNegative)
{
    PrivilegeValueType result{};
    if (oldValuePositive)
    {
        result = oldValuePositive;
    }
    if (oldValueNegative)
    {
        result = std::max(*result, Helpers::absValue(*oldValueNegative));
    }
    return result;
}

AuthorizationStatus DefaultAuthorization::assignDiscussionThreadMessagePrivilege(const User& currentUser,
                                                                                 const DiscussionThreadMessage& message,
                                                                                 const User& targetUser,
                                                                                 PrivilegeValueIntType newValue) const
{
    if (currentUser.id() == targetUser.id()) return AuthorizationStatus::NOT_ALLOWED;

    if (isThrottled(UserActionThrottling::EDIT_PRIVILEGES, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    const auto status = isAllowed(&currentUser, message, DiscussionThreadMessagePrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }

    PrivilegeValueType oldValuePositive, oldValueNegative;
    grantedPrivilegeStore_.calculateDiscussionThreadMessagePrivilege(&targetUser, message,
                                                                     Context::getCurrentTime(),
                                                                     oldValuePositive, oldValueNegative);
    const auto oldValue = computeOldValueForAssigningPrivileges(oldValuePositive, oldValueNegative);

    return allowPrivilegeAssignment(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateDiscussionThreadMessagePrivilege(const User& currentUser,
                                                                                 const DiscussionThread& thread,
                                                                                 DiscussionThreadMessagePrivilege /*privilege*/,
                                                                                 PrivilegeValueType oldValue,
                                                                                 PrivilegeValueIntType newValue) const
{
    if (isThrottled(UserActionThrottling::EDIT_PRIVILEGES, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    const auto status = isAllowed(&currentUser, thread, DiscussionThreadMessagePrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateDiscussionThreadPrivilege(const User& currentUser,
                                                                          const DiscussionThread& thread,
                                                                          DiscussionThreadPrivilege /*privilege*/,
                                                                          PrivilegeValueType oldValue,
                                                                          PrivilegeValueIntType newValue) const
{
    if (isThrottled(UserActionThrottling::EDIT_PRIVILEGES, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    const auto status = isAllowed(&currentUser, thread, DiscussionThreadPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::assignDiscussionThreadPrivilege(const User& currentUser,
                                                                          const DiscussionThread& thread,
                                                                          const User& targetUser,
                                                                          PrivilegeValueIntType newValue) const
{
    if (currentUser.id() == targetUser.id()) return AuthorizationStatus::NOT_ALLOWED;

    if (isThrottled(UserActionThrottling::EDIT_PRIVILEGES, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    const auto status = isAllowed(&currentUser, thread, DiscussionThreadPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }

    PrivilegeValueType oldValuePositive, oldValueNegative;
    grantedPrivilegeStore_.calculateDiscussionThreadPrivilege(&targetUser, thread, Context::getCurrentTime(),
                                                              oldValuePositive, oldValueNegative);
    const auto oldValue = computeOldValueForAssigningPrivileges(oldValuePositive, oldValueNegative);

    return allowPrivilegeAssignment(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}


AuthorizationStatus DefaultAuthorization::updateDiscussionThreadMessagePrivilege(const User& currentUser,
                                                                                 const DiscussionTag& tag,
                                                                                 DiscussionThreadMessagePrivilege /*privilege*/,
                                                                                 PrivilegeValueType oldValue,
                                                                                 PrivilegeValueIntType newValue) const
{
    if (isThrottled(UserActionThrottling::EDIT_PRIVILEGES, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    const auto status = isAllowed(&currentUser, tag, DiscussionThreadMessagePrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateDiscussionThreadPrivilege(const User& currentUser,
                                                                          const DiscussionTag& tag,
                                                                          DiscussionThreadPrivilege /*privilege*/,
                                                                          PrivilegeValueType oldValue,
                                                                          PrivilegeValueIntType newValue) const
{
    if (isThrottled(UserActionThrottling::EDIT_PRIVILEGES, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    const auto status = isAllowed(&currentUser, tag, DiscussionThreadPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateDiscussionTagPrivilege(const User& currentUser,
                                                                       const DiscussionTag& tag,
                                                                       DiscussionTagPrivilege /*privilege*/,
                                                                       PrivilegeValueType oldValue,
                                                                       PrivilegeValueIntType newValue) const
{
    if (isThrottled(UserActionThrottling::EDIT_PRIVILEGES, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    const auto status = isAllowed(&currentUser, tag, DiscussionTagPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::assignDiscussionTagPrivilege(const User& currentUser,
                                                                       const DiscussionTag& tag,
                                                                       const User& targetUser,
                                                                       PrivilegeValueIntType newValue) const
{
    if (currentUser.id() == targetUser.id()) return AuthorizationStatus::NOT_ALLOWED;

    if (isThrottled(UserActionThrottling::EDIT_PRIVILEGES, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    const auto status = isAllowed(&currentUser, tag, DiscussionTagPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }

    PrivilegeValueType oldValuePositive, oldValueNegative;
    grantedPrivilegeStore_.calculateDiscussionTagPrivilege(&targetUser, tag, Context::getCurrentTime(),
                                                           oldValuePositive, oldValueNegative);
    const auto oldValue = computeOldValueForAssigningPrivileges(oldValuePositive, oldValueNegative);

    return allowPrivilegeAssignment(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateDiscussionCategoryPrivilege(const User& currentUser,
                                                                            const DiscussionCategory& category,
                                                                            DiscussionCategoryPrivilege /*privilege*/,
                                                                            PrivilegeValueType oldValue,
                                                                            PrivilegeValueIntType newValue) const
{
    if (isThrottled(UserActionThrottling::EDIT_PRIVILEGES, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    const auto status = isAllowed(&currentUser, category, DiscussionCategoryPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateDiscussionThreadMessagePrivilege(const User& currentUser,
                                                                                 DiscussionThreadMessagePrivilege /*privilege*/,
                                                                                 PrivilegeValueType oldValue,
                                                                                 PrivilegeValueIntType newValue) const
{
    if (isThrottled(UserActionThrottling::EDIT_PRIVILEGES, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    const auto status = isAllowed(&currentUser, DiscussionThreadMessagePrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateDiscussionThreadPrivilege(const User& currentUser,
                                                                          DiscussionThreadPrivilege /*privilege*/,
                                                                          PrivilegeValueType oldValue,
                                                                          PrivilegeValueIntType newValue) const
{
    if (isThrottled(UserActionThrottling::EDIT_PRIVILEGES, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    const auto status = isAllowed(&currentUser, DiscussionThreadPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateDiscussionTagPrivilege(const User& currentUser,
                                                                       DiscussionTagPrivilege /*privilege*/,
                                                                       PrivilegeValueType oldValue,
                                                                       PrivilegeValueIntType newValue) const
{
    if (isThrottled(UserActionThrottling::EDIT_PRIVILEGES, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    const auto status = isAllowed(&currentUser, DiscussionTagPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateDiscussionCategoryPrivilege(const User& currentUser,
                                                                            DiscussionCategoryPrivilege /*privilege*/,
                                                                            PrivilegeValueType oldValue,
                                                                            PrivilegeValueIntType newValue) const
{
    if (isThrottled(UserActionThrottling::EDIT_PRIVILEGES, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    const auto status = isAllowed(&currentUser, DiscussionCategoryPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::assignDiscussionCategoryPrivilege(const User& currentUser,
                                                                            const DiscussionCategory& category,
                                                                            const User& targetUser,
                                                                            PrivilegeValueIntType newValue) const
{
    if (currentUser.id() == targetUser.id()) return AuthorizationStatus::NOT_ALLOWED;

    if (isThrottled(UserActionThrottling::EDIT_PRIVILEGES, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    const auto status = isAllowed(&currentUser, category, DiscussionCategoryPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }

    PrivilegeValueType oldValuePositive, oldValueNegative;
    grantedPrivilegeStore_.calculateDiscussionCategoryPrivilege(&targetUser, category,
                                                                Context::getCurrentTime(),
                                                                oldValuePositive, oldValueNegative);
    const auto oldValue = computeOldValueForAssigningPrivileges(oldValuePositive, oldValueNegative);

    return allowPrivilegeAssignment(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateForumWidePrivilege(const User& currentUser,
                                                                   ForumWidePrivilege /*privilege*/,
                                                                   PrivilegeValueType oldValue,
                                                                   PrivilegeValueIntType newValue) const
{
    if (isThrottled(UserActionThrottling::EDIT_PRIVILEGES, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    const auto status = isAllowed(&currentUser, ForumWidePrivilege::ADJUST_FORUM_WIDE_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateForumWideDefaultPrivilegeLevel(
        const User& currentUser, ForumWideDefaultPrivilegeDuration /*privilege*/,
        PrivilegeValueIntType /*newValue*/, PrivilegeDurationIntType /*newDuration*/) const
{
    if (isThrottled(UserActionThrottling::EDIT_PRIVILEGES, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    const auto status = isAllowed(&currentUser, ForumWidePrivilege::ADJUST_FORUM_WIDE_PRIVILEGE, with);
    return status;
}

AuthorizationStatus DefaultAuthorization::getForumWideRequiredPrivileges(const User& currentUser) const
{
    PrivilegeValueType with;
    const auto status = isAllowed(&currentUser, ForumWidePrivilege::VIEW_FORUM_WIDE_REQUIRED_PRIVILEGES, with);
    return status;
}

AuthorizationStatus DefaultAuthorization::getForumWideAssignedPrivileges(const User& currentUser) const
{
    PrivilegeValueType with;
    const auto status = isAllowed(&currentUser, ForumWidePrivilege::VIEW_FORUM_WIDE_ASSIGNED_PRIVILEGES, with);
    return status;
}

AuthorizationStatus DefaultAuthorization::getUserAssignedPrivileges(const User& currentUser, 
                                                                    const User& targetUser) const
{
    if (currentUser.id() == targetUser.id()) return AuthorizationStatus::OK;

    PrivilegeValueType with;
    const auto status = isAllowed(&currentUser, ForumWidePrivilege::VIEW_USER_ASSIGNED_PRIVILEGES, with);
    return status;
}

AuthorizationStatus DefaultAuthorization::getAllowForumWidePrivilegeChange(const Entities::User& currentUser) const
{
    PrivilegeValueType with;
    return isAllowed(&currentUser, ForumWidePrivilege::ADJUST_FORUM_WIDE_PRIVILEGE, with);
}

AuthorizationStatus DefaultAuthorization::assignForumWidePrivilege(const User& currentUser,
                                                                   const User& targetUser,
                                                                   PrivilegeValueIntType newValue) const
{
    if (currentUser.id() == targetUser.id()) return AuthorizationStatus::NOT_ALLOWED;

    if (isThrottled(UserActionThrottling::EDIT_PRIVILEGES, currentUser)) return AuthorizationStatus::THROTTLED;

    PrivilegeValueType with;
    const auto status = isAllowed(&currentUser, ForumWidePrivilege::ADJUST_FORUM_WIDE_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }

    PrivilegeValueType oldValuePositive, oldValueNegative;
    grantedPrivilegeStore_.calculateForumWidePrivilege(&targetUser, Context::getCurrentTime(),
                                                       oldValuePositive, oldValueNegative);
    const auto oldValue = computeOldValueForAssigningPrivileges(oldValuePositive, oldValueNegative);

    return allowPrivilegeAssignment(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}
