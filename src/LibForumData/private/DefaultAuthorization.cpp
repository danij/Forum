#include "DefaultAuthorization.h"

#include "ContextProviders.h"

using namespace Forum;
using namespace Forum::Authorization;
using namespace Forum::Entities;

DefaultAuthorization::DefaultAuthorization(GrantedPrivilegeStore& privilegeStore,
                                           ForumWidePrivilegeStore& forumWidePrivilegeStore)
    : grantedPrivilegeStore_(privilegeStore), forumWidePrivilegeStore_(forumWidePrivilegeStore)
{
}

AuthorizationStatus DefaultAuthorization::login(IdType userId) const
{
    PrivilegeValueType with;
    return isAllowed(userId, ForumWidePrivilege::LOGIN, with);
}

AuthorizationStatus DefaultAuthorization::getUsers(const User& currentUser) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_ALL_USERS, with);
}

AuthorizationStatus DefaultAuthorization::getUserById(const User& currentUser, const User& user) const
{
    if (currentUser.id() == user.id())
    {
        return AuthorizationStatus::OK;
    }
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_USER_INFO, with);
}

AuthorizationStatus DefaultAuthorization::getUserByName(const User& currentUser, const User& user) const
{
    if (currentUser.id() == user.id())
    {
        return AuthorizationStatus::OK;
    }
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_USER_INFO, with);
}

AuthorizationStatus DefaultAuthorization::addNewUser(const User& currentUser, StringView name) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), ForumWidePrivilege::ADD_USER, with);
}

AuthorizationStatus DefaultAuthorization::changeUserName(const User& currentUser, const User& user, StringView newName) const
{
    PrivilegeValueType with;
    if (currentUser.id() == user.id())
    {
        return isAllowed(currentUser.id(), ForumWidePrivilege::CHANGE_OWN_USER_NAME, with);
    }
    return isAllowed(currentUser.id(), ForumWidePrivilege::CHANGE_ANY_USER_NAME, with);
}

AuthorizationStatus DefaultAuthorization::changeUserInfo(const User& currentUser, const User& user, StringView newInfo) const
{
    PrivilegeValueType with;
    if (currentUser.id() == user.id())
    {
        return isAllowed(currentUser.id(), ForumWidePrivilege::CHANGE_OWN_USER_INFO, with);
    }
    return isAllowed(currentUser.id(), ForumWidePrivilege::CHANGE_ANY_USER_INFO, with);
}

AuthorizationStatus DefaultAuthorization::deleteUser(const User& currentUser, const User& user) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), ForumWidePrivilege::DELETE_ANY_USER, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreads(const User& currentUser) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_ALL_DISCUSSION_THREADS, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadById(const User& currentUser, const DiscussionThread& thread) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), thread, DiscussionThreadPrivilege::VIEW, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadsOfUser(const User& currentUser, const User& user) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_DISCUSSION_THREADS_OF_USER, with);
}

AuthorizationStatus DefaultAuthorization::getSubscribedDiscussionThreadsOfUser(const User& currentUser, const User& user) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadsWithTag(const User& currentUser, const DiscussionTag& tag) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), tag, DiscussionTagPrivilege::GET_DISCUSSION_THREADS, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadsOfCategory(const User& currentUser,
                                                                         const DiscussionCategory& category) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), category, DiscussionCategoryPrivilege::GET_DISCUSSION_THREADS, with);
}

AuthorizationStatus DefaultAuthorization::addNewDiscussionThread(const User& currentUser, StringView name) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), ForumWidePrivilege::ADD_DISCUSSION_THREAD, with);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionThreadName(const User& currentUser,
                                                                     const DiscussionThread& thread,
                                                                     StringView newName) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), thread, DiscussionThreadPrivilege::CHANGE_NAME, with);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionThreadPinDisplayOrder(const User& currentUser,
                                                                                const DiscussionThread& thread,
                                                                                uint16_t newValue) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), thread, DiscussionThreadPrivilege::CHANGE_PIN_DISPLAY_ORDER, with);
}

AuthorizationStatus DefaultAuthorization::deleteDiscussionThread(const User& currentUser, const DiscussionThread& thread) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), thread, DiscussionThreadPrivilege::DELETE, with);
}

AuthorizationStatus DefaultAuthorization::mergeDiscussionThreads(const User& currentUser, const DiscussionThread& from,
                                                                 const DiscussionThread& into) const
{
    return isAllowed(currentUser.id(), from, into, DiscussionThreadPrivilege::MERGE);
}

AuthorizationStatus DefaultAuthorization::subscribeToDiscussionThread(const User& currentUser,
                                                                      const DiscussionThread& thread) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), thread, DiscussionThreadPrivilege::SUBSCRIBE, with);
}

AuthorizationStatus DefaultAuthorization::unsubscribeFromDiscussionThread(const User& currentUser,
                                                                          const DiscussionThread& thread) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), thread, DiscussionThreadPrivilege::UNSUBSCRIBE, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadMessageById(const User& currentUser,
                                                                         const DiscussionThreadMessage& message) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), message, DiscussionThreadMessagePrivilege::VIEW, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadMessagesOfUserByCreated(const User& currentUser,
                                                                                     const User& user) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_DISCUSSION_THREAD_MESSAGES_OF_USER, with);
}

AuthorizationStatus DefaultAuthorization::getMessageComments(const User& currentUser) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_ALL_MESSAGE_COMMENTS, with);
}

AuthorizationStatus DefaultAuthorization::getMessageCommentsOfDiscussionThreadMessage(const User& currentUser, const DiscussionThreadMessage& message) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), message, DiscussionThreadMessagePrivilege::GET_MESSAGE_COMMENTS, with);
}

AuthorizationStatus DefaultAuthorization::getMessageCommentsOfUser(const User& currentUser, const User& user) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_MESSAGE_COMMENTS_OF_USER, with);
}

AuthorizationStatus DefaultAuthorization::addNewDiscussionMessageInThread(const User& currentUser,
                                                                          const DiscussionThread& thread,
                                                                          StringView content) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), thread, DiscussionThreadPrivilege::ADD_MESSAGE, with);
}

AuthorizationStatus DefaultAuthorization::deleteDiscussionMessage(const User& currentUser,
                                                                  const DiscussionThreadMessage& message) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), message, DiscussionThreadMessagePrivilege::DELETE, with);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionThreadMessageContent(const User& currentUser,
                                                                               const DiscussionThreadMessage& message,
                                                                               StringView newContent,
                                                                               StringView changeReason) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), message, DiscussionThreadMessagePrivilege::CHANGE_CONTENT, with);
}

AuthorizationStatus DefaultAuthorization::moveDiscussionThreadMessage(const User& currentUser,
                                                                      const DiscussionThreadMessage& message,
                                                                      const DiscussionThread& intoThread) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), message, DiscussionThreadMessagePrivilege::MOVE, with);
}

AuthorizationStatus DefaultAuthorization::upVoteDiscussionThreadMessage(const User& currentUser,
                                                                        const DiscussionThreadMessage& message) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), message, DiscussionThreadMessagePrivilege::UP_VOTE, with);
}

AuthorizationStatus DefaultAuthorization::downVoteDiscussionThreadMessage(const User& currentUser,
                                                                          const DiscussionThreadMessage& message) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), message, DiscussionThreadMessagePrivilege::DOWN_VOTE, with);
}

AuthorizationStatus DefaultAuthorization::resetVoteDiscussionThreadMessage(const User& currentUser,
                                                                           const DiscussionThreadMessage& message) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), message, DiscussionThreadMessagePrivilege::RESET_VOTE, with);
}

AuthorizationStatus DefaultAuthorization::addCommentToDiscussionThreadMessage(const User& currentUser,
                                                                              const DiscussionThreadMessage& message,
                                                                              StringView content) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), message, DiscussionThreadMessagePrivilege::ADD_COMMENT, with);
}

AuthorizationStatus DefaultAuthorization::setMessageCommentToSolved(const User& currentUser,
                                                                    const MessageComment& comment) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), comment.parentMessage(),
                     DiscussionThreadMessagePrivilege::SET_COMMENT_TO_SOLVED, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionTagById(const User& currentUser,
                                                               const DiscussionTag& tag) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), tag, DiscussionTagPrivilege::VIEW, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionTags(const User& currentUser) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_ALL_DISCUSSION_TAGS, with);
}

AuthorizationStatus DefaultAuthorization::addNewDiscussionTag(const User& currentUser, StringView name) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), ForumWidePrivilege::ADD_DISCUSSION_TAG, with);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionTagName(const User& currentUser, const DiscussionTag& tag,
                                                                  StringView newName) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), tag, DiscussionTagPrivilege::CHANGE_NAME, with);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionTagUiBlob(const User& currentUser, const DiscussionTag& tag,
                                                                    StringView blob) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), tag, DiscussionTagPrivilege::CHANGE_UIBLOB, with);
}

AuthorizationStatus DefaultAuthorization::deleteDiscussionTag(const User& currentUser, const DiscussionTag& tag) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), tag, DiscussionTagPrivilege::DELETE, with);
}

AuthorizationStatus DefaultAuthorization::addDiscussionTagToThread(const User& currentUser, const DiscussionTag& tag,
                                                                   const DiscussionThread& thread) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), thread, DiscussionThreadPrivilege::ADD_TAG, with);
}

AuthorizationStatus DefaultAuthorization::removeDiscussionTagFromThread(const User& currentUser, const DiscussionTag& tag,
                                                                        const DiscussionThread& thread) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), thread, DiscussionThreadPrivilege::REMOVE_TAG, with);
}

AuthorizationStatus DefaultAuthorization::mergeDiscussionTags(const User& currentUser, const DiscussionTag& from,
                                                              const DiscussionTag& into) const
{
    return isAllowed(currentUser.id(), from, into, DiscussionTagPrivilege::MERGE);
}

AuthorizationStatus DefaultAuthorization::getDiscussionCategoryById(const User& currentUser,
                                                                    const DiscussionCategory& category) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), category, DiscussionCategoryPrivilege::VIEW, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionCategories(const User& currentUser) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_ALL_DISCUSSION_CATEGORIES, with);
}

AuthorizationStatus DefaultAuthorization::getDiscussionCategoriesFromRoot(const User& currentUser) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_DISCUSSION_CATEGORIES_FROM_ROOT, with);
}

AuthorizationStatus DefaultAuthorization::addNewDiscussionCategory(const User& currentUser, StringView name,
                                                                   const DiscussionCategory* parent) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), ForumWidePrivilege::ADD_DISCUSSION_CATEGORY, with);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionCategoryName(const User& currentUser,
                                                                       const DiscussionCategory& category,
                                                                       StringView newName) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), category, DiscussionCategoryPrivilege::CHANGE_NAME, with);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionCategoryDescription(const User& currentUser,
                                                                              const DiscussionCategory& category,
                                                                              StringView newDescription) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), category, DiscussionCategoryPrivilege::CHANGE_DESCRIPTION, with);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionCategoryParent(const User& currentUser,
                                                                         const DiscussionCategory& category,
                                                                         const DiscussionCategory* newParent) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), category, DiscussionCategoryPrivilege::CHANGE_PARENT, with);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionCategoryDisplayOrder(const User& currentUser,
                                                                               const DiscussionCategory& category,
                                                                               int_fast16_t newDisplayOrder) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), category, DiscussionCategoryPrivilege::CHANGE_DISPLAYORDER, with);
}

AuthorizationStatus DefaultAuthorization::deleteDiscussionCategory(const User& currentUser,
                                                                   const DiscussionCategory& category) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), category, DiscussionCategoryPrivilege::DELETE, with);
}

AuthorizationStatus DefaultAuthorization::addDiscussionTagToCategory(const User& currentUser, const DiscussionTag& tag,
                                                                     const DiscussionCategory& category) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), category, DiscussionCategoryPrivilege::ADD_TAG, with);
}

AuthorizationStatus DefaultAuthorization::removeDiscussionTagFromCategory(const User& currentUser,
                                                                          const DiscussionTag& tag,
                                                                          const DiscussionCategory& category) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), category, DiscussionCategoryPrivilege::REMOVE_TAG, with);
}

AuthorizationStatus DefaultAuthorization::getEntitiesCount(const User& currentUser) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_ENTITIES_COUNT, with);
}

AuthorizationStatus DefaultAuthorization::getVersion(const User& currentUser) const
{
    PrivilegeValueType with;
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_VERSION, with);
}

AuthorizationStatus DefaultAuthorization::isAllowed(IdTypeRef userId, const DiscussionThreadMessage& message,
                                                    DiscussionThreadMessagePrivilege privilege, PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(userId, message, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(IdTypeRef userId, const DiscussionThread& thread,
                                                    DiscussionThreadMessagePrivilege privilege, PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(userId, thread, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(IdTypeRef userId, const DiscussionTag& tag,
                                                    DiscussionThreadMessagePrivilege privilege, PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(userId, tag, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}


AuthorizationStatus DefaultAuthorization::isAllowed(IdTypeRef userId, const DiscussionThread& thread,
                                                    DiscussionThreadPrivilege privilege, PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(userId, thread, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(IdTypeRef userId, const DiscussionTag& tag,
                                                    DiscussionThreadPrivilege privilege, PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(userId, tag, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(IdTypeRef userId, const DiscussionThread& from,
                                                    const DiscussionThread& into, DiscussionThreadPrivilege privilege) const
{
    return static_cast<bool>(grantedPrivilegeStore_.isAllowed(userId, from, privilege, Context::getCurrentTime()))
            && static_cast<bool>(grantedPrivilegeStore_.isAllowed(userId, into, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(IdTypeRef userId, const DiscussionTag& tag,
                                                    DiscussionTagPrivilege privilege, PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(userId, tag, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(IdTypeRef userId, const DiscussionTag& from,
                                                    const DiscussionTag& into, DiscussionTagPrivilege privilege) const
{
    return static_cast<bool>(grantedPrivilegeStore_.isAllowed(userId, from, privilege, Context::getCurrentTime()))
            && static_cast<bool>(grantedPrivilegeStore_.isAllowed(userId, into, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(IdTypeRef userId, const DiscussionCategory& category,
                                                    DiscussionCategoryPrivilege privilege, PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(userId, category, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(IdTypeRef userId, DiscussionThreadMessagePrivilege privilege,
                                                    PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(userId, forumWidePrivilegeStore_, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(IdTypeRef userId, DiscussionThreadPrivilege privilege,
                                                    PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(userId, forumWidePrivilegeStore_, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(IdTypeRef userId, DiscussionTagPrivilege privilege,
                                                    PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(userId, forumWidePrivilegeStore_, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(IdTypeRef userId, DiscussionCategoryPrivilege privilege,
                                                    PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(userId, forumWidePrivilegeStore_, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}
AuthorizationStatus DefaultAuthorization::isAllowed(IdTypeRef userId, ForumWidePrivilege privilege,
                                                    PrivilegeValueType& with) const
{
    return (with = grantedPrivilegeStore_.isAllowed(userId, forumWidePrivilegeStore_, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

static bool allowPrivilegeUpdate(PrivilegeValueType oldValue, PrivilegeValueIntType newValue,
                                 PrivilegeValueType currentPermissions)
{
    PrivilegeValueIntType currentPermissionsValue = currentPermissions ? *currentPermissions : 0;
    return ((! oldValue) || (std::abs(*oldValue) <= currentPermissionsValue))
        && (std::abs(newValue) <= currentPermissionsValue);
}

static bool allowPrivilegeAssignment(PrivilegeValueType oldValue, PrivilegeValueIntType newValue,
    PrivilegeValueType currentPermissions)
{
    PrivilegeValueIntType currentPermissionsValue = currentPermissions ? *currentPermissions : 0;
    return (( ! oldValue) || (std::abs(*oldValue) < currentPermissionsValue))
        && (std::abs(newValue) < currentPermissionsValue);
}

AuthorizationStatus DefaultAuthorization::updateDiscussionThreadMessagePrivilege(const User& currentUser,
                                                                                 const DiscussionThreadMessage& message,
                                                                                 DiscussionThreadMessagePrivilege privilege,
                                                                                 PrivilegeValueType oldValue,
                                                                                 PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), message, DiscussionThreadMessagePrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::assignDiscussionThreadMessagePrivilege(const User& currentUser,
                                                                                 const DiscussionThreadMessage& message,
                                                                                 const User& targetUser,
                                                                                 DiscussionThreadMessagePrivilege privilege,
                                                                                 PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), message, DiscussionThreadMessagePrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }

    PrivilegeValueType oldValue;
    isAllowed(targetUser.id(), message, privilege, oldValue);

    return allowPrivilegeAssignment(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateDiscussionThreadMessagePrivilege(const User& currentUser,
                                                                                 const DiscussionThread& thread,
                                                                                 DiscussionThreadMessagePrivilege privilege,
                                                                                 PrivilegeValueType oldValue,
                                                                                 PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), thread, DiscussionThreadMessagePrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateDiscussionThreadPrivilege(const User& currentUser,
                                                                          const DiscussionThread& thread,
                                                                          DiscussionThreadPrivilege privilege,
                                                                          PrivilegeValueType oldValue,
                                                                          PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), thread, DiscussionThreadPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}


AuthorizationStatus DefaultAuthorization::updateDiscussionThreadMessageDefaultPrivilegeDuration(
        const User& currentUser, const DiscussionThread& thread, DiscussionThreadMessageDefaultPrivilegeDuration privilege,
        PrivilegeDefaultDurationIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), thread, DiscussionThreadPrivilege::ADJUST_PRIVILEGE, with);
    return status;
}

AuthorizationStatus DefaultAuthorization::assignDiscussionThreadMessagePrivilege(const User& currentUser,
                                                                                 const DiscussionThread& thread,
                                                                                 const User& targetUser,
                                                                                 DiscussionThreadMessagePrivilege privilege,
                                                                                 PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), thread, DiscussionThreadPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }

    PrivilegeValueType oldValue;
    isAllowed(targetUser.id(), thread, privilege, oldValue);

    return allowPrivilegeAssignment(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::assignDiscussionThreadPrivilege(const User& currentUser,
                                                                          const DiscussionThread& thread,
                                                                          const User& targetUser,
                                                                          DiscussionThreadPrivilege privilege,
                                                                          PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), thread, DiscussionThreadPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }

    PrivilegeValueType oldValue;
    isAllowed(targetUser.id(), thread, privilege, oldValue);

    return allowPrivilegeAssignment(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}


AuthorizationStatus DefaultAuthorization::updateDiscussionThreadMessagePrivilege(const User& currentUser,
                                                                                 const DiscussionTag& tag,
                                                                                 DiscussionThreadMessagePrivilege privilege,
                                                                                 PrivilegeValueType oldValue,
                                                                                 PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), tag, DiscussionThreadMessagePrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateDiscussionThreadPrivilege(const User& currentUser,
                                                                          const DiscussionTag& tag,
                                                                          DiscussionThreadPrivilege privilege,
                                                                          PrivilegeValueType oldValue,
                                                                          PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), tag, DiscussionThreadPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateDiscussionTagPrivilege(const User& currentUser,
                                                                       const DiscussionTag& tag,
                                                                       DiscussionTagPrivilege privilege,
                                                                       PrivilegeValueType oldValue,
                                                                       PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), tag, DiscussionTagPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateDiscussionThreadMessageDefaultPrivilegeDuration(
        const User& currentUser, const DiscussionTag& tag, DiscussionThreadMessageDefaultPrivilegeDuration privilege,
        PrivilegeDefaultDurationIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), tag, DiscussionTagPrivilege::ADJUST_PRIVILEGE, with);
    return status;
}

AuthorizationStatus DefaultAuthorization::assignDiscussionThreadMessagePrivilege(const User& currentUser,
                                                                                 const DiscussionTag& tag,
                                                                                 const User& targetUser,
                                                                                 DiscussionThreadMessagePrivilege privilege,
                                                                                 PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), tag, DiscussionTagPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }

    PrivilegeValueType oldValue;
    isAllowed(targetUser.id(), tag, privilege, oldValue);

    return allowPrivilegeAssignment(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::assignDiscussionThreadPrivilege(const User& currentUser,
                                                                          const DiscussionTag& tag,
                                                                          const User& targetUser,
                                                                          DiscussionThreadPrivilege privilege,
                                                                          PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), tag, DiscussionTagPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }

    PrivilegeValueType oldValue;
    isAllowed(targetUser.id(), tag, privilege, oldValue);

    return allowPrivilegeAssignment(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::assignDiscussionTagPrivilege(const User& currentUser,
                                                                       const DiscussionTag& tag,
                                                                       const User& targetUser,
                                                                       DiscussionTagPrivilege privilege,
                                                                       PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), tag, DiscussionTagPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }

    PrivilegeValueType oldValue;
    isAllowed(targetUser.id(), tag, privilege, oldValue);

    return allowPrivilegeAssignment(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateDiscussionCategoryPrivilege(const User& currentUser,
                                                                            const DiscussionCategory& category,
                                                                            DiscussionCategoryPrivilege privilege,
                                                                            PrivilegeValueType oldValue,
                                                                            PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), category, DiscussionCategoryPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateDiscussionThreadMessagePrivilege(const User& currentUser,
                                                                                 DiscussionThreadMessagePrivilege privilege,
                                                                                 PrivilegeValueType oldValue,
                                                                                 PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), DiscussionThreadMessagePrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateDiscussionThreadPrivilege(const User& currentUser,
                                                                          DiscussionThreadPrivilege privilege,
                                                                          PrivilegeValueType oldValue,
                                                                          PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), DiscussionThreadPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateDiscussionTagPrivilege(const User& currentUser,
                                                                       DiscussionTagPrivilege privilege,
                                                                       PrivilegeValueType oldValue,
                                                                       PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), DiscussionTagPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateDiscussionCategoryPrivilege(const User& currentUser,
                                                                            DiscussionCategoryPrivilege privilege,
                                                                            PrivilegeValueType oldValue,
                                                                            PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), DiscussionCategoryPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::assignDiscussionCategoryPrivilege(const User& currentUser,
                                                                            const DiscussionCategory& category,
                                                                            const User& targetUser,
                                                                            DiscussionCategoryPrivilege privilege,
                                                                            PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), category, DiscussionCategoryPrivilege::ADJUST_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }

    PrivilegeValueType oldValue;
    isAllowed(targetUser.id(), category, privilege, oldValue);

    return allowPrivilegeAssignment(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateForumWidePrivilege(const User& currentUser,
                                                                   ForumWidePrivilege privilege,
                                                                   PrivilegeValueType oldValue,
                                                                   PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), ForumWidePrivilege::ADJUST_FORUM_WIDE_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }
    return allowPrivilegeUpdate(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::updateDiscussionThreadMessageDefaultPrivilegeDuration(
        const User& currentUser, DiscussionThreadMessageDefaultPrivilegeDuration privilege,
        PrivilegeDefaultDurationIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), ForumWidePrivilege::ADJUST_FORUM_WIDE_PRIVILEGE, with);
    return status;
}

AuthorizationStatus DefaultAuthorization::updateForumWideDefaultPrivilegeDuration(
        const User& currentUser, ForumWideDefaultPrivilegeDuration privilege,
        PrivilegeDefaultDurationIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), ForumWidePrivilege::ADJUST_FORUM_WIDE_PRIVILEGE, with);
    return status;
}

AuthorizationStatus DefaultAuthorization::assignDiscussionThreadMessagePrivilege(const User& currentUser,
                                                                                 const User& targetUser,
                                                                                 DiscussionThreadMessagePrivilege privilege,
                                                                                 PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), ForumWidePrivilege::ADJUST_FORUM_WIDE_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }

    PrivilegeValueType oldValue;
    isAllowed(targetUser.id(), privilege, oldValue);

    return allowPrivilegeAssignment(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::assignDiscussionThreadPrivilege(const User& currentUser,
                                                                          const User& targetUser,
                                                                          DiscussionThreadPrivilege privilege,
                                                                          PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), ForumWidePrivilege::ADJUST_FORUM_WIDE_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }

    PrivilegeValueType oldValue;
    isAllowed(targetUser.id(), privilege, oldValue);

    return allowPrivilegeAssignment(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::assignDiscussionTagPrivilege(const User& currentUser,
                                                                       const User& targetUser,
                                                                       DiscussionTagPrivilege privilege,
                                                                       PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), ForumWidePrivilege::ADJUST_FORUM_WIDE_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }

    PrivilegeValueType oldValue;
    isAllowed(targetUser.id(), privilege, oldValue);

    return allowPrivilegeAssignment(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::assignDiscussionCategoryPrivilege(const User& currentUser,
                                                                            const User& targetUser,
                                                                            DiscussionCategoryPrivilege privilege,
                                                                            PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), ForumWidePrivilege::ADJUST_FORUM_WIDE_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }

    PrivilegeValueType oldValue;
    isAllowed(targetUser.id(), privilege, oldValue);

    return allowPrivilegeAssignment(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::assignForumWidePrivilege(const User& currentUser,
                                                                   const User& targetUser,
                                                                   ForumWidePrivilege privilege,
                                                                   PrivilegeValueIntType newValue) const
{
    PrivilegeValueType with;
    auto status = isAllowed(currentUser.id(), ForumWidePrivilege::ADJUST_FORUM_WIDE_PRIVILEGE, with);
    if (status != AuthorizationStatus::OK)
    {
        return status;
    }

    PrivilegeValueType oldValue;
    isAllowed(targetUser.id(), privilege, oldValue);

    return allowPrivilegeAssignment(oldValue, newValue, with) ? AuthorizationStatus::OK : AuthorizationStatus::NOT_ALLOWED;
}
