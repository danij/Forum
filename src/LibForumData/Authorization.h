#pragma once

#include "TypeHelpers.h"
#include "Entities.h"
#include "AuthorizationPrivileges.h"

#include <memory>

namespace Forum
{
    namespace Authorization
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
            virtual AuthorizationStatus deleteUser(const Entities::User& currentUser,
                                                   const Entities::User& user) const = 0;
        };
        typedef std::shared_ptr<IUserAuthorization> UserAuthorizationRef;


        class IDiscussionThreadAuthorization
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IDiscussionThreadAuthorization)

            virtual AuthorizationStatus getDiscussionThreads(const Entities::User& currentUser) const = 0;
            virtual AuthorizationStatus getDiscussionThreadById(const Entities::User& currentUser,
                                                                const Entities::DiscussionThread& thread) const = 0;

            virtual AuthorizationStatus getDiscussionThreadsOfUser(const Entities::User& currentUser,
                                                                   const Entities::User& user) const = 0;
            virtual AuthorizationStatus getSubscribedDiscussionThreadsOfUser(const Entities::User& currentUser,
                                                                             const Entities::User& user) const = 0;

            virtual AuthorizationStatus getDiscussionThreadsWithTag(const Entities::User& currentUser,
                                                                    const Entities::DiscussionTag& tag) const = 0;

            virtual AuthorizationStatus getDiscussionThreadsOfCategory(const Entities::User& currentUser,
                                                                       const Entities::DiscussionCategory& category) const = 0;

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
                                                                               PrivilegeValueIntType oldValue,
                                                                               PrivilegeValueIntType newValue) const = 0;
            virtual AuthorizationStatus updateDiscussionThreadPrivilege(const Entities::User& currentUser,
                                                                        const Entities::DiscussionThread& thread,
                                                                        DiscussionThreadPrivilege privilege,
                                                                        PrivilegeValueIntType oldValue,
                                                                        PrivilegeValueIntType newValue) const = 0;
        };
        typedef std::shared_ptr<IDiscussionThreadAuthorization> DiscussionThreadAuthorizationRef;


        class IDiscussionThreadMessageAuthorization
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IDiscussionThreadMessageAuthorization)

            virtual AuthorizationStatus getDiscussionThreadMessageById(const Entities::User& currentUser,
                                                                       const Entities::DiscussionThreadMessage& message) const = 0;
            virtual AuthorizationStatus getDiscussionThreadMessagesOfUserByCreated(const Entities::User& currentUser,
                                                                                   const Entities::User& user) const = 0;

            virtual AuthorizationStatus getMessageComments(const Entities::User& currentUser) const = 0;
            virtual AuthorizationStatus getMessageCommentsOfDiscussionThreadMessage(const Entities::User& currentUser,
                                                                                    const Entities::DiscussionThreadMessage& message) const = 0;
            virtual AuthorizationStatus getMessageCommentsOfUser(const Entities::User& currentUser,
                                                                 const Entities::User& user) const = 0;

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
                                                                               PrivilegeValueIntType oldValue,
                                                                               PrivilegeValueIntType newValue) const = 0;
        };
        typedef std::shared_ptr<IDiscussionThreadMessageAuthorization> DiscussionThreadMessageAuthorizationRef;


        class IDiscussionTagAuthorization
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IDiscussionTagAuthorization)

            virtual AuthorizationStatus getDiscussionTagById(const Entities::User& currentUser,
                                                             const Entities::DiscussionTag& tag) const = 0;
            virtual AuthorizationStatus getDiscussionTags(const Entities::User& currentUser) const = 0;

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
                                                                               PrivilegeValueIntType oldValue,
                                                                               PrivilegeValueIntType newValue) const = 0;
            virtual AuthorizationStatus updateDiscussionThreadPrivilege(const Entities::User& currentUser,
                                                                        const Entities::DiscussionTag& tag,
                                                                        DiscussionThreadPrivilege privilege,
                                                                        PrivilegeValueIntType oldValue,
                                                                        PrivilegeValueIntType newValue) const = 0;
            virtual AuthorizationStatus updateDiscussionTagPrivilege(const Entities::User& currentUser,
                                                                     const Entities::DiscussionTag& tag,
                                                                     DiscussionTagPrivilege privilege,
                                                                     PrivilegeValueIntType oldValue,
                                                                     PrivilegeValueIntType newValue) const = 0;
        };
        typedef std::shared_ptr<IDiscussionTagAuthorization> DiscussionTagAuthorizationRef;


        class IDiscussionCategoryAuthorization
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IDiscussionCategoryAuthorization)

            virtual AuthorizationStatus getDiscussionCategoryById(const Entities::User& currentUser,
                                                                  const Entities::DiscussionCategory& category) const = 0;
            virtual AuthorizationStatus getDiscussionCategories(const Entities::User& currentUser) const = 0;
            virtual AuthorizationStatus getDiscussionCategoriesFromRoot(const Entities::User& currentUser) const = 0;

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
                                                                          PrivilegeValueIntType oldValue,
                                                                          PrivilegeValueIntType newValue) const = 0;
        };
        typedef std::shared_ptr<IDiscussionCategoryAuthorization> DiscussionCategoryAuthorizationRef;

        class IForumWideAuthorization
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IForumWideAuthorization)

            virtual AuthorizationStatus updateDiscussionThreadMessagePrivilege(const Entities::User& currentUser,
                                                                               DiscussionThreadMessagePrivilege privilege,
                                                                               PrivilegeValueIntType oldValue,
                                                                               PrivilegeValueIntType newValue) const = 0;
            virtual AuthorizationStatus updateDiscussionThreadPrivilege(const Entities::User& currentUser,
                                                                        DiscussionThreadPrivilege privilege,
                                                                        PrivilegeValueIntType oldValue,
                                                                        PrivilegeValueIntType newValue) const = 0;
            virtual AuthorizationStatus updateDiscussionTagPrivilege(const Entities::User& currentUser,
                                                                     DiscussionTagPrivilege privilege,
                                                                     PrivilegeValueIntType oldValue,
                                                                     PrivilegeValueIntType newValue) const = 0;
            virtual AuthorizationStatus updateDiscussionCategoryPrivilege(const Entities::User& currentUser,
                                                                          DiscussionCategoryPrivilege privilege,
                                                                          PrivilegeValueIntType oldValue,
                                                                          PrivilegeValueIntType newValue) const = 0;
            virtual AuthorizationStatus updateForumWidePrivilege(const Entities::User& currentUser,
                                                                 ForumWidePrivilege privilege,
                                                                 PrivilegeValueIntType oldValue,
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
}
