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
    return isAllowed(userId, ForumWidePrivilege::LOGIN);
}

AuthorizationStatus DefaultAuthorization::getUsers(const User& currentUser) const
{
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_ALL_USERS);
}

AuthorizationStatus DefaultAuthorization::getUserById(const User& currentUser, const User& user) const
{
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_USER_INFO);
}

AuthorizationStatus DefaultAuthorization::getUserByName(const User& currentUser, const User& user) const
{
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_USER_INFO);
}

AuthorizationStatus DefaultAuthorization::addNewUser(const User& currentUser, StringView name) const
{
    return isAllowed(currentUser.id(), ForumWidePrivilege::ADD_USER);
}

AuthorizationStatus DefaultAuthorization::changeUserName(const User& currentUser, const User& user, StringView newName) const
{
    if (currentUser.id() == user.id())
    {
        return AuthorizationStatus::OK;
    }
    return isAllowed(currentUser.id(), ForumWidePrivilege::CHANGE_ANY_USER_NAME);
}

AuthorizationStatus DefaultAuthorization::changeUserInfo(const User& currentUser, const User& user, StringView newInfo) const
{
    if (currentUser.id() == user.id())
    {
        return AuthorizationStatus::OK;
    }
    return isAllowed(currentUser.id(), ForumWidePrivilege::CHANGE_ANY_USER_NAME);
}

AuthorizationStatus DefaultAuthorization::deleteUser(const User& currentUser, const User& user) const
{
    return isAllowed(currentUser.id(), ForumWidePrivilege::DELETE_ANY_USER);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreads(const User& currentUser) const
{
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_ALL_DISCUSSION_THREADS);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadById(const User& currentUser, const DiscussionThread& thread) const
{
    return isAllowed(currentUser, thread, DiscussionThreadPrivilege::VIEW);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadsOfUser(const User& currentUser, const User& user) const
{
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_DISCUSSION_THREADS_OF_USER);
}

AuthorizationStatus DefaultAuthorization::getSubscribedDiscussionThreadsOfUser(const User& currentUser, const User& user) const
{
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_SUBSCRIBED_DISCUSSION_THREADS_OF_USER);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadsWithTag(const User& currentUser, const DiscussionTag& tag) const
{
    return isAllowed(currentUser, tag, DiscussionTagPrivilege::GET_DISCUSSION_THREADS);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadsOfCategory(const User& currentUser,
                                                                         const DiscussionCategory& category) const
{
    return isAllowed(currentUser, category, DiscussionCategoryPrivilege::GET_DISCUSSION_THREADS);
}

AuthorizationStatus DefaultAuthorization::addNewDiscussionThread(const User& currentUser, StringView name) const
{
    return isAllowed(currentUser.id(), ForumWidePrivilege::ADD_DISCUSSION_THREAD);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionThreadName(const User& currentUser,
                                                                     const DiscussionThread& thread,
                                                                     StringView newName) const
{
    return isAllowed(currentUser, thread, DiscussionThreadPrivilege::CHANGE_NAME);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionThreadPinDisplayOrder(const User& currentUser,
                                                                                const DiscussionThread& thread,
                                                                                uint16_t newValue) const
{
    return isAllowed(currentUser, thread, DiscussionThreadPrivilege::CHANGE_PIN_DISPLAY_ORDER);
}

AuthorizationStatus DefaultAuthorization::deleteDiscussionThread(const User& currentUser, const DiscussionThread& thread) const
{
    return isAllowed(currentUser, thread, DiscussionThreadPrivilege::DELETE);
}

AuthorizationStatus DefaultAuthorization::mergeDiscussionThreads(const User& currentUser, const DiscussionThread& from,
                                                                 const DiscussionThread& into) const
{
    return isAllowed(currentUser, from, into, DiscussionThreadPrivilege::MERGE);
}

AuthorizationStatus DefaultAuthorization::subscribeToDiscussionThread(const User& currentUser,
                                                                      const DiscussionThread& thread) const
{
    return isAllowed(currentUser, thread, DiscussionThreadPrivilege::SUBSCRIBE);
}

AuthorizationStatus DefaultAuthorization::unsubscribeFromDiscussionThread(const User& currentUser,
                                                                          const DiscussionThread& thread) const
{
    return isAllowed(currentUser, thread, DiscussionThreadPrivilege::UNSUBSCRIBE);
}

AuthorizationStatus DefaultAuthorization::getDiscussionThreadMessagesOfUserByCreated(const User& currentUser,
                                                                                     const User& user) const
{
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_DISCUSSION_THREAD_MESSAGES_OF_USER);
}

AuthorizationStatus DefaultAuthorization::getMessageComments(const User& currentUser) const
{
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_ALL_MESSAGE_COMMENTS);
}

AuthorizationStatus DefaultAuthorization::getMessageCommentsOfDiscussionThreadMessage(const User& currentUser, const DiscussionThreadMessage& message) const
{
    return isAllowed(currentUser, message, DiscussionThreadMessagePrivilege::GET_MESSAGE_COMMENTS);
}

AuthorizationStatus DefaultAuthorization::getMessageCommentsOfUser(const User& currentUser, const User& user) const
{
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_MESSAGE_COMMENTS_OF_USER);
}

AuthorizationStatus DefaultAuthorization::addNewDiscussionMessageInThread(const User& currentUser,
                                                                          const DiscussionThread& thread,
                                                                          StringView content) const
{
    return isAllowed(currentUser, thread, DiscussionThreadPrivilege::ADD_MESSAGE);
}

AuthorizationStatus DefaultAuthorization::deleteDiscussionMessage(const User& currentUser,
                                                                  const DiscussionThreadMessage& message) const
{
    return isAllowed(currentUser, message, DiscussionThreadMessagePrivilege::DELETE);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionThreadMessageContent(const User& currentUser,
                                                                               const DiscussionThreadMessage& message,
                                                                               StringView newContent,
                                                                               StringView changeReason) const
{
    return isAllowed(currentUser, message, DiscussionThreadMessagePrivilege::CHANGE_CONTENT);
}

AuthorizationStatus DefaultAuthorization::moveDiscussionThreadMessage(const User& currentUser,
                                                                      const DiscussionThreadMessage& message,
                                                                      const DiscussionThread& intoThread) const
{
    return isAllowed(currentUser, message, DiscussionThreadMessagePrivilege::MOVE);
}

AuthorizationStatus DefaultAuthorization::upVoteDiscussionThreadMessage(const User& currentUser,
                                                                        const DiscussionThreadMessage& message) const
{
    return isAllowed(currentUser, message, DiscussionThreadMessagePrivilege::UP_VOTE);
}

AuthorizationStatus DefaultAuthorization::downVoteDiscussionThreadMessage(const User& currentUser,
                                                                          const DiscussionThreadMessage& message) const
{
    return isAllowed(currentUser, message, DiscussionThreadMessagePrivilege::DOWN_VOTE);
}

AuthorizationStatus DefaultAuthorization::resetVoteDiscussionThreadMessage(const User& currentUser,
                                                                           const DiscussionThreadMessage& message) const
{
    return isAllowed(currentUser, message, DiscussionThreadMessagePrivilege::RESET_VOTE);
}

AuthorizationStatus DefaultAuthorization::addCommentToDiscussionThreadMessage(const User& currentUser,
                                                                              const DiscussionThreadMessage& message,
                                                                              StringView content) const
{
    return isAllowed(currentUser, message, DiscussionThreadMessagePrivilege::ADD_COMMENT);
}

AuthorizationStatus DefaultAuthorization::setMessageCommentToSolved(const User& currentUser,
                                                                    const MessageComment& comment) const
{
    return isAllowed(currentUser, comment.parentMessage(), DiscussionThreadMessagePrivilege::SET_COMMENT_TO_SOLVED);
}

AuthorizationStatus DefaultAuthorization::getDiscussionTags(const User& currentUser) const
{
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_ALL_DISCUSSION_TAGS);
}

AuthorizationStatus DefaultAuthorization::addNewDiscussionTag(const User& currentUser, StringView name) const
{
    return isAllowed(currentUser.id(), ForumWidePrivilege::ADD_DISCUSSION_TAG);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionTagName(const User& currentUser, const DiscussionTag& tag,
                                                                  StringView newName) const
{
    return isAllowed(currentUser, tag, DiscussionTagPrivilege::CHANGE_NAME);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionTagUiBlob(const User& currentUser, const DiscussionTag& tag,
                                                                    StringView blob) const
{
    return isAllowed(currentUser, tag, DiscussionTagPrivilege::CHANGE_UIBLOB);
}

AuthorizationStatus DefaultAuthorization::deleteDiscussionTag(const User& currentUser, const DiscussionTag& tag) const
{
    return isAllowed(currentUser, tag, DiscussionTagPrivilege::DELETE);
}

AuthorizationStatus DefaultAuthorization::addDiscussionTagToThread(const User& currentUser, const DiscussionTag& tag,
                                                                   const DiscussionThread& thread) const
{
    return isAllowed(currentUser, thread, DiscussionThreadPrivilege::ADD_TAG);
}

AuthorizationStatus DefaultAuthorization::removeDiscussionTagFromThread(const User& currentUser, const DiscussionTag& tag,
                                                                        const DiscussionThread& thread) const
{
    return isAllowed(currentUser, thread, DiscussionThreadPrivilege::REMOVE_TAG);
}

AuthorizationStatus DefaultAuthorization::mergeDiscussionTags(const User& currentUser, const DiscussionTag& from,
                                                              const DiscussionTag& into) const
{
    return isAllowed(currentUser, from, into, DiscussionTagPrivilege::MERGE);
}

AuthorizationStatus DefaultAuthorization::getDiscussionCategoryById(const User& currentUser,
                                                                    const DiscussionCategory& category) const
{
    return isAllowed(currentUser, category, DiscussionCategoryPrivilege::VIEW);
}

AuthorizationStatus DefaultAuthorization::getDiscussionCategories(const User& currentUser) const
{
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_ALL_DISCUSSION_CATEGORIES);
}

AuthorizationStatus DefaultAuthorization::getDiscussionCategoriesFromRoot(const User& currentUser) const
{
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_DISCUSSION_CATEGORIES_FROM_ROOT);
}

AuthorizationStatus DefaultAuthorization::addNewDiscussionCategory(const User& currentUser, StringView name,
                                                                   const DiscussionCategory* parent) const
{
    return isAllowed(currentUser.id(), ForumWidePrivilege::ADD_DISCUSSION_CATEGORY);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionCategoryName(const User& currentUser,
                                                                       const DiscussionCategory& category,
                                                                       StringView newName) const
{
    return isAllowed(currentUser, category, DiscussionCategoryPrivilege::CHANGE_NAME);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionCategoryDescription(const User& currentUser,
                                                                              const DiscussionCategory& category,
                                                                              StringView newDescription) const
{
    return isAllowed(currentUser, category, DiscussionCategoryPrivilege::CHANGE_DESCRIPTION);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionCategoryParent(const User& currentUser,
                                                                         const DiscussionCategory& category,
                                                                         const DiscussionCategory* newParent) const
{
    return isAllowed(currentUser, category, DiscussionCategoryPrivilege::CHANGE_PARENT);
}

AuthorizationStatus DefaultAuthorization::changeDiscussionCategoryDisplayOrder(const User& currentUser,
                                                                               const DiscussionCategory& category,
                                                                               int_fast16_t newDisplayOrder) const
{
    return isAllowed(currentUser, category, DiscussionCategoryPrivilege::CHANGE_DISPLAYORDER);
}

AuthorizationStatus DefaultAuthorization::deleteDiscussionCategory(const User& currentUser,
                                                                   const DiscussionCategory& category) const
{
    return isAllowed(currentUser, category, DiscussionCategoryPrivilege::DELETE);
}

AuthorizationStatus DefaultAuthorization::addDiscussionTagToCategory(const User& currentUser, const DiscussionTag& tag,
                                                                     const DiscussionCategory& category) const
{
    return isAllowed(currentUser, category, DiscussionCategoryPrivilege::ADD_TAG);
}

AuthorizationStatus DefaultAuthorization::removeDiscussionTagFromCategory(const User& currentUser,
                                                                          const DiscussionTag& tag,
                                                                          const DiscussionCategory& category) const
{
    return isAllowed(currentUser, category, DiscussionCategoryPrivilege::REMOVE_TAG);
}

AuthorizationStatus DefaultAuthorization::getEntitiesCount(const User& currentUser) const
{
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_ENTITIES_COUNT);
}

AuthorizationStatus DefaultAuthorization::getVersion(const User& currentUser) const
{
    return isAllowed(currentUser.id(), ForumWidePrivilege::GET_VERSION);
}

AuthorizationStatus DefaultAuthorization::isAllowed(const User& user, const DiscussionThreadMessage& message,
                                                    DiscussionThreadMessagePrivilege privilege) const
{
    return grantedPrivilegeStore_.isAllowed(user, message, privilege, Context::getCurrentTime())
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(const User& user, const DiscussionThread& thread,
                                                    DiscussionThreadPrivilege privilege) const
{
    return grantedPrivilegeStore_.isAllowed(user, thread, privilege, Context::getCurrentTime())
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;

}

AuthorizationStatus DefaultAuthorization::isAllowed(const User& user, const DiscussionThread& from,
                                                    const DiscussionThread& into, DiscussionThreadPrivilege privilege) const
{
    return static_cast<bool>(grantedPrivilegeStore_.isAllowed(user, from, privilege, Context::getCurrentTime()))
            && static_cast<bool>(grantedPrivilegeStore_.isAllowed(user, into, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(const User& user, const DiscussionTag& tag,
                                                    DiscussionTagPrivilege privilege) const
{
    return grantedPrivilegeStore_.isAllowed(user, tag, privilege, Context::getCurrentTime())
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;

}

AuthorizationStatus DefaultAuthorization::isAllowed(const User& user, const DiscussionTag& from,
                                                    const DiscussionTag& into, DiscussionTagPrivilege privilege) const
{
    return static_cast<bool>(grantedPrivilegeStore_.isAllowed(user, from, privilege, Context::getCurrentTime()))
            && static_cast<bool>(grantedPrivilegeStore_.isAllowed(user, into, privilege, Context::getCurrentTime()))
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(const User& user, const DiscussionCategory& category,
                                                    DiscussionCategoryPrivilege privilege) const
{
    return grantedPrivilegeStore_.isAllowed(user, category, privilege, Context::getCurrentTime())
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}

AuthorizationStatus DefaultAuthorization::isAllowed(IdType userId, ForumWidePrivilege privilege) const
{
    return grantedPrivilegeStore_.isAllowed(userId, forumWidePrivilegeStore_, privilege, Context::getCurrentTime())
            ? AuthorizationStatus::OK
            : AuthorizationStatus::NOT_ALLOWED;
}
