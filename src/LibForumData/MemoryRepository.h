#pragma once

#include "ContextProviders.h"
#include "EntityCollection.h"
#include "Observers.h"
#include "Repository.h"
#include "ResourceGuard.h"

#include <boost/core/noncopyable.hpp>
#include <boost/regex/icu.hpp>

namespace Forum
{
    namespace Repository
    {
        class MemoryRepository;

        /**
        * Retrieves the user that is performing the current action and also performs an update on the last seen if needed
        * The update is performed on the spot if a write lock is held or
        * delayed until the lock is destroyed in the case of a read lock, to avoid deadlocks
        * Do not keep references to it outside of MemoryRepository methods
        */
        struct PerformedByWithLastSeenUpdateGuard final
        {
            explicit PerformedByWithLastSeenUpdateGuard(const MemoryRepository& repository);
            ~PerformedByWithLastSeenUpdateGuard();

            /**
            * Get the current user that performs the action and optionally schedule the update of last seen
            */
            PerformedByType get(const Entities::EntityCollection& collection);

            /**
            * Get the current user that performs the action and optionally also perform the update of last seen
            * This method takes advantage if a write lock on the collection is already secured
            */
            Entities::UserRef getAndUpdate(Entities::EntityCollection& collection);

        private:
            MemoryRepository& repository_;
            std::function<void()> lastSeenUpdate_;
        };

        class MemoryRepository final : public IReadRepository, public IWriteRepository, private boost::noncopyable
        {
        public:
            MemoryRepository();

            virtual ReadEvents& readEvents() override;
            virtual WriteEvents& writeEvents() override;

            virtual StatusCode getEntitiesCount(std::ostream& output) const override;

            virtual StatusCode getUsers(std::ostream& output, RetrieveUsersBy by) const override;

            virtual StatusCode getUserById(const Entities::IdType& id, std::ostream& output) const override;
            virtual StatusCode getUserByName(const std::string& name, std::ostream& output) const override;

            virtual StatusCode addNewUser(const std::string& name, std::ostream& output) override;
            virtual StatusCode changeUserName(const Entities::IdType& id, const std::string& newName,
                                              std::ostream& output) override;
            virtual StatusCode changeUserInfo(const Entities::IdType& id, const std::string& newInfo,
                                              std::ostream& output) override;
            virtual StatusCode deleteUser(const Entities::IdType& id, std::ostream& output) override;

            virtual StatusCode getDiscussionThreads(std::ostream& output, RetrieveDiscussionThreadsBy by) const override;

            /**
             * Calling the function changes state:
             * - Increases the number of visits
             * - Stores that the current user has visited the discussion thread
             */
            virtual StatusCode getDiscussionThreadById(const Entities::IdType& id, std::ostream& output) override;

            virtual StatusCode getDiscussionThreadsOfUser(const Entities::IdType& id, std::ostream& output, 
                                                          RetrieveDiscussionThreadsBy by) const override;

            virtual StatusCode addNewDiscussionThread(const std::string& name, std::ostream& output) override;
            virtual StatusCode changeDiscussionThreadName(const Entities::IdType& id, const std::string& newName,
                                                          std::ostream& output) override;
            virtual StatusCode deleteDiscussionThread(const Entities::IdType& id, std::ostream& output) override;
            virtual StatusCode mergeDiscussionThreads(const Entities::IdType& fromId, const Entities::IdType& intoId, 
                                                      std::ostream& output) override;

            virtual StatusCode addNewDiscussionMessageInThread(const Entities::IdType& threadId,
                                                               const std::string& content, std::ostream& output) override;
            virtual StatusCode deleteDiscussionMessage(const Entities::IdType& id, std::ostream& output) override;
            virtual StatusCode changeDiscussionThreadMessageContent(const Entities::IdType& id, 
                                                                    const std::string& newContent,
                                                                    const std::string& changeReason, 
                                                                    std::ostream& output) override;
            virtual StatusCode moveDiscussionThreadMessage(const Entities::IdType& messageId, 
                                                           const Entities::IdType& intoThreadId, 
                                                           std::ostream& output) override ;
            virtual StatusCode upVoteDiscussionThreadMessage(const Entities::IdType& id, std::ostream& output) override;
            virtual StatusCode downVoteDiscussionThreadMessage(const Entities::IdType& id, std::ostream& output) override;
            virtual StatusCode resetVoteDiscussionThreadMessage(const Entities::IdType& id, std::ostream& output) override;

            virtual StatusCode getDiscussionThreadMessagesOfUserByCreated(const Entities::IdType& id,
                                                                          std::ostream& output) const override;

            virtual StatusCode getDiscussionTags(std::ostream& output, RetrieveDiscussionTagsBy by) const override;
            
            virtual StatusCode addNewDiscussionTag(const std::string& name, std::ostream& output) override;
            virtual StatusCode changeDiscussionTagName(const Entities::IdType& id, const std::string& newName,
                                                       std::ostream& output) override;
            virtual StatusCode changeDiscussionTagUiBlob(const Entities::IdType& id, const std::string& blob,
                                                         std::ostream& output) override;
            virtual StatusCode deleteDiscussionTag(const Entities::IdType& id, std::ostream& output) override;

            virtual StatusCode getDiscussionThreadsWithTag(const Entities::IdType& id, std::ostream& output,
                                                           RetrieveDiscussionThreadsBy by) const override;
            
            virtual StatusCode addDiscussionTagToThread(const Entities::IdType& tagId, const Entities::IdType& threadId, 
                                                        std::ostream& output) override;
            virtual StatusCode removeDiscussionTagFromThread(const Entities::IdType& tagId, 
                                                             const Entities::IdType& threadId, 
                                                             std::ostream& output) override;
            virtual StatusCode mergeDiscussionTags(const Entities::IdType& fromId, const Entities::IdType& intoId,
                                                   std::ostream& output) override;

            virtual StatusCode addNewDiscussionCategory(const std::string& name, const Entities::IdType& parentId,
                                                        std::ostream& output) override;
            virtual StatusCode changeDiscussionCategoryName(const Entities::IdType& id, const std::string& newName,
                                                            std::ostream& output) override;
            virtual StatusCode changeDiscussionCategoryDescription(const Entities::IdType& id, 
                                                                   const std::string& newDescription,
                                                                   std::ostream& output) override;
            virtual StatusCode changeDiscussionCategoryParent(const Entities::IdType& id, 
                                                              const Entities::IdType& newParentId,
                                                              std::ostream& output) override;
            virtual StatusCode changeDiscussionCategoryDisplayOrder(const Entities::IdType& id, 
                                                                    int_fast16_t newDisplayOrder,
                                                                    std::ostream& output) override;
            virtual StatusCode deleteDiscussionCategory(const Entities::IdType& id, std::ostream& output) override;

            virtual StatusCode getDiscussionCategoryById(const Entities::IdType& id, std::ostream& output) const override;
            virtual StatusCode getDiscussionCategories(std::ostream& output, RetrieveDiscussionCategoriesBy by) const override;
            virtual StatusCode getDiscussionCategoriesFromRoot(std::ostream& output) const override;

            virtual StatusCode addDiscussionTagToCategory(const Entities::IdType& tagId, 
                                                          const Entities::IdType& categoryId, 
                                                          std::ostream& output) override;
            virtual StatusCode removeDiscussionTagFromCategory(const Entities::IdType& tagId, 
                                                               const Entities::IdType& categoryId, 
                                                               std::ostream& output) override;
            virtual StatusCode getDiscussionThreadsOfCategory(const Entities::IdType& id, std::ostream& output,
                                                              RetrieveDiscussionThreadsBy by) const override;


        private:
            StatusCode voteDiscussionThreadMessage(const Entities::IdType& id, std::ostream& output, bool up);

            friend struct PerformedByWithLastSeenUpdateGuard;

            PerformedByWithLastSeenUpdateGuard preparePerformedBy() const
            {
                return PerformedByWithLastSeenUpdateGuard(*this);
            }

            static void updateCreated(Entities::CreatedMixin& entity)
            {
                entity.created() = Context::getCurrentTime();
                entity.creationDetails().ip = Context::getCurrentUserIpAddress();
                entity.creationDetails().userAgent = Context::getCurrentUserBrowserUserAgent();
            }

            template<typename ByType>
            static void updateLastUpdated(Entities::LastUpdatedMixin<ByType>& entity, 
                                          const typename Entities::LastUpdatedMixin<ByType>::ByTypeRef& by)
            {
                entity.lastUpdated() = Context::getCurrentTime();
                entity.lastUpdatedDetails().ip = Context::getCurrentUserIpAddress();
                entity.lastUpdatedDetails().userAgent = Context::getCurrentUserBrowserUserAgent();
                entity.lastUpdatedBy() = by;
            }

            Helpers::ResourceGuard<Entities::EntityCollection> collection_;
            ReadEvents readEvents_;
            WriteEvents writeEvents_;

            boost::u32regex validUserNameRegex;
            boost::u32regex validDiscussionThreadNameRegex;
            boost::u32regex validDiscussionMessageContentRegex;
            boost::u32regex validDiscussionMessageChangeReasonRegex;
            boost::u32regex validDiscussionTagNameRegex;
            boost::u32regex validDiscussionCategoryNameRegex;
        };

        inline ObserverContext_ createObserverContext(PerformedByType performedBy)
        {
            return ObserverContext_(performedBy, Context::getCurrentTime(), Context::getDisplayContext());
        }
    }
}
