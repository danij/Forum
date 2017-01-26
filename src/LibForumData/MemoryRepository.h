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

            virtual void getEntitiesCount(std::ostream& output) const override;

            virtual void getUsersByName(std::ostream& output) const override;
            virtual void getUsersByCreated(std::ostream& output) const override;
            virtual void getUsersByLastSeen(std::ostream& output) const override;

            virtual void getUserById(const Entities::IdType& id, std::ostream& output) const override;
            virtual void getUserByName(const std::string& name, std::ostream& output) const override;

            virtual void addNewUser(const std::string& name, std::ostream& output) override;
            virtual void changeUserName(const Entities::IdType& id, const std::string& newName,
                                        std::ostream& output) override;
            virtual void changeUserInfo(const Entities::IdType& id, const std::string& newInfo,
                                        std::ostream& output) override;
            virtual void deleteUser(const Entities::IdType& id, std::ostream& output) override;

            virtual void getDiscussionThreadsByName(std::ostream& output) const override;
            virtual void getDiscussionThreadsByCreated(std::ostream& output) const override;
            virtual void getDiscussionThreadsByLastUpdated(std::ostream& output) const override;
            virtual void getDiscussionThreadsByMessageCount(std::ostream& output) const override;

            /**
             * Calling the function changes state:
             * - Increases the number of visits
             * - Stores that the current user has visited the discussion thread
             */
            virtual void getDiscussionThreadById(const Entities::IdType& id, std::ostream& output) override;

            virtual void getDiscussionThreadsOfUserByName(const Entities::IdType& id,
                                                          std::ostream& output) const override;
            virtual void getDiscussionThreadsOfUserByCreated(const Entities::IdType& id,
                                                             std::ostream& output) const override;
            virtual void getDiscussionThreadsOfUserByLastUpdated(const Entities::IdType& id,
                                                                 std::ostream& output) const override;
            virtual void getDiscussionThreadsOfUserByMessageCount(const Entities::IdType& id,
                                                                  std::ostream& output) const override;

            virtual void addNewDiscussionThread(const std::string& name, std::ostream& output) override;
            virtual void changeDiscussionThreadName(const Entities::IdType& id, const std::string& newName,
                                                    std::ostream& output) override;
            virtual void deleteDiscussionThread(const Entities::IdType& id, std::ostream& output) override;
            virtual void mergeDiscussionThreads(const Entities::IdType& fromId, const Entities::IdType& intoId, 
                                                std::ostream& output) override;

            virtual void addNewDiscussionMessageInThread(const Entities::IdType& threadId,
                                                         const std::string& content, std::ostream& output) override;
            virtual void deleteDiscussionMessage(const Entities::IdType& id, std::ostream& output) override;
            virtual void changeDiscussionThreadMessageContent(const Entities::IdType& id, const std::string& newContent,
                                                              const std::string& changeReason, std::ostream& output) override;
            virtual void moveDiscussionThreadMessage(const Entities::IdType& messageId, 
                                                     const Entities::IdType& intoThreadId, std::ostream& output) override ;
            virtual void upVoteDiscussionThreadMessage(const Entities::IdType& id, std::ostream& output) override;
            virtual void downVoteDiscussionThreadMessage(const Entities::IdType& id, std::ostream& output) override;
            virtual void resetVoteDiscussionThreadMessage(const Entities::IdType& id, std::ostream& output) override;

            virtual void getDiscussionThreadMessagesOfUserByCreated(const Entities::IdType& id,
                                                                    std::ostream& output) const override;

            virtual void getDiscussionTagsByName(std::ostream& output) const override;
            virtual void getDiscussionTagsByMessageCount(std::ostream& output) const override;
            virtual void addNewDiscussionTag(const std::string& name, std::ostream& output) override;
            virtual void changeDiscussionTagName(const Entities::IdType& id, const std::string& newName,
                                                 std::ostream& output) override;
            virtual void changeDiscussionTagUiBlob(const Entities::IdType& id, const std::string& blob,
                                                   std::ostream& output) override;
            virtual void deleteDiscussionTag(const Entities::IdType& id, std::ostream& output) override;

            virtual void getDiscussionThreadsWithTagByName(const Entities::IdType& id, 
                                                           std::ostream& output) const override;
            virtual void getDiscussionThreadsWithTagByCreated(const Entities::IdType& id,
                                                              std::ostream& output) const override;
            virtual void getDiscussionThreadsWithTagByLastUpdated(const Entities::IdType& id,
                                                                  std::ostream& output) const override;
            virtual void getDiscussionThreadsWithTagByMessageCount(const Entities::IdType& id,
                                                                   std::ostream& output) const override;
            virtual void addDiscussionTagToThread(const Entities::IdType& tagId, const Entities::IdType& threadId, 
                                                  std::ostream& output) override;
            virtual void removeDiscussionTagFromThread(const Entities::IdType& tagId, const Entities::IdType& threadId, 
                                                       std::ostream& output) override;
            virtual void mergeDiscussionTags(const Entities::IdType& fromId, const Entities::IdType& intoId,
                                             std::ostream& output) override;

        private:
            void voteDiscussionThreadMessage(const Entities::IdType& id, std::ostream& output, bool up);

            friend struct PerformedByWithLastSeenUpdateGuard;

            PerformedByWithLastSeenUpdateGuard preparePerformedBy() const
            {
                return PerformedByWithLastSeenUpdateGuard(*this);
            }

            Helpers::ResourceGuard<Entities::EntityCollection> collection_;
            ReadEvents readEvents_;
            WriteEvents writeEvents_;

            boost::u32regex validUserNameRegex;
            boost::u32regex validDiscussionThreadNameRegex;
            boost::u32regex validDiscussionMessageContentRegex;
            boost::u32regex validDiscussionMessageChangeReasonRegex;
            boost::u32regex validDiscussionTagNameRegex;
        };

        inline ObserverContext_ createObserverContext(PerformedByType performedBy)
        {
            return ObserverContext_(performedBy, Context::getCurrentTime(), Context::getDisplayContext());
        }
    }
}
