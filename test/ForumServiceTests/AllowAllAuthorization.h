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
                                      public IStatisticsAuthorization,
                                      public IMetricsAuthorization
        {
            AuthorizationStatus login(Entities::IdType userId) const override { return {}; }

            AuthorizationStatus getUsers(const Entities::User& currentUser) const override { return {}; }

            AuthorizationStatus getUserById(const Entities::User& currentUser, 
                                            const Entities::User& user) const override { return {}; }
            AuthorizationStatus getUserByName(const Entities::User& currentUser,
                                              const Entities::User& user) const override { return {}; }

            AuthorizationStatus addNewUser(const Entities::User& currentUser, StringView name) const override { return {}; }
            AuthorizationStatus changeUserName(const Entities::User& currentUser, 
                                               const Entities::User& user, StringView newName) const override { return {}; }
            AuthorizationStatus changeUserInfo(const Entities::User& currentUser, 
                                               const Entities::User& user, StringView newInfo) const override { return {}; }
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
                                                         const Entities::DiscussionCategoryRef& parent) const override { return {}; }
            AuthorizationStatus changeDiscussionCategoryName(const Entities::User& currentUser, 
                                                             const Entities::DiscussionCategory& category, 
                                                             StringView newName) const override { return {}; }
            AuthorizationStatus changeDiscussionCategoryDescription(const Entities::User& currentUser, 
                                                                    const Entities::DiscussionCategory& category,
                                                                    StringView newDescription) const override { return {}; }
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