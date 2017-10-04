#pragma once

#include "Authorization.h"

namespace Forum
{
    namespace Authorization
    {
        class AllowAllAuthorization : public IUserAuthorization,
                                      public IDiscussionThreadAuthorization,
                                      public IDiscussionThreadMessageAuthorization,
                                      public IDiscussionTagAuthorization,
                                      public IDiscussionCategoryAuthorization,
                                      public IForumWideAuthorization,
                                      public IStatisticsAuthorization,
                                      public IMetricsAuthorization
        {
            AuthorizationStatus login(Entities::IdType userId) const override { return {}; }

            AuthorizationStatus getUsers(const Entities::User& currentUser) const override { return {}; }

            AuthorizationStatus getUserById(const Entities::User& currentUser,
                                            const Entities::User& user) const override { return {}; }
            AuthorizationStatus getUserByName(const Entities::User& currentUser,
                                              const Entities::User& user) const override { return {}; }
            AuthorizationStatus getUserVoteHistory(const Entities::User& currentUser,
                                                   const Entities::User& user) const override { return {}; }

            AuthorizationStatus addNewUser(const Entities::User& currentUser, StringView name) const override { return {}; }
            AuthorizationStatus changeUserName(const Entities::User& currentUser,
                                               const Entities::User& user, StringView newName) const override { return {}; }
            AuthorizationStatus changeUserInfo(const Entities::User& currentUser,
                                               const Entities::User& user, StringView newInfo) const override { return {}; }
            AuthorizationStatus changeUserTitle(const Entities::User& currentUser,
                                                const Entities::User& user, StringView newTitle) const override { return {}; }
            AuthorizationStatus changeUserSignature(const Entities::User& currentUser,
                                                    const Entities::User& user, StringView newSignature) const override { return {}; }
            AuthorizationStatus changeUserLogo(const Entities::User& currentUser,
                                               const Entities::User& user, StringView newLogo) const override { return {}; }
            AuthorizationStatus deleteUserLogo(const Entities::User& currentUser,
                                               const Entities::User& user) const override { return {}; }
            AuthorizationStatus deleteUser(const Entities::User& currentUser,
                                           const Entities::User& user) const override { return {}; }

            AuthorizationStatus getDiscussionThreads(const Entities::User& currentUser) const override { return {}; }
            AuthorizationStatus getDiscussionThreadById(const Entities::User& currentUser,
                                                        const Entities::DiscussionThread& thread) const override { return {}; }

            AuthorizationStatus getDiscussionThreadsOfUser(const Entities::User& currentUser,
                                                           const Entities::User& user) const override { return {}; }
            AuthorizationStatus getSubscribedDiscussionThreadsOfUser(const Entities::User& currentUser,
                                                                     const Entities::User& user) const override { return {}; }

            AuthorizationStatus getDiscussionThreadsWithTag(const Entities::User& currentUser,
                                                            const Entities::DiscussionTag& tag) const override { return {}; }

            AuthorizationStatus getDiscussionThreadsOfCategory(const Entities::User& currentUser,
                                                               const Entities::DiscussionCategory& category) const override { return {}; }

            AuthorizationStatus addNewDiscussionThread(const Entities::User& currentUser,
                                                       StringView name) const override { return {}; }
            AuthorizationStatus changeDiscussionThreadName(const Entities::User& currentUser,
                                                           const Entities::DiscussionThread& thread,
                                                           StringView newName) const override { return {}; }
            AuthorizationStatus changeDiscussionThreadPinDisplayOrder(const Entities::User& currentUser,
                                                                      const Entities::DiscussionThread& thread,
                                                                      uint16_t newValue) const override { return {}; }
            AuthorizationStatus deleteDiscussionThread(const Entities::User& currentUser,
                                                       const Entities::DiscussionThread& thread) const override { return {}; }
            AuthorizationStatus mergeDiscussionThreads(const Entities::User& currentUser,
                                                       const Entities::DiscussionThread& from,
                                                       const Entities::DiscussionThread& into) const override { return {}; }
            AuthorizationStatus subscribeToDiscussionThread(const Entities::User& currentUser,
                                                            const Entities::DiscussionThread& thread) const override { return {}; }
            AuthorizationStatus unsubscribeFromDiscussionThread(const Entities::User& currentUser,
                                                                const Entities::DiscussionThread& thread) const override { return {}; }

            AuthorizationStatus getDiscussionThreadMessageById(const Entities::User& currentUser,
                                                               const Entities::DiscussionThreadMessage& message) const override { return {}; }
            AuthorizationStatus getDiscussionThreadMessagesOfUserByCreated(const Entities::User& currentUser,
                                                                           const Entities::User& user) const override { return {}; }
            AuthorizationStatus getDiscussionThreadMessageRank(const Entities::User& currentUser,
                                                               const Entities::DiscussionThreadMessage& message) const override { return {}; }

            AuthorizationStatus getMessageComments(const Entities::User& currentUser) const override { return {}; }
            AuthorizationStatus getMessageCommentsOfDiscussionThreadMessage(const Entities::User& currentUser,
                                                                            const Entities::DiscussionThreadMessage& message) const override { return {}; }
            AuthorizationStatus getMessageCommentsOfUser(const Entities::User& currentUser,
                                                         const Entities::User& user) const override { return {}; }

            AuthorizationStatus addNewDiscussionMessageInThread(const Entities::User& currentUser,
                                                                const Entities::DiscussionThread& thread,
                                                                StringView content) const override { return {}; }
            AuthorizationStatus deleteDiscussionMessage(const Entities::User& currentUser,
                                                        const Entities::DiscussionThreadMessage& message) const override { return {}; }
            AuthorizationStatus changeDiscussionThreadMessageContent(const Entities::User& currentUser,
                                                                     const Entities::DiscussionThreadMessage& message,
                                                                     StringView newContent,
                                                                     StringView changeReason) const override { return {}; }
            AuthorizationStatus moveDiscussionThreadMessage(const Entities::User& currentUser,
                                                            const Entities::DiscussionThreadMessage& message,
                                                            const Entities::DiscussionThread& intoThread) const override { return {}; }
            AuthorizationStatus upVoteDiscussionThreadMessage(const Entities::User& currentUser,
                                                              const Entities::DiscussionThreadMessage& message) const override { return {}; }
            AuthorizationStatus downVoteDiscussionThreadMessage(const Entities::User& currentUser,
                                                                const Entities::DiscussionThreadMessage& message) const override { return {}; }
            AuthorizationStatus resetVoteDiscussionThreadMessage(const Entities::User& currentUser,
                                                                 const Entities::DiscussionThreadMessage& message) const override { return {}; }

            AuthorizationStatus addCommentToDiscussionThreadMessage(const Entities::User& currentUser,
                                                                    const Entities::DiscussionThreadMessage& message,
                                                                    StringView content) const override { return {}; }
            AuthorizationStatus setMessageCommentToSolved(const Entities::User& currentUser,
                                                          const Entities::MessageComment& comment) const override { return {}; }

            AuthorizationStatus getDiscussionTagById(const Entities::User& currentUser,
                                                     const Entities::DiscussionTag& tag) const override { return {}; }

            AuthorizationStatus getDiscussionTags(const Entities::User& currentUser) const override { return {}; }

            AuthorizationStatus addNewDiscussionTag(const Entities::User& currentUser,
                                                    StringView name) const override { return {}; }
            AuthorizationStatus changeDiscussionTagName(const Entities::User& currentUser,
                                                        const Entities::DiscussionTag& tag,
                                                        StringView newName) const override { return {}; }
            AuthorizationStatus changeDiscussionTagUiBlob(const Entities::User& currentUser,
                                                          const Entities::DiscussionTag& tag,
                                                          StringView blob) const override { return {}; }
            AuthorizationStatus deleteDiscussionTag(const Entities::User& currentUser,
                                                    const Entities::DiscussionTag& tag) const override { return {}; }
            AuthorizationStatus addDiscussionTagToThread(const Entities::User& currentUser,
                                                         const Entities::DiscussionTag& tag,
                                                         const Entities::DiscussionThread& thread) const override { return {}; }
            AuthorizationStatus removeDiscussionTagFromThread(const Entities::User& currentUser,
                                                              const Entities::DiscussionTag& tag,
                                                              const Entities::DiscussionThread& thread) const override { return {}; }
            AuthorizationStatus mergeDiscussionTags(const Entities::User& currentUser,
                                                    const Entities::DiscussionTag& from,
                                                    const Entities::DiscussionTag& into) const override { return {}; }

            AuthorizationStatus getDiscussionCategoryById(const Entities::User& currentUser,
                                                          const Entities::DiscussionCategory& category) const override { return {}; }
            AuthorizationStatus getDiscussionCategories(const Entities::User& currentUser) const override { return {}; }
            AuthorizationStatus getDiscussionCategoriesFromRoot(const Entities::User& currentUser) const override { return {}; }

            AuthorizationStatus addNewDiscussionCategory(const Entities::User& currentUser,
                                                         StringView name,
                                                         const Entities::DiscussionCategory* parent) const override { return {}; }
            AuthorizationStatus changeDiscussionCategoryName(const Entities::User& currentUser,
                                                             const Entities::DiscussionCategory& category,
                                                             StringView newName) const override { return {}; }
            AuthorizationStatus changeDiscussionCategoryDescription(const Entities::User& currentUser,
                                                                    const Entities::DiscussionCategory& category,
                                                                    StringView newDescription) const override { return {}; }
            AuthorizationStatus changeDiscussionCategoryParent(const Entities::User& currentUser,
                                                               const Entities::DiscussionCategory& category,
                                                               const Entities::DiscussionCategory* newParent) const override { return {}; }
            AuthorizationStatus changeDiscussionCategoryDisplayOrder(const Entities::User& currentUser,
                                                                     const Entities::DiscussionCategory& category,
                                                                     int_fast16_t newDisplayOrder) const override { return {}; }
            AuthorizationStatus deleteDiscussionCategory(const Entities::User& currentUser,
                                                         const Entities::DiscussionCategory& category) const override { return {}; }
            AuthorizationStatus addDiscussionTagToCategory(const Entities::User& currentUser,
                                                           const Entities::DiscussionTag& tag,
                                                           const Entities::DiscussionCategory& category) const override { return {}; }
            AuthorizationStatus removeDiscussionTagFromCategory(const Entities::User& currentUser,
                                                                const Entities::DiscussionTag& tag,
                                                                const Entities::DiscussionCategory& category) const override { return {}; }

            AuthorizationStatus getEntitiesCount(const Entities::User& currentUser) const override { return {}; }

            AuthorizationStatus getVersion(const Entities::User& currentUser) const override { return {}; }

            AuthorizationStatus updateDiscussionThreadMessagePrivilege(const Entities::User& currentUser,
                                                                       const Entities::DiscussionThread& thread,
                                                                       DiscussionThreadMessagePrivilege privilege,
                                                                       PrivilegeValueType oldValue,
                                                                       PrivilegeValueIntType newValue) const override { return {}; }
            AuthorizationStatus updateDiscussionThreadPrivilege(const Entities::User& currentUser,
                                                                const Entities::DiscussionThread& thread,
                                                                DiscussionThreadPrivilege privilege,
                                                                PrivilegeValueType oldValue,
                                                                PrivilegeValueIntType newValue) const override { return {}; }
            AuthorizationStatus updateDiscussionThreadMessageDefaultPrivilegeDuration(
                    const Entities::User& currentUser, const Entities::DiscussionThread& thread,
                    DiscussionThreadMessageDefaultPrivilegeDuration privilege,
                    PrivilegeDefaultDurationIntType newValue) const override { return {}; }
            AuthorizationStatus assignDiscussionThreadPrivilege(const Entities::User& currentUser,
                                                                const Entities::DiscussionThread& thread,
                                                                const Entities::User& targetUser,
                                                                PrivilegeValueIntType newValue) const override { return {}; }
            AuthorizationStatus updateDiscussionThreadMessagePrivilege(const Entities::User& currentUser,
                                                                       const Entities::DiscussionThreadMessage& message,
                                                                       DiscussionThreadMessagePrivilege privilege,
                                                                       PrivilegeValueType oldValue,
                                                                       PrivilegeValueIntType newValue) const override { return {}; }
            AuthorizationStatus updateDiscussionThreadMessagePrivilege(const Entities::User& currentUser,
                                                                       const Entities::DiscussionTag& tag,
                                                                       DiscussionThreadMessagePrivilege privilege,
                                                                       PrivilegeValueType oldValue,
                                                                       PrivilegeValueIntType newValue) const override { return {}; }
            AuthorizationStatus updateDiscussionThreadPrivilege(const Entities::User& currentUser,
                                                                const Entities::DiscussionTag& tag,
                                                                DiscussionThreadPrivilege privilege,
                                                                PrivilegeValueType oldValue,
                                                                PrivilegeValueIntType newValue) const override { return {}; }
            AuthorizationStatus updateDiscussionTagPrivilege(const Entities::User& currentUser,
                                                             const Entities::DiscussionTag& tag,
                                                             DiscussionTagPrivilege privilege,
                                                             PrivilegeValueType oldValue,
                                                             PrivilegeValueIntType newValue) const override { return {}; }
            AuthorizationStatus updateDiscussionThreadMessageDefaultPrivilegeDuration(
                    const Entities::User& currentUser, const Entities::DiscussionTag& tag,
                    DiscussionThreadMessageDefaultPrivilegeDuration privilege,
                    PrivilegeDefaultDurationIntType newValue) const override { return {}; }
            AuthorizationStatus assignDiscussionTagPrivilege(const Entities::User& currentUser,
                                                             const Entities::DiscussionTag& tag,
                                                             const Entities::User& targetUser,
                                                             PrivilegeValueIntType newValue) const override { return {}; }
            AuthorizationStatus updateDiscussionCategoryPrivilege(const Entities::User& currentUser,
                                                                  const Entities::DiscussionCategory& category,
                                                                  DiscussionCategoryPrivilege privilege,
                                                                  PrivilegeValueType oldValue,
                                                                  PrivilegeValueIntType newValue) const override { return {}; }
            AuthorizationStatus assignDiscussionCategoryPrivilege(const Entities::User& currentUser,
                                                                  const Entities::DiscussionCategory& category,
                                                                  const Entities::User& targetUser,
                                                                  PrivilegeValueIntType newValue) const override { return {}; }
            AuthorizationStatus updateDiscussionThreadMessagePrivilege(const Entities::User& currentUser,
                                                                       DiscussionThreadMessagePrivilege privilege,
                                                                       PrivilegeValueType oldValue,
                                                                       PrivilegeValueIntType newValue) const override { return {}; }
            AuthorizationStatus assignDiscussionThreadMessagePrivilege(const Entities::User& currentUser,
                                                                       const Entities::DiscussionThreadMessage& message,
                                                                       const Entities::User& targetUser,
                                                                       PrivilegeValueIntType newValue) const override { return {}; }
            AuthorizationStatus updateDiscussionThreadPrivilege(const Entities::User& currentUser,
                                                                DiscussionThreadPrivilege privilege,
                                                                PrivilegeValueType oldValue,
                                                                PrivilegeValueIntType newValue) const override { return {}; }
            AuthorizationStatus updateDiscussionTagPrivilege(const Entities::User& currentUser,
                                                             DiscussionTagPrivilege privilege,
                                                             PrivilegeValueType oldValue,
                                                             PrivilegeValueIntType newValue) const override { return {}; }
            AuthorizationStatus updateDiscussionCategoryPrivilege(const Entities::User& currentUser,
                                                                  DiscussionCategoryPrivilege privilege,
                                                                  PrivilegeValueType oldValue,
                                                                  PrivilegeValueIntType newValue) const override { return {}; }
            AuthorizationStatus updateForumWidePrivilege(const Entities::User& currentUser,
                                                         ForumWidePrivilege privilege, PrivilegeValueType oldValue,
                                                         PrivilegeValueIntType newValue) const override { return {}; }
            AuthorizationStatus updateDiscussionThreadMessageDefaultPrivilegeDuration(
                    const Entities::User& currentUser, DiscussionThreadMessageDefaultPrivilegeDuration privilege,
                    PrivilegeDefaultDurationIntType newValue) const override { return {}; }
            AuthorizationStatus updateForumWideDefaultPrivilegeDuration(
                    const Entities::User& currentUser, ForumWideDefaultPrivilegeDuration privilege,
                    PrivilegeDefaultDurationIntType newValue) const override { return {}; }
            AuthorizationStatus assignForumWidePrivilege(const Entities::User& currentUser,
                                                         const Entities::User& targetUser,
                                                         PrivilegeValueIntType newValue) const override { return{}; }
        };
    }
}
