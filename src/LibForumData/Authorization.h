#pragma once

#include "TypeHelpers.h"
#include "Entities.h"

#include <memory>

namespace Forum
{
    namespace Authorization
    {
        enum AuthorizationStatusCode : uint_fast32_t
        {
            OK = 0,
            NOT_ALLOWED,
            THROTTLED
        };

        struct AuthorizationStatus
        {
            AuthorizationStatusCode code = AuthorizationStatusCode::OK;
            bool showIP = true;
        };

        class IUserAuthorization
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IUserAuthorization);

            virtual AuthorizationStatus getUsers(const Entities::User& currentUser) const = 0;

            virtual AuthorizationStatus getUserById(const Entities::User& currentUser, 
                                                    const Entities::User& user) const = 0;
            virtual AuthorizationStatus getUserByName(const Entities::User& currentUser,
                                                      const Entities::User& user) const = 0;

            virtual AuthorizationStatus addNewUser(const Entities::User& currentUser, const StringView& name) const = 0;
            virtual AuthorizationStatus changeUserName(const Entities::User& currentUser, 
                                                       const Entities::User& user, const StringView& newName) const = 0;
            virtual AuthorizationStatus changeUserInfo(const Entities::User& currentUser, 
                                                       const Entities::User& user, const StringView& newInfo) const = 0;
            virtual AuthorizationStatus deleteUser(const Entities::User& currentUser, 
                                                   const Entities::User& user) const = 0;
        };
        typedef std::shared_ptr<IUserAuthorization> UserAuthorizationRef;


        class IDiscussionThreadAuthorization
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IDiscussionThreadAuthorization);

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
                                                               const StringView& name) const = 0;
            virtual AuthorizationStatus changeDiscussionThreadName(const Entities::User& currentUser, 
                                                                   const Entities::DiscussionThread& thread, 
                                                                   const StringView& newName) const = 0;
            virtual AuthorizationStatus deleteDiscussionThread(const Entities::User& currentUser, 
                                                               const Entities::DiscussionThread& thread) const = 0;
            virtual AuthorizationStatus mergeDiscussionThreads(const Entities::User& currentUser, 
                                                               const Entities::DiscussionThread& from, 
                                                               const Entities::DiscussionThread& into) const = 0;
            virtual AuthorizationStatus subscribeToDiscussionThread(const Entities::User& currentUser, 
                                                                    const Entities::DiscussionThread& thread) const = 0;
            virtual AuthorizationStatus unsubscribeFromDiscussionThread(const Entities::User& currentUser, 
                                                                        const Entities::DiscussionThread& thread) const = 0;
        };
        typedef std::shared_ptr<IDiscussionThreadAuthorization> DiscussionThreadAuthorizationRef;


        class IDiscussionThreadMessageAuthorization
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IDiscussionThreadMessageAuthorization)

            virtual AuthorizationStatus getDiscussionThreadMessagesOfUserByCreated(const Entities::User& currentUser, 
                                                                                   const Entities::User& user) const = 0;
                                        
            virtual AuthorizationStatus getMessageComments(const Entities::User& currentUser) const = 0;
            virtual AuthorizationStatus getMessageCommentsOfDiscussionThreadMessage(const Entities::User& currentUser,
                                                                                    const Entities::DiscussionThreadMessage& message) const = 0;
            virtual AuthorizationStatus getMessageCommentsOfUser(const Entities::User& currentUser, 
                                                                 const Entities::User& user) const = 0;
                                        
            virtual AuthorizationStatus addNewDiscussionMessageInThread(const Entities::User& currentUser, 
                                                                        const Entities::DiscussionThread& thread,
                                                                        const StringView& content) const = 0;
            virtual AuthorizationStatus deleteDiscussionMessage(const Entities::User& currentUser, 
                                                                const Entities::DiscussionThreadMessage& message) const = 0;
            virtual AuthorizationStatus changeDiscussionThreadMessageContent(const Entities::User& currentUser, 
                                                                             const Entities::DiscussionThreadMessage& message, 
                                                                             const StringView& newContent,
                                                                             const StringView& changeReason) const = 0;
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
                                                                            const StringView& content) const = 0;
            virtual AuthorizationStatus setMessageCommentToSolved(const Entities::User& currentUser, 
                                                                  const Entities::MessageComment& comment) const = 0;
        };
        typedef std::shared_ptr<IDiscussionThreadMessageAuthorization> DiscussionThreadMessageAuthorizationRef;


        class IDiscussionTagAuthorization
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IDiscussionTagAuthorization)

            virtual AuthorizationStatus getDiscussionTags(const Entities::User& currentUser) const = 0;

            virtual AuthorizationStatus addNewDiscussionTag(const Entities::User& currentUser, 
                                                            const StringView& name) const = 0;
            virtual AuthorizationStatus changeDiscussionTagName(const Entities::User& currentUser, 
                                                                const Entities::DiscussionTag& tag, 
                                                                const StringView& newName) const = 0;
            virtual AuthorizationStatus changeDiscussionTagUiBlob(const Entities::User& currentUser, 
                                                                  const Entities::DiscussionTag& tag, 
                                                                  const StringView& blob) const = 0;
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
                                                                 const StringView& name, 
                                                                 const Entities::DiscussionCategoryRef& parent) const = 0;
            virtual AuthorizationStatus changeDiscussionCategoryName(const Entities::User& currentUser, 
                                                                     const Entities::DiscussionCategory& category, 
                                                                     const StringView& newName) const = 0;
            virtual AuthorizationStatus changeDiscussionCategoryDescription(const Entities::User& currentUser, 
                                                                            const Entities::DiscussionCategory& category,
                                                                            const StringView& newDescription) const = 0;
            virtual AuthorizationStatus changeDiscussionCategoryParent(const Entities::User& currentUser, 
                                                                       const Entities::DiscussionCategory& category,
                                                                       const Entities::DiscussionCategoryRef& newParent) const = 0;
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
        };
        typedef std::shared_ptr<IDiscussionCategoryAuthorization> DiscussionCategoryAuthorizationRef;

        class IStatisticsAuthorization
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IStatisticsAuthorization);
            
            virtual AuthorizationStatus getEntitiesCount(const Entities::User& currentUser) const = 0;
        };
        typedef std::shared_ptr<IStatisticsAuthorization> StatisticsAuthorizationRef;


        class IMetricsAuthorization
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IMetricsAuthorization);

            virtual AuthorizationStatus getVersion(const Entities::User& currentUser) const = 0;
        };
        typedef std::shared_ptr<IMetricsAuthorization> MetricsAuthorizationRef;

        //TODO: move to tests after implementing a concrete authorization strategy
        class AllowAllAuthorization : public IUserAuthorization, 
                                      public IDiscussionThreadAuthorization,
                                      public IDiscussionThreadMessageAuthorization,
                                      public IDiscussionTagAuthorization,
                                      public IDiscussionCategoryAuthorization,
                                      public IStatisticsAuthorization,
                                      public IMetricsAuthorization
        {
            AuthorizationStatus getUsers(const Entities::User& currentUser) const override { return{}; }

            AuthorizationStatus getUserById(const Entities::User& currentUser, 
                                            const Entities::User& user) const override { return {}; }
            AuthorizationStatus getUserByName(const Entities::User& currentUser,
                                              const Entities::User& user) const override { return {}; }

            AuthorizationStatus addNewUser(const Entities::User& currentUser, const StringView& name) const override { return {}; }
            AuthorizationStatus changeUserName(const Entities::User& currentUser, 
                                               const Entities::User& user, const StringView& newName) const override { return {}; }
            AuthorizationStatus changeUserInfo(const Entities::User& currentUser, 
                                               const Entities::User& user, const StringView& newInfo) const override { return {}; }
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
                                                       const StringView& name) const override { return {}; }
            AuthorizationStatus changeDiscussionThreadName(const Entities::User& currentUser, 
                                                           const Entities::DiscussionThread& thread, 
                                                           const StringView& newName) const override { return {}; }
            AuthorizationStatus deleteDiscussionThread(const Entities::User& currentUser, 
                                                       const Entities::DiscussionThread& thread) const override { return {}; }
            AuthorizationStatus mergeDiscussionThreads(const Entities::User& currentUser, 
                                                       const Entities::DiscussionThread& from, 
                                                       const Entities::DiscussionThread& into) const override { return {}; }
            AuthorizationStatus subscribeToDiscussionThread(const Entities::User& currentUser, 
                                                            const Entities::DiscussionThread& thread) const override { return {}; }
            AuthorizationStatus unsubscribeFromDiscussionThread(const Entities::User& currentUser, 
                                                                const Entities::DiscussionThread& thread) const override { return {}; }

            AuthorizationStatus getDiscussionThreadMessagesOfUserByCreated(const Entities::User& currentUser, 
                                                                           const Entities::User& user) const override { return {}; }
                                
            AuthorizationStatus getMessageComments(const Entities::User& currentUser) const override { return {}; }
            AuthorizationStatus getMessageCommentsOfDiscussionThreadMessage(const Entities::User& currentUser,
                                                                            const Entities::DiscussionThreadMessage& message) const override { return {}; }
            AuthorizationStatus getMessageCommentsOfUser(const Entities::User& currentUser, 
                                                         const Entities::User& user) const override { return {}; }
                                
            AuthorizationStatus addNewDiscussionMessageInThread(const Entities::User& currentUser, 
                                                                const Entities::DiscussionThread& thread,
                                                                const StringView& content) const override { return {}; }
            AuthorizationStatus deleteDiscussionMessage(const Entities::User& currentUser, 
                                                        const Entities::DiscussionThreadMessage& message) const override { return {}; }
            AuthorizationStatus changeDiscussionThreadMessageContent(const Entities::User& currentUser, 
                                                                     const Entities::DiscussionThreadMessage& message, 
                                                                     const StringView& newContent,
                                                                     const StringView& changeReason) const override { return {}; }
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
                                                                    const StringView& content) const override { return {}; }
            AuthorizationStatus setMessageCommentToSolved(const Entities::User& currentUser, 
                                                          const Entities::MessageComment& comment) const override { return {}; }

            AuthorizationStatus getDiscussionTags(const Entities::User& currentUser) const override { return {}; }

            AuthorizationStatus addNewDiscussionTag(const Entities::User& currentUser, 
                                                    const StringView& name) const override { return {}; }
            AuthorizationStatus changeDiscussionTagName(const Entities::User& currentUser, 
                                                        const Entities::DiscussionTag& tag, 
                                                        const StringView& newName) const override { return {}; }
            AuthorizationStatus changeDiscussionTagUiBlob(const Entities::User& currentUser, 
                                                          const Entities::DiscussionTag& tag, 
                                                          const StringView& blob) const override { return {}; }
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
                                                         const StringView& name, 
                                                         const Entities::DiscussionCategoryRef& parent) const override { return {}; }
            AuthorizationStatus changeDiscussionCategoryName(const Entities::User& currentUser, 
                                                             const Entities::DiscussionCategory& category, 
                                                             const StringView& newName) const override { return {}; }
            AuthorizationStatus changeDiscussionCategoryDescription(const Entities::User& currentUser, 
                                                                    const Entities::DiscussionCategory& category,
                                                                    const StringView& newDescription) const override { return {}; }
            AuthorizationStatus changeDiscussionCategoryParent(const Entities::User& currentUser, 
                                                               const Entities::DiscussionCategory& category,
                                                               const Entities::DiscussionCategoryRef& newParent) const override { return {}; }
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
        };
    }
}