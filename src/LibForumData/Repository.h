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
        class IReadRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IReadRepository);

            virtual ReadEvents& readEvents() = 0;

            virtual void getEntitiesCount(std::ostream& output) const = 0;

            virtual void getUsersByName(std::ostream& output) const = 0;
            virtual void getUsersByCreated(std::ostream& output) const = 0;
            virtual void getUsersByLastSeen(std::ostream& output) const = 0;

            virtual void getUserById(const Entities::IdType& id, std::ostream& output) const = 0;
            virtual void getUserByName(const std::string& name, std::ostream& output) const = 0;

            virtual void getDiscussionThreadsByName(std::ostream& output) const = 0;
            virtual void getDiscussionThreadsByCreated(std::ostream& output) const = 0;
            virtual void getDiscussionThreadsByLastUpdated(std::ostream& output) const = 0;
            virtual void getDiscussionThreadsByMessageCount(std::ostream& output) const = 0;
            virtual void getDiscussionThreadById(const Entities::IdType& id, std::ostream& output) = 0;

            virtual void getDiscussionThreadsOfUserByName(const Entities::IdType& id,
                                                          std::ostream& output) const = 0;
            virtual void getDiscussionThreadsOfUserByCreated(const Entities::IdType& id,
                                                             std::ostream& output) const = 0;
            virtual void getDiscussionThreadsOfUserByLastUpdated(const Entities::IdType& id,
                                                                 std::ostream& output) const = 0;
            virtual void getDiscussionThreadsOfUserByMessageCount(const Entities::IdType& id,
                                                                  std::ostream& output) const = 0;

            virtual void getDiscussionThreadMessagesOfUserByCreated(const Entities::IdType& id,
                                                                    std::ostream& output) const = 0;

            virtual void getDiscussionTagsByName(std::ostream& output) const = 0;
            virtual void getDiscussionTagsByMessageCount(std::ostream& output) const = 0;
            virtual void getDiscussionThreadsWithTagByName(const Entities::IdType& id, std::ostream& output) const = 0;
            virtual void getDiscussionThreadsWithTagByCreated(const Entities::IdType& id, 
                                                              std::ostream& output) const = 0;
            virtual void getDiscussionThreadsWithTagByLastUpdated(const Entities::IdType& id, 
                                                                  std::ostream& output) const = 0;
            virtual void getDiscussionThreadsWithTagByMessageCount(const Entities::IdType& id, 
                                                                   std::ostream& output) const = 0;

            virtual void getDiscussionCategoryById(const Entities::IdType& id, std::ostream& output) const = 0;
            virtual void getDiscussionCategoriesByName(std::ostream& output) const = 0;
            virtual void getDiscussionCategoriesByMessageCount(std::ostream& output) const = 0;
            virtual void getDiscussionCategoriesFromRoot(std::ostream& output) const = 0;

            virtual void getDiscussionThreadsOfCategoryByName(const Entities::IdType& id,
                                                              std::ostream& output) const = 0;
            virtual void getDiscussionThreadsOfCategoryByCreated(const Entities::IdType& id,
                                                                 std::ostream& output) const = 0;
            virtual void getDiscussionThreadsOfCategoryByLastUpdated(const Entities::IdType& id,
                                                                     std::ostream& output) const = 0;
            virtual void getDiscussionThreadsOfCategoryByMessageCount(const Entities::IdType& id,
                                                                      std::ostream& output) const = 0;

        };

        typedef std::shared_ptr<IReadRepository> ReadRepositoryRef;

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
            NOT_ALLOWED
        };

        class IWriteRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IWriteRepository);

            virtual WriteEvents& writeEvents() = 0;

            virtual void addNewUser(const std::string& name, std::ostream& output) = 0;
            virtual void changeUserName(const Entities::IdType& id, const std::string& newName,
                                        std::ostream& output) = 0;
            virtual void changeUserInfo(const Entities::IdType& id, const std::string& newInfo,
                                        std::ostream& output) = 0;
            virtual void deleteUser(const Entities::IdType& id, std::ostream& output) = 0;

            virtual void addNewDiscussionThread(const std::string& name, std::ostream& output) = 0;
            virtual void changeDiscussionThreadName(const Entities::IdType& id, const std::string& newName,
                                                    std::ostream& output) = 0;
            virtual void deleteDiscussionThread(const Entities::IdType& id, std::ostream& output) = 0;
            virtual void mergeDiscussionThreads(const Entities::IdType& fromId, const Entities::IdType& intoId,
                                                std::ostream& output) = 0;

            virtual void addNewDiscussionMessageInThread(const Entities::IdType& threadId,
                                                         const std::string& content, std::ostream& output) = 0;
            virtual void deleteDiscussionMessage(const Entities::IdType& id, std::ostream& output) = 0;
            virtual void changeDiscussionThreadMessageContent(const Entities::IdType& id, const std::string& newContent,
                                                              const std::string& changeReason, std::ostream& output) = 0;
            virtual void moveDiscussionThreadMessage(const Entities::IdType& messageId, 
                                                     const Entities::IdType& intoThreadId, std::ostream& output) = 0;
            virtual void upVoteDiscussionThreadMessage(const Entities::IdType& id, std::ostream& output) = 0;
            virtual void downVoteDiscussionThreadMessage(const Entities::IdType& id, std::ostream& output) = 0;
            virtual void resetVoteDiscussionThreadMessage(const Entities::IdType& id, std::ostream& output) = 0;

            virtual void addNewDiscussionTag(const std::string& name, std::ostream& output) = 0;
            virtual void changeDiscussionTagName(const Entities::IdType& id, const std::string& newName,
                                                 std::ostream& output) = 0;
            virtual void changeDiscussionTagUiBlob(const Entities::IdType& id, const std::string& blob,
                                                   std::ostream& output) = 0;
            virtual void deleteDiscussionTag(const Entities::IdType& id, std::ostream& output) = 0;
            virtual void addDiscussionTagToThread(const Entities::IdType& tagId, const Entities::IdType& threadId, 
                                                  std::ostream& output) = 0;
            virtual void removeDiscussionTagFromThread(const Entities::IdType& tagId, const Entities::IdType& threadId, 
                                                       std::ostream& output) = 0;
            virtual void mergeDiscussionTags(const Entities::IdType& fromId, const Entities::IdType& intoId,
                                             std::ostream& output) = 0;

            virtual void addNewDiscussionCategory(const std::string& name, const Entities::IdType& parentId, 
                                                  std::ostream& output) = 0;
            virtual void changeDiscussionCategoryName(const Entities::IdType& id, const std::string& newName,
                                                      std::ostream& output) = 0;
            virtual void changeDiscussionCategoryDescription(const Entities::IdType& id, const std::string& newDescription,
                                                             std::ostream& output) = 0;
            virtual void changeDiscussionCategoryParent(const Entities::IdType& id, const Entities::IdType& newParentId,
                                                        std::ostream& output) = 0;
            virtual void changeDiscussionCategoryDisplayOrder(const Entities::IdType& id, int_fast16_t newDisplayOrder,
                                                              std::ostream& output) = 0;
            virtual void deleteDiscussionCategory(const Entities::IdType& id, std::ostream& output) = 0;
            virtual void addDiscussionTagToCategory(const Entities::IdType& tagId, const Entities::IdType& categoryId, 
                                                    std::ostream& output) = 0;
            virtual void removeDiscussionTagFromCategory(const Entities::IdType& tagId, const Entities::IdType& categoryId, 
                                                         std::ostream& output) = 0;

        };

        typedef std::shared_ptr<IWriteRepository> WriteRepositoryRef;


        class IMetricsRepository
        {
        public:
            DECLARE_INTERFACE_MANDATORY(IMetricsRepository);

            virtual void getVersion(std::ostream& output) = 0;
        };

        typedef std::shared_ptr<IMetricsRepository> MetricsRepositoryRef;
    }
}
