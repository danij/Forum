#pragma once

#include "Authorization.h"
#include "AuthorizationGrantedPrivilegeStore.h"

#include <boost/noncopyable.hpp>

namespace Forum
{
    namespace Authorization
    {
        class DefaultAuthorization final : private boost::noncopyable,
                                           public IUserAuthorization,
                                           public IDiscussionThreadAuthorization,
                                           public IDiscussionThreadMessageAuthorization,
                                           public IDiscussionTagAuthorization,
                                           public IDiscussionCategoryAuthorization,
                                           public IForumWideAuthorization,
                                           public IStatisticsAuthorization,
                                           public IMetricsAuthorization
        {
        public:
            explicit DefaultAuthorization(GrantedPrivilegeStore& grantedPrivilegeStore,
                                          ForumWidePrivilegeStore& forumWidePrivilegeStore);

            AuthorizationStatus login(Entities::IdType userId) const override;

            AuthorizationStatus getUsers(const Entities::User& currentUser) const override;

            AuthorizationStatus getUserById(const Entities::User& currentUser,
                                            const Entities::User& user) const override;
            AuthorizationStatus getUserByName(const Entities::User& currentUser,
                                              const Entities::User& user) const override;

            AuthorizationStatus addNewUser(const Entities::User& currentUser, StringView name) const override;
            AuthorizationStatus changeUserName(const Entities::User& currentUser,
                                               const Entities::User& user, StringView newName) const override;
            AuthorizationStatus changeUserInfo(const Entities::User& currentUser,
                                               const Entities::User& user, StringView newInfo) const override;
            AuthorizationStatus deleteUser(const Entities::User& currentUser,
                                           const Entities::User& user) const override;

            AuthorizationStatus getDiscussionThreads(const Entities::User& currentUser) const override;
            AuthorizationStatus getDiscussionThreadById(const Entities::User& currentUser,
                                                        const Entities::DiscussionThread& thread) const override;

            AuthorizationStatus getDiscussionThreadsOfUser(const Entities::User& currentUser,
                                                           const Entities::User& user) const override;
            AuthorizationStatus getSubscribedDiscussionThreadsOfUser(const Entities::User& currentUser,
                                                                     const Entities::User& user) const override;

            AuthorizationStatus getDiscussionThreadsWithTag(const Entities::User& currentUser,
                                                            const Entities::DiscussionTag& tag) const override;

            AuthorizationStatus getDiscussionThreadsOfCategory(const Entities::User& currentUser,
                                                               const Entities::DiscussionCategory& category) const override;

            AuthorizationStatus addNewDiscussionThread(const Entities::User& currentUser,
                                                       StringView name) const override;
            AuthorizationStatus changeDiscussionThreadName(const Entities::User& currentUser,
                                                           const Entities::DiscussionThread& thread,
                                                           StringView newName) const override;
            AuthorizationStatus changeDiscussionThreadPinDisplayOrder(const Entities::User& currentUser,
                                                                      const Entities::DiscussionThread& thread,
                                                                      uint16_t newValue) const override;
            AuthorizationStatus deleteDiscussionThread(const Entities::User& currentUser,
                                                       const Entities::DiscussionThread& thread) const override;
            AuthorizationStatus mergeDiscussionThreads(const Entities::User& currentUser,
                                                       const Entities::DiscussionThread& from,
                                                       const Entities::DiscussionThread& into) const override;
            AuthorizationStatus subscribeToDiscussionThread(const Entities::User& currentUser,
                                                            const Entities::DiscussionThread& thread) const override;
            AuthorizationStatus unsubscribeFromDiscussionThread(const Entities::User& currentUser,
                                                                const Entities::DiscussionThread& thread) const override;

            AuthorizationStatus getDiscussionThreadMessagesOfUserByCreated(const Entities::User& currentUser,
                                                                           const Entities::User& user) const override;

            AuthorizationStatus getMessageComments(const Entities::User& currentUser) const override;
            AuthorizationStatus getMessageCommentsOfDiscussionThreadMessage(const Entities::User& currentUser,
                                                                            const Entities::DiscussionThreadMessage& message) const override;
            AuthorizationStatus getMessageCommentsOfUser(const Entities::User& currentUser,
                                                         const Entities::User& user) const override;

            AuthorizationStatus getDiscussionThreadMessageById(const Entities::User& currentUser,
                                                               const Entities::DiscussionThreadMessage& message) const override;
            AuthorizationStatus addNewDiscussionMessageInThread(const Entities::User& currentUser,
                                                                const Entities::DiscussionThread& thread,
                                                                StringView content) const override;
            AuthorizationStatus deleteDiscussionMessage(const Entities::User& currentUser,
                                                        const Entities::DiscussionThreadMessage& message) const override;
            AuthorizationStatus changeDiscussionThreadMessageContent(const Entities::User& currentUser,
                                                                     const Entities::DiscussionThreadMessage& message,
                                                                     StringView newContent,
                                                                     StringView changeReason) const override;
            AuthorizationStatus moveDiscussionThreadMessage(const Entities::User& currentUser,
                                                            const Entities::DiscussionThreadMessage& message,
                                                            const Entities::DiscussionThread& intoThread) const override;
            AuthorizationStatus upVoteDiscussionThreadMessage(const Entities::User& currentUser,
                                                              const Entities::DiscussionThreadMessage& message) const override;
            AuthorizationStatus downVoteDiscussionThreadMessage(const Entities::User& currentUser,
                                                                const Entities::DiscussionThreadMessage& message) const override;
            AuthorizationStatus resetVoteDiscussionThreadMessage(const Entities::User& currentUser,
                                                                 const Entities::DiscussionThreadMessage& message) const override;

            AuthorizationStatus addCommentToDiscussionThreadMessage(const Entities::User& currentUser,
                                                                    const Entities::DiscussionThreadMessage& message,
                                                                    StringView content) const override;
            AuthorizationStatus setMessageCommentToSolved(const Entities::User& currentUser,
                                                          const Entities::MessageComment& comment) const override;

            AuthorizationStatus getDiscussionTagById(const Entities::User& currentUser,
                                                     const Entities::DiscussionTag& tag) const override;
            AuthorizationStatus getDiscussionTags(const Entities::User& currentUser) const override;

            AuthorizationStatus addNewDiscussionTag(const Entities::User& currentUser, StringView name) const override;
            AuthorizationStatus changeDiscussionTagName(const Entities::User& currentUser,
                                                        const Entities::DiscussionTag& tag,
                                                        StringView newName) const override;
            AuthorizationStatus changeDiscussionTagUiBlob(const Entities::User& currentUser,
                                                          const Entities::DiscussionTag& tag,
                                                          StringView blob) const override;
            AuthorizationStatus deleteDiscussionTag(const Entities::User& currentUser,
                                                    const Entities::DiscussionTag& tag) const override;
            AuthorizationStatus addDiscussionTagToThread(const Entities::User& currentUser,
                                                         const Entities::DiscussionTag& tag,
                                                         const Entities::DiscussionThread& thread) const override;
            AuthorizationStatus removeDiscussionTagFromThread(const Entities::User& currentUser,
                                                              const Entities::DiscussionTag& tag,
                                                              const Entities::DiscussionThread& thread) const override;
            AuthorizationStatus mergeDiscussionTags(const Entities::User& currentUser,
                                                    const Entities::DiscussionTag& from,
                                                    const Entities::DiscussionTag& into) const override;

            AuthorizationStatus getDiscussionCategoryById(const Entities::User& currentUser,
                                                          const Entities::DiscussionCategory& category) const override;
            AuthorizationStatus getDiscussionCategories(const Entities::User& currentUser) const override;
            AuthorizationStatus getDiscussionCategoriesFromRoot(const Entities::User& currentUser) const override;

            AuthorizationStatus addNewDiscussionCategory(const Entities::User& currentUser,
                                                         StringView name,
                                                         const Entities::DiscussionCategory* parent) const override;
            AuthorizationStatus changeDiscussionCategoryName(const Entities::User& currentUser,
                                                             const Entities::DiscussionCategory& category,
                                                             StringView newName) const override;
            AuthorizationStatus changeDiscussionCategoryDescription(const Entities::User& currentUser,
                                                                    const Entities::DiscussionCategory& category,
                                                                    StringView newDescription) const override;
            AuthorizationStatus changeDiscussionCategoryParent(const Entities::User& currentUser,
                                                               const Entities::DiscussionCategory& category,
                                                               const Entities::DiscussionCategory* newParent) const override;
            AuthorizationStatus changeDiscussionCategoryDisplayOrder(const Entities::User& currentUser,
                                                                     const Entities::DiscussionCategory& category,
                                                                     int_fast16_t newDisplayOrder) const override;
            AuthorizationStatus deleteDiscussionCategory(const Entities::User& currentUser,
                                                         const Entities::DiscussionCategory& category) const override;
            AuthorizationStatus addDiscussionTagToCategory(const Entities::User& currentUser,
                                                           const Entities::DiscussionTag& tag,
                                                           const Entities::DiscussionCategory& category) const override;
            AuthorizationStatus removeDiscussionTagFromCategory(const Entities::User& currentUser,
                                                                const Entities::DiscussionTag& tag,
                                                                const Entities::DiscussionCategory& category) const override;

            AuthorizationStatus getEntitiesCount(const Entities::User& currentUser) const override;

            AuthorizationStatus getVersion(const Entities::User& currentUser) const override;

            AuthorizationStatus updateDiscussionThreadMessagePrivilege(const Entities::User& currentUser,
                                                                       const Entities::DiscussionThreadMessage& message,
                                                                       DiscussionThreadMessagePrivilege privilege,
                                                                       PrivilegeValueType oldValue,
                                                                       PrivilegeValueIntType newValue) const override;
            AuthorizationStatus updateDiscussionThreadMessagePrivilege(const Entities::User& currentUser,
                                                                       const Entities::DiscussionThread& thread,
                                                                       DiscussionThreadMessagePrivilege privilege,
                                                                       PrivilegeValueType oldValue,
                                                                       PrivilegeValueIntType newValue) const override;
            AuthorizationStatus updateDiscussionThreadPrivilege(const Entities::User& currentUser,
                                                                const Entities::DiscussionThread& thread,
                                                                DiscussionThreadPrivilege privilege,
                                                                PrivilegeValueType oldValue,
                                                                PrivilegeValueIntType newValue) const override;
            AuthorizationStatus updateDiscussionThreadMessagePrivilege(const Entities::User& currentUser,
                                                                       const Entities::DiscussionTag& tag,
                                                                       DiscussionThreadMessagePrivilege privilege,
                                                                       PrivilegeValueType oldValue,
                                                                       PrivilegeValueIntType newValue) const override;
            AuthorizationStatus updateDiscussionThreadPrivilege(const Entities::User& currentUser,
                                                                const Entities::DiscussionTag& tag,
                                                                DiscussionThreadPrivilege privilege,
                                                                PrivilegeValueType oldValue,
                                                                PrivilegeValueIntType newValue) const override;
            AuthorizationStatus updateDiscussionTagPrivilege(const Entities::User& currentUser,
                                                             const Entities::DiscussionTag& tag,
                                                             DiscussionTagPrivilege privilege,
                                                             PrivilegeValueType oldValue,
                                                             PrivilegeValueIntType newValue) const override;
            AuthorizationStatus updateDiscussionCategoryPrivilege(const Entities::User& currentUser,
                                                                  const Entities::DiscussionCategory& category,
                                                                  DiscussionCategoryPrivilege privilege,
                                                                  PrivilegeValueType oldValue,
                                                                  PrivilegeValueIntType newValue) const override;
            AuthorizationStatus updateDiscussionThreadMessagePrivilege(const Entities::User& currentUser,
                                                                       DiscussionThreadMessagePrivilege privilege,
                                                                       PrivilegeValueType oldValue,
                                                                       PrivilegeValueIntType newValue) const override;
            AuthorizationStatus updateDiscussionThreadPrivilege(const Entities::User& currentUser,
                                                                DiscussionThreadPrivilege privilege,
                                                                PrivilegeValueType oldValue,
                                                                PrivilegeValueIntType newValue) const override;
            AuthorizationStatus updateDiscussionTagPrivilege(const Entities::User& currentUser,
                                                             DiscussionTagPrivilege privilege,
                                                             PrivilegeValueType oldValue,
                                                             PrivilegeValueIntType newValue) const override;
            AuthorizationStatus updateDiscussionCategoryPrivilege(const Entities::User& currentUser,
                                                                  DiscussionCategoryPrivilege privilege,
                                                                  PrivilegeValueType oldValue,
                                                                  PrivilegeValueIntType newValue) const override;
            AuthorizationStatus updateForumWidePrivilege(const Entities::User& currentUser,
                                                         ForumWidePrivilege privilege, PrivilegeValueType oldValue,
                                                         PrivilegeValueIntType newValue) const override;
        private:
            AuthorizationStatus isAllowed(Entities::IdTypeRef userId, const Entities::DiscussionThreadMessage& message,
                                          DiscussionThreadMessagePrivilege privilege, PrivilegeValueType& with) const;
            AuthorizationStatus isAllowed(Entities::IdTypeRef userId, const Entities::DiscussionThread& thread,
                                          DiscussionThreadMessagePrivilege privilege, PrivilegeValueType& with) const;
            AuthorizationStatus isAllowed(Entities::IdTypeRef userId, const Entities::DiscussionTag& tag,
                                          DiscussionThreadMessagePrivilege privilege, PrivilegeValueType& with) const;

            AuthorizationStatus isAllowed(Entities::IdTypeRef userId, const Entities::DiscussionThread& thread,
                                          DiscussionThreadPrivilege privilege, PrivilegeValueType& with) const;
            AuthorizationStatus isAllowed(Entities::IdTypeRef userId, const Entities::DiscussionTag& tag,
                                          DiscussionThreadPrivilege privilege, PrivilegeValueType& with) const;

            AuthorizationStatus isAllowed(Entities::IdTypeRef userId, const Entities::DiscussionThread& from,
                                          const Entities::DiscussionThread& into, DiscussionThreadPrivilege privilege) const;

            AuthorizationStatus isAllowed(Entities::IdTypeRef userId, const Entities::DiscussionTag& tag,
                                          DiscussionTagPrivilege privilege, PrivilegeValueType& with) const;

            AuthorizationStatus isAllowed(Entities::IdTypeRef userId, const Entities::DiscussionTag& from,
                                          const Entities::DiscussionTag& into, DiscussionTagPrivilege privilege) const;

            AuthorizationStatus isAllowed(Entities::IdTypeRef userId, const Entities::DiscussionCategory& category,
                                          DiscussionCategoryPrivilege privilege, PrivilegeValueType& with) const;

            AuthorizationStatus isAllowed(Entities::IdTypeRef userId, DiscussionThreadMessagePrivilege privilege,
                                          PrivilegeValueType& with) const;
            AuthorizationStatus isAllowed(Entities::IdTypeRef userId, DiscussionThreadPrivilege privilege,
                                          PrivilegeValueType& with) const;
            AuthorizationStatus isAllowed(Entities::IdTypeRef userId, DiscussionTagPrivilege privilege,
                                          PrivilegeValueType& with) const;
            AuthorizationStatus isAllowed(Entities::IdTypeRef userId, DiscussionCategoryPrivilege privilege,
                                          PrivilegeValueType& with) const;
            AuthorizationStatus isAllowed(Entities::IdTypeRef userId, ForumWidePrivilege privilege,
                                          PrivilegeValueType& with) const;

            GrantedPrivilegeStore& grantedPrivilegeStore_;
            ForumWidePrivilegeStore& forumWidePrivilegeStore_;
        };
    }
}
