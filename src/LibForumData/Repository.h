#pragma once

#include "Observers.h"
#include "TypeHelpers.h"

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>

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
            LastSeen
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

        /**
         * return StatusCode from repository methods so that the code can easily be converted to a HTTP code
         * if needed, without parsing the output 
         */
        
        class IReadRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IReadRepository);

            virtual ReadEvents& readEvents() = 0;

            virtual StatusCode getEntitiesCount(std::ostream& output) const = 0;

            virtual StatusCode getUsers(std::ostream& output, RetrieveUsersBy by) const = 0;

            virtual StatusCode getUserById(const Entities::IdType& id, std::ostream& output) const = 0;
            virtual StatusCode getUserByName(const std::string& name, std::ostream& output) const = 0;

            virtual StatusCode getDiscussionThreads(std::ostream& output, RetrieveDiscussionThreadsBy by) const = 0;
            virtual StatusCode getDiscussionThreadById(const Entities::IdType& id, std::ostream& output) = 0;

            virtual StatusCode getDiscussionThreadsOfUser(const Entities::IdType& id, std::ostream& output, 
                                                          RetrieveDiscussionThreadsBy by) const = 0;

            virtual StatusCode getDiscussionThreadMessagesOfUserByCreated(const Entities::IdType& id,
                                                                          std::ostream& output) const = 0;

            virtual StatusCode getDiscussionTags(std::ostream& output, RetrieveDiscussionTagsBy by) const = 0;
            virtual StatusCode getDiscussionThreadsWithTag(const Entities::IdType& id, std::ostream& output, 
                                                           RetrieveDiscussionThreadsBy by) const = 0;

            virtual StatusCode getDiscussionCategoryById(const Entities::IdType& id, std::ostream& output) const = 0;
            virtual StatusCode getDiscussionCategories(std::ostream& output, RetrieveDiscussionCategoriesBy by) const = 0;
            virtual StatusCode getDiscussionCategoriesFromRoot(std::ostream& output) const = 0;

            virtual StatusCode getDiscussionThreadsOfCategory(const Entities::IdType& id, std::ostream& output, 
                                                              RetrieveDiscussionThreadsBy by) const = 0;

        };

        typedef std::shared_ptr<IReadRepository> ReadRepositoryRef;

        class IWriteRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IWriteRepository);

            virtual WriteEvents& writeEvents() = 0;

            virtual StatusCode addNewUser(const std::string& name, std::ostream& output) = 0;
            virtual StatusCode changeUserName(const Entities::IdType& id, const std::string& newName,
                                              std::ostream& output) = 0;
            virtual StatusCode changeUserInfo(const Entities::IdType& id, const std::string& newInfo,
                                              std::ostream& output) = 0;
            virtual StatusCode deleteUser(const Entities::IdType& id, std::ostream& output) = 0;

            virtual StatusCode addNewDiscussionThread(const std::string& name, std::ostream& output) = 0;
            virtual StatusCode changeDiscussionThreadName(const Entities::IdType& id, const std::string& newName,
                                                          std::ostream& output) = 0;
            virtual StatusCode deleteDiscussionThread(const Entities::IdType& id, std::ostream& output) = 0;
            virtual StatusCode mergeDiscussionThreads(const Entities::IdType& fromId, const Entities::IdType& intoId,
                                                      std::ostream& output) = 0;

            virtual StatusCode addNewDiscussionMessageInThread(const Entities::IdType& threadId,
                                                               const std::string& content, std::ostream& output) = 0;
            virtual StatusCode deleteDiscussionMessage(const Entities::IdType& id, std::ostream& output) = 0;
            virtual StatusCode changeDiscussionThreadMessageContent(const Entities::IdType& id, const std::string& newContent,
                                                                    const std::string& changeReason, std::ostream& output) = 0;
            virtual StatusCode moveDiscussionThreadMessage(const Entities::IdType& messageId, 
                                                           const Entities::IdType& intoThreadId, std::ostream& output) = 0;
            virtual StatusCode upVoteDiscussionThreadMessage(const Entities::IdType& id, std::ostream& output) = 0;
            virtual StatusCode downVoteDiscussionThreadMessage(const Entities::IdType& id, std::ostream& output) = 0;
            virtual StatusCode resetVoteDiscussionThreadMessage(const Entities::IdType& id, std::ostream& output) = 0;

            virtual StatusCode addNewDiscussionTag(const std::string& name, std::ostream& output) = 0;
            virtual StatusCode changeDiscussionTagName(const Entities::IdType& id, const std::string& newName,
                                                       std::ostream& output) = 0;
            virtual StatusCode changeDiscussionTagUiBlob(const Entities::IdType& id, const std::string& blob,
                                                         std::ostream& output) = 0;
            virtual StatusCode deleteDiscussionTag(const Entities::IdType& id, std::ostream& output) = 0;
            virtual StatusCode addDiscussionTagToThread(const Entities::IdType& tagId, const Entities::IdType& threadId, 
                                                        std::ostream& output) = 0;
            virtual StatusCode removeDiscussionTagFromThread(const Entities::IdType& tagId, 
                                                             const Entities::IdType& threadId, std::ostream& output) = 0;
            virtual StatusCode mergeDiscussionTags(const Entities::IdType& fromId, const Entities::IdType& intoId,
                                                   std::ostream& output) = 0;

            virtual StatusCode addNewDiscussionCategory(const std::string& name, const Entities::IdType& parentId, 
                                                        std::ostream& output) = 0;
            virtual StatusCode changeDiscussionCategoryName(const Entities::IdType& id, const std::string& newName,
                                                            std::ostream& output) = 0;
            virtual StatusCode changeDiscussionCategoryDescription(const Entities::IdType& id, 
                                                                   const std::string& newDescription,
                                                                   std::ostream& output) = 0;
            virtual StatusCode changeDiscussionCategoryParent(const Entities::IdType& id, 
                                                              const Entities::IdType& newParentId,
                                                              std::ostream& output) = 0;
            virtual StatusCode changeDiscussionCategoryDisplayOrder(const Entities::IdType& id, 
                                                                    int_fast16_t newDisplayOrder, 
                                                                    std::ostream& output) = 0;
            virtual StatusCode deleteDiscussionCategory(const Entities::IdType& id, std::ostream& output) = 0;
            virtual StatusCode addDiscussionTagToCategory(const Entities::IdType& tagId, 
                                                          const Entities::IdType& categoryId, std::ostream& output) = 0;
            virtual StatusCode removeDiscussionTagFromCategory(const Entities::IdType& tagId, 
                                                               const Entities::IdType& categoryId, 
                                                               std::ostream& output) = 0;

        };

        typedef std::shared_ptr<IWriteRepository> WriteRepositoryRef;


        class IMetricsRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IMetricsRepository);

            virtual StatusCode getVersion(std::ostream& output) = 0;
        };

        typedef std::shared_ptr<IMetricsRepository> MetricsRepositoryRef;
    }
}
