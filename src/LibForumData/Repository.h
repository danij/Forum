#pragma once

#include "Observers.h"
#include "TypeHelpers.h"

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
            NOT_UPDATED_SINCE_LAST_CHECK
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

        typedef std::string OutStream;

        /**
         * return StatusCode from repository methods so that the code can easily be converted to a HTTP code
         * if needed, without parsing the output 
         */
        
        class IUserRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IUserRepository);

            virtual StatusCode getUsers(OutStream& output, RetrieveUsersBy by) const = 0;

            virtual StatusCode getUserById(const Entities::IdType& id, OutStream& output) const = 0;
            virtual StatusCode getUserByName(const StringView& name, OutStream& output) const = 0;

            virtual StatusCode addNewUser(const StringView& name, OutStream& output) = 0;
            virtual StatusCode changeUserName(const Entities::IdType& id, const StringView& newName,
                                              OutStream& output) = 0;
            virtual StatusCode changeUserInfo(const Entities::IdType& id, const StringView& newInfo,
                                              OutStream& output) = 0;
            virtual StatusCode deleteUser(const Entities::IdType& id, OutStream& output) = 0;
        };
        typedef std::shared_ptr<IUserRepository> UserRepositoryRef;


        class IDiscussionThreadRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IDiscussionThreadRepository);

            virtual StatusCode getDiscussionThreads(OutStream& output, RetrieveDiscussionThreadsBy by) const = 0;
            virtual StatusCode getDiscussionThreadById(const Entities::IdType& id, OutStream& output) = 0;

            virtual StatusCode getDiscussionThreadsOfUser(const Entities::IdType& id, OutStream& output,
                                                          RetrieveDiscussionThreadsBy by) const = 0;

            virtual StatusCode getDiscussionThreadsWithTag(const Entities::IdType& id, OutStream& output,
                                                           RetrieveDiscussionThreadsBy by) const = 0;

            virtual StatusCode getDiscussionThreadsOfCategory(const Entities::IdType& id, OutStream& output,
                                                              RetrieveDiscussionThreadsBy by) const = 0;

            virtual StatusCode addNewDiscussionThread(const StringView& name, OutStream& output) = 0;
            virtual StatusCode changeDiscussionThreadName(const Entities::IdType& id, const StringView& newName,
                                                          OutStream& output) = 0;
            virtual StatusCode deleteDiscussionThread(const Entities::IdType& id, OutStream& output) = 0;
            virtual StatusCode mergeDiscussionThreads(const Entities::IdType& fromId, const Entities::IdType& intoId,
                                                      OutStream& output) = 0;
        };
        typedef std::shared_ptr<IDiscussionThreadRepository> DiscussionThreadRepositoryRef;


        class IDiscussionThreadMessageRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IDiscussionThreadMessageRepository)

            virtual StatusCode getDiscussionThreadMessagesOfUserByCreated(const Entities::IdType& id,
                                                                          OutStream& output) const = 0;

            virtual StatusCode getMessageComments(OutStream& output) const = 0;
            virtual StatusCode getMessageCommentsOfDiscussionThreadMessage(const Entities::IdType& id, 
                                                                           OutStream& output) const = 0;
            virtual StatusCode getMessageCommentsOfUser(const Entities::IdType& id,  OutStream& output) const = 0;

            virtual StatusCode addNewDiscussionMessageInThread(const Entities::IdType& threadId,
                                                               const StringView& content, OutStream& output) = 0;
            virtual StatusCode deleteDiscussionMessage(const Entities::IdType& id, OutStream& output) = 0;
            virtual StatusCode changeDiscussionThreadMessageContent(const Entities::IdType& id, const StringView& newContent,
                                                                    const StringView& changeReason, OutStream& output) = 0;
            virtual StatusCode moveDiscussionThreadMessage(const Entities::IdType& messageId, 
                                                           const Entities::IdType& intoThreadId, OutStream& output) = 0;
            virtual StatusCode upVoteDiscussionThreadMessage(const Entities::IdType& id, OutStream& output) = 0;
            virtual StatusCode downVoteDiscussionThreadMessage(const Entities::IdType& id, OutStream& output) = 0;
            virtual StatusCode resetVoteDiscussionThreadMessage(const Entities::IdType& id, OutStream& output) = 0;

            virtual StatusCode addCommentToDiscussionThreadMessage(const Entities::IdType& messageId, 
                                                                   const StringView& content, OutStream& output) = 0;
            virtual StatusCode setMessageCommentToSolved(const Entities::IdType& id, OutStream& output) = 0;
        };
        typedef std::shared_ptr<IDiscussionThreadMessageRepository> DiscussionThreadMessageRepositoryRef;


        class IDiscussionTagRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IDiscussionTagRepository)

            virtual StatusCode getDiscussionTags(OutStream& output, RetrieveDiscussionTagsBy by) const = 0;

            virtual StatusCode addNewDiscussionTag(const StringView& name, OutStream& output) = 0;
            virtual StatusCode changeDiscussionTagName(const Entities::IdType& id, const StringView& newName,
                                                       OutStream& output) = 0;
            virtual StatusCode changeDiscussionTagUiBlob(const Entities::IdType& id, const StringView& blob,
                                                         OutStream& output) = 0;
            virtual StatusCode deleteDiscussionTag(const Entities::IdType& id, OutStream& output) = 0;
            virtual StatusCode addDiscussionTagToThread(const Entities::IdType& tagId, const Entities::IdType& threadId, 
                                                        OutStream& output) = 0;
            virtual StatusCode removeDiscussionTagFromThread(const Entities::IdType& tagId, 
                                                             const Entities::IdType& threadId, OutStream& output) = 0;
            virtual StatusCode mergeDiscussionTags(const Entities::IdType& fromId, const Entities::IdType& intoId,
                                                   OutStream& output) = 0;
        };
        typedef std::shared_ptr<IDiscussionTagRepository> DiscussionTagRepositoryRef;


        class IDiscussionCategoryRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IDiscussionCategoryRepository)

            virtual StatusCode getDiscussionCategoryById(const Entities::IdType& id, OutStream& output) const = 0;
            virtual StatusCode getDiscussionCategories(OutStream& output, RetrieveDiscussionCategoriesBy by) const = 0;
            virtual StatusCode getDiscussionCategoriesFromRoot(OutStream& output) const = 0;

            virtual StatusCode addNewDiscussionCategory(const StringView& name, const Entities::IdType& parentId, 
                                                        OutStream& output) = 0;
            virtual StatusCode changeDiscussionCategoryName(const Entities::IdType& id, const StringView& newName,
                                                            OutStream& output) = 0;
            virtual StatusCode changeDiscussionCategoryDescription(const Entities::IdType& id, 
                                                                   const StringView& newDescription,
                                                                   OutStream& output) = 0;
            virtual StatusCode changeDiscussionCategoryParent(const Entities::IdType& id, 
                                                              const Entities::IdType& newParentId,
                                                              OutStream& output) = 0;
            virtual StatusCode changeDiscussionCategoryDisplayOrder(const Entities::IdType& id, 
                                                                    int_fast16_t newDisplayOrder, 
                                                                    OutStream& output) = 0;
            virtual StatusCode deleteDiscussionCategory(const Entities::IdType& id, OutStream& output) = 0;
            virtual StatusCode addDiscussionTagToCategory(const Entities::IdType& tagId, 
                                                          const Entities::IdType& categoryId, OutStream& output) = 0;
            virtual StatusCode removeDiscussionTagFromCategory(const Entities::IdType& tagId, 
                                                               const Entities::IdType& categoryId, 
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
