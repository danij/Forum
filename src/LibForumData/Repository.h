#pragma once

#include "EntityCollection.h"
#include "Observers.h"
#include "TypeHelpers.h"
#include "StringBuffer.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Forum
{
    namespace Repository
    {
        enum StatusCode : uint_fast32_t
        {
            OK = 0,
            INVALID_PARAMETERS,
            VALUE_TOO_LONG,
            VALUE_TOO_SHORT,
            ALREADY_EXISTS,
            NOT_FOUND,
            NO_EFFECT,
            CIRCULAR_REFERENCE_NOT_ALLOWED,
            NOT_ALLOWED,
            NOT_UPDATED_SINCE_LAST_CHECK,
            UNAUTHORIZED,
            THROTTLED,
            USER_WITH_SAME_AUTH_ALREADY_EXISTS
        };

        enum class RetrieveUsersBy
        {
            Name,
            Created,
            LastSeen,
            ThreadCount,
            MessageCount
        };

        enum class RetrieveDiscussionThreadsBy
        {
            Name,
            Created,
            LastUpdated,
            MessageCount
        };

        enum class RetrieveDiscussionTagsBy
        {
            Name,
            MessageCount
        };

        enum class RetrieveDiscussionCategoriesBy
        {
            Name,
            MessageCount
        };

        typedef Json::StringBuffer OutStream;

        template<typename T>
        struct StatusWithResource
        {
            StatusWithResource(T resource, StatusCode status) : resource(std::move(resource)), status(status)
            {
            }

            StatusWithResource(T resource) : StatusWithResource(resource, StatusCode::OK)
            {
            }

            StatusWithResource(StatusCode status) : StatusWithResource({}, status)
            {
            }

            StatusWithResource(const StatusWithResource&) = default;
            StatusWithResource(StatusWithResource&&) = default;

            StatusWithResource& operator=(const StatusWithResource&) = default;
            StatusWithResource& operator=(StatusWithResource&&) = default;

            T resource;
            StatusCode status;
        };

        /**
         * return StatusCode from repository methods so that the code can easily be converted to a HTTP code
         * if needed, without parsing the output 
         */
        
        class IUserRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IUserRepository);

            virtual StatusCode getUsers(OutStream& output, RetrieveUsersBy by) const = 0;

            virtual StatusCode getUserById(Entities::IdTypeRef id, OutStream& output) const = 0;
            virtual StatusCode getUserByName(StringView name, OutStream& output) const = 0;

            virtual StatusCode addNewUser(StringView name, StringView auth, OutStream& output) = 0;
            virtual StatusCode changeUserName(Entities::IdTypeRef id, StringView newName, OutStream& output) = 0;
            virtual StatusCode changeUserInfo(Entities::IdTypeRef id, StringView newInfo, OutStream& output) = 0;
            virtual StatusCode deleteUser(Entities::IdTypeRef id, OutStream& output) = 0;
        };
        typedef std::shared_ptr<IUserRepository> UserRepositoryRef;

        class IUserDirectWriteRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IUserDirectWriteRepository);

            virtual StatusWithResource<Entities::UserRef> addNewUser(Entities::EntityCollection& collection,
                                                                     StringView name, StringView auth) = 0;
            virtual StatusWithResource<Entities::UserRef> changeUserName(Entities::EntityCollection& collection,
                                                                         Entities::IdTypeRef id, StringView newName) = 0;
            virtual StatusWithResource<Entities::UserRef> changeUserInfo(Entities::EntityCollection& collection,
                                                                         Entities::IdTypeRef id, StringView newInfo) = 0;
            virtual StatusCode deleteUser(Entities::EntityCollection& collection, Entities::IdTypeRef id) = 0;
        };
        typedef std::shared_ptr<IUserDirectWriteRepository> UserDirectWriteRepositoryRef;

        class IDiscussionThreadRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IDiscussionThreadRepository);

            virtual StatusCode getDiscussionThreads(OutStream& output, RetrieveDiscussionThreadsBy by) const = 0;
            virtual StatusCode getDiscussionThreadById(Entities::IdTypeRef id, OutStream& output) = 0;

            virtual StatusCode getDiscussionThreadsOfUser(Entities::IdTypeRef id, OutStream& output,
                                                          RetrieveDiscussionThreadsBy by) const = 0;
            virtual StatusCode getSubscribedDiscussionThreadsOfUser(Entities::IdTypeRef id, OutStream& output,
                                                                    RetrieveDiscussionThreadsBy by) const = 0;

            virtual StatusCode getDiscussionThreadsWithTag(Entities::IdTypeRef id, OutStream& output,
                                                           RetrieveDiscussionThreadsBy by) const = 0;

            virtual StatusCode getDiscussionThreadsOfCategory(Entities::IdTypeRef id, OutStream& output,
                                                              RetrieveDiscussionThreadsBy by) const = 0;

            virtual StatusCode addNewDiscussionThread(StringView name, OutStream& output) = 0;
            virtual StatusCode changeDiscussionThreadName(Entities::IdTypeRef id, StringView newName,
                                                          OutStream& output) = 0;
            virtual StatusCode changeDiscussionThreadPinDisplayOrder(Entities::IdTypeRef id, uint16_t newValue,
                                                                     OutStream& output) = 0;
            virtual StatusCode deleteDiscussionThread(Entities::IdTypeRef id, OutStream& output) = 0;
            virtual StatusCode mergeDiscussionThreads(Entities::IdTypeRef fromId, Entities::IdTypeRef intoId,
                                                      OutStream& output) = 0;
            virtual StatusCode subscribeToDiscussionThread(Entities::IdTypeRef id, OutStream& output) = 0;
            virtual StatusCode unsubscribeFromDiscussionThread(Entities::IdTypeRef id, OutStream& output) = 0;
        };
        typedef std::shared_ptr<IDiscussionThreadRepository> DiscussionThreadRepositoryRef;


        class IDiscussionThreadMessageRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IDiscussionThreadMessageRepository)

            virtual StatusCode getDiscussionThreadMessagesOfUserByCreated(Entities::IdTypeRef id,
                                                                          OutStream& output) const = 0;

            virtual StatusCode getMessageComments(OutStream& output) const = 0;
            virtual StatusCode getMessageCommentsOfDiscussionThreadMessage(Entities::IdTypeRef id, 
                                                                           OutStream& output) const = 0;
            virtual StatusCode getMessageCommentsOfUser(Entities::IdTypeRef id,  OutStream& output) const = 0;

            virtual StatusCode addNewDiscussionMessageInThread(Entities::IdTypeRef threadId,
                                                               StringView content, OutStream& output) = 0;
            virtual StatusCode deleteDiscussionMessage(Entities::IdTypeRef id, OutStream& output) = 0;
            virtual StatusCode changeDiscussionThreadMessageContent(Entities::IdTypeRef id, StringView newContent,
                                                                    StringView changeReason, OutStream& output) = 0;
            virtual StatusCode moveDiscussionThreadMessage(Entities::IdTypeRef messageId, 
                                                           Entities::IdTypeRef intoThreadId, OutStream& output) = 0;
            virtual StatusCode upVoteDiscussionThreadMessage(Entities::IdTypeRef id, OutStream& output) = 0;
            virtual StatusCode downVoteDiscussionThreadMessage(Entities::IdTypeRef id, OutStream& output) = 0;
            virtual StatusCode resetVoteDiscussionThreadMessage(Entities::IdTypeRef id, OutStream& output) = 0;

            virtual StatusCode addCommentToDiscussionThreadMessage(Entities::IdTypeRef messageId, 
                                                                   StringView content, OutStream& output) = 0;
            virtual StatusCode setMessageCommentToSolved(Entities::IdTypeRef id, OutStream& output) = 0;
        };
        typedef std::shared_ptr<IDiscussionThreadMessageRepository> DiscussionThreadMessageRepositoryRef;


        class IDiscussionTagRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IDiscussionTagRepository)

            virtual StatusCode getDiscussionTags(OutStream& output, RetrieveDiscussionTagsBy by) const = 0;

            virtual StatusCode addNewDiscussionTag(StringView name, OutStream& output) = 0;
            virtual StatusCode changeDiscussionTagName(Entities::IdTypeRef id, StringView newName,
                                                       OutStream& output) = 0;
            virtual StatusCode changeDiscussionTagUiBlob(Entities::IdTypeRef id, StringView blob,
                                                         OutStream& output) = 0;
            virtual StatusCode deleteDiscussionTag(Entities::IdTypeRef id, OutStream& output) = 0;
            virtual StatusCode addDiscussionTagToThread(Entities::IdTypeRef tagId, Entities::IdTypeRef threadId, 
                                                        OutStream& output) = 0;
            virtual StatusCode removeDiscussionTagFromThread(Entities::IdTypeRef tagId, 
                                                             Entities::IdTypeRef threadId, OutStream& output) = 0;
            virtual StatusCode mergeDiscussionTags(Entities::IdTypeRef fromId, Entities::IdTypeRef intoId,
                                                   OutStream& output) = 0;
        };
        typedef std::shared_ptr<IDiscussionTagRepository> DiscussionTagRepositoryRef;


        class IDiscussionCategoryRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IDiscussionCategoryRepository)

            virtual StatusCode getDiscussionCategoryById(Entities::IdTypeRef id, OutStream& output) const = 0;
            virtual StatusCode getDiscussionCategories(OutStream& output, RetrieveDiscussionCategoriesBy by) const = 0;
            virtual StatusCode getDiscussionCategoriesFromRoot(OutStream& output) const = 0;

            virtual StatusCode addNewDiscussionCategory(StringView name, Entities::IdTypeRef parentId, 
                                                        OutStream& output) = 0;
            virtual StatusCode changeDiscussionCategoryName(Entities::IdTypeRef id, StringView newName,
                                                            OutStream& output) = 0;
            virtual StatusCode changeDiscussionCategoryDescription(Entities::IdTypeRef id, 
                                                                   StringView newDescription,
                                                                   OutStream& output) = 0;
            virtual StatusCode changeDiscussionCategoryParent(Entities::IdTypeRef id, 
                                                              Entities::IdTypeRef newParentId,
                                                              OutStream& output) = 0;
            virtual StatusCode changeDiscussionCategoryDisplayOrder(Entities::IdTypeRef id, 
                                                                    int_fast16_t newDisplayOrder, 
                                                                    OutStream& output) = 0;
            virtual StatusCode deleteDiscussionCategory(Entities::IdTypeRef id, OutStream& output) = 0;
            virtual StatusCode addDiscussionTagToCategory(Entities::IdTypeRef tagId, 
                                                          Entities::IdTypeRef categoryId, OutStream& output) = 0;
            virtual StatusCode removeDiscussionTagFromCategory(Entities::IdTypeRef tagId, 
                                                               Entities::IdTypeRef categoryId, 
                                                               OutStream& output) = 0;
        };
        typedef std::shared_ptr<IDiscussionCategoryRepository> DiscussionCategoryRepositoryRef;


        class IObservableRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IObservableRepository)

            virtual ReadEvents& readEvents() = 0;
            virtual WriteEvents& writeEvents() = 0;
        };
        typedef std::shared_ptr<IObservableRepository> ObservableRepositoryRef;

        class IStatisticsRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IStatisticsRepository);
            
            virtual StatusCode getEntitiesCount(OutStream& output) const = 0;
        };
        typedef std::shared_ptr<IStatisticsRepository> StatisticsRepositoryRef;


        class IMetricsRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IMetricsRepository);

            virtual StatusCode getVersion(OutStream& output) = 0;
        };
        typedef std::shared_ptr<IMetricsRepository> MetricsRepositoryRef;
    }
}
