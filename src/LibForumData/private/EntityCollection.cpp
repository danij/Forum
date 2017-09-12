#include "EntityCollection.h"
#include "VectorWithFreeQueue.h"

#include "StateHelpers.h"
#include "ContextProviders.h"

#include <future>

using namespace Forum::Authorization;
using namespace Forum::Entities;
using namespace Forum::Helpers;

/**
 * Used to prevent the individual removal of threads from a user's created threads collection when deleting a user
 */
static thread_local bool alsoDeleteThreadsFromUser = true;
/**
 * Used to prevent the individual removal of message from a user's created messages collection when deleting a user
 */
static thread_local bool alsoDeleteMessagesFromUser = true;

struct EntityCollection::Impl
{
    struct ManagedEntities
    {
        VectorWithFreeQueue<User> users;
        VectorWithFreeQueue<DiscussionThread> threads;
        VectorWithFreeQueue<DiscussionThreadMessage> threadMessages;
        VectorWithFreeQueue<DiscussionTag> tags;
        VectorWithFreeQueue<DiscussionCategory> categories;
        VectorWithFreeQueue<MessageComment> messageComments;

    } managedEntities;

    UserCollection users_;
    DiscussionThreadCollectionWithHashedId threads_;
    DiscussionThreadMessageCollection threadMessages_;
    DiscussionTagCollection tags_;
    DiscussionCategoryCollection categories_;
    MessageCommentCollection messageComments_;

    GrantedPrivilegeStore grantedPrivileges_;

    bool batchInsertInProgress_{ false };

    void insertUser(UserPtr user)
    {
        assert(user);
        users_.add(user);
    }

    void deleteUser(UserPtr user)
    {
        assert(user);
        if ( ! users_.remove(user))
        {
            return;
        }

        for (DiscussionThreadMessagePtr message : user->votedMessages())
        {
            assert(message);
            message->removeVote(user);
        }

        for (MessageCommentPtr comment : user->messageComments().byId())
        {
            assert(comment);
            if (comment->solved())
            {
                comment->parentMessage().solvedCommentsCount() -= 1;
            }
            comment->parentMessage().removeComment(comment);
        }

        for (DiscussionThreadPtr thread : user->subscribedThreads().byId())
        {
            assert(thread);
            thread->subscribedUsers().erase(user);
        }

        {
            //no need to delete the message from the user as we're deleting the whole user anyway
            BoolTemporaryChanger changer(alsoDeleteMessagesFromUser, false);
            for (auto& message : user->threadMessages().byId())
            {
                assert(message);
                //Each discussion message holds a reference to the user that created it and the parent thread
                //As such, delete the discussion message before deleting the thread and the user
                deleteDiscussionThreadMessage(message);
            }
        }
        {
            //no need to delete the thread from the user as we're deleting the whole user anyway
            BoolTemporaryChanger changer(alsoDeleteThreadsFromUser, false);
            for (auto& thread : user->threads().byId())
            {
                assert(thread);
                //Each discussion thread holds a reference to the user that created it
                //As such, delete the discussion thread before deleting the user
                deleteDiscussionThread(thread);
            }
        }
    }

    void insertDiscussionThread(DiscussionThreadPtr thread)
    {
        assert(thread);
        threads_.add(thread);
    }

    void deleteDiscussionThread(DiscussionThreadPtr threadPtr)
    {
        assert(threadPtr);

        if ( ! threads_.remove(threadPtr))
        {
            return;
        }

        DiscussionThread& thread = *threadPtr;

        thread.aboutToBeDeleted() = true;

        for (auto& message : thread.messages().byId())
        {
            assert(message);
            //Each discussion message holds a reference to the user that created it and the parent thread
            //As such, delete the discussion message before deleting the thread
            deleteDiscussionThreadMessage(message);
        }

        if (alsoDeleteThreadsFromUser)
        {
            thread.createdBy().threads().remove(threadPtr);
        }

        for (DiscussionCategoryPtr category : thread.categories())
        {
            assert(category);
            category->deleteDiscussionThread(threadPtr);
        }

        for (DiscussionTagPtr tag : thread.tags())
        {
            assert(tag);
            tag->deleteDiscussionThread(threadPtr);
        }

        for (UserPtr user : thread.subscribedUsers())
        {
            assert(user);
            user->subscribedThreads().remove(threadPtr);
        }
    }

    void insertDiscussionThreadMessage(DiscussionThreadMessagePtr message)
    {
        assert(message);
        threadMessages_.add(message);
    }

    void deleteDiscussionThreadMessage(DiscussionThreadMessagePtr message)
    {
        assert(message);
        if ( ! threadMessages_.remove(message))
        {
            return;
        }

        if (alsoDeleteMessagesFromUser)
        {
            message->createdBy().threadMessages().remove(message);
        }

        auto messageComments = message->comments();
        if (messageComments)
        {
            for (MessageCommentPtr comment : messageComments->byId())
            {
                deleteMessageComment(comment);
            }
        }

        DiscussionThread& parentThread = *(message->parentThread());
        if (parentThread.aboutToBeDeleted())
        {
            return;
        }

        parentThread.deleteDiscussionThreadMessage(message);
        parentThread.resetVisitorsSinceLastEdit();
        parentThread.latestVisibleChange() = Context::getCurrentTime();

        for (DiscussionTagPtr tag : parentThread.tags())
        {
            assert(tag);
            tag->updateMessageCount(-1);
        }

        for (DiscussionCategoryPtr category : parentThread.categories())
        {
            assert(category);
            category->updateMessageCount(message->parentThread(), -1);
        }
    }

    void insertDiscussionTag(DiscussionTagPtr tag)
    {
        assert(tag);
        tags_.add(tag);
    }

    void deleteDiscussionTag(DiscussionTagPtr tag)
    {
        assert(tag);

        if ( ! tags_.remove (tag))
        {
            return;
        }

        for (DiscussionCategoryPtr category : tag->categories())
        {
            assert(category);
            category->removeTag(tag);
        }
        for (DiscussionThreadPtr thread : tag->threads().byId())
        {
            assert(thread);
            thread->removeTag(tag);
        }
    }

    void insertDiscussionCategory(DiscussionCategoryPtr category)
    {
        assert(category);
        categories_.add(category);
    }

    void deleteDiscussionCategory(DiscussionCategoryPtr category)
    {
        assert(category);

        if ( ! categories_.remove(category))
        {
            return;
        }
        for (DiscussionTagPtr tag : category->tags())
        {
            assert(tag);
            tag->removeCategory(category);
        }
    }

    void insertMessageComment(MessageCommentPtr comment)
    {
        assert(comment);
        messageComments_.add(comment);
    }

    void deleteMessageComment(MessageCommentPtr comment)
    {
        assert(comment);
        messageComments_.remove(comment);
    }

    void setEventListeners()
    {
        User::changeNotifications().onPrepareUpdateAuth         = [this](auto& user) { this->onPrepareUpdateUserAuth(user); };
        User::changeNotifications().onPrepareUpdateAuth         = [this](auto& user) { this->onPrepareUpdateUserAuth(user); };
        User::changeNotifications().onPrepareUpdateName         = [this](auto& user) { this->onPrepareUpdateUserName(user); };
        User::changeNotifications().onPrepareUpdateLastSeen     = [this](auto& user) { this->onPrepareUpdateUserLastSeen(user); };
        User::changeNotifications().onPrepareUpdateThreadCount  = [this](auto& user) { this->onPrepareUpdateUserThreadCount(user); };
        User::changeNotifications().onPrepareUpdateMessageCount = [this](auto& user) { this->onPrepareUpdateUserMessageCount(user); };

        User::changeNotifications().onUpdateAuth         = [this](auto& user) { this->onUpdateUserAuth(user); };
        User::changeNotifications().onUpdateAuth         = [this](auto& user) { this->onUpdateUserAuth(user); };
        User::changeNotifications().onUpdateName         = [this](auto& user) { this->onUpdateUserName(user); };
        User::changeNotifications().onUpdateLastSeen     = [this](auto& user) { this->onUpdateUserLastSeen(user); };
        User::changeNotifications().onUpdateThreadCount  = [this](auto& user) { this->onUpdateUserThreadCount(user); };
        User::changeNotifications().onUpdateMessageCount = [this](auto& user) { this->onUpdateUserMessageCount(user); };

        DiscussionThread::changeNotifications().onPrepareUpdateName                 = [this](auto& thread) { this->onPrepareUpdateDiscussionThreadName(thread); };
        DiscussionThread::changeNotifications().onPrepareUpdateLastUpdated          = [this](auto& thread) { this->onPrepareUpdateDiscussionThreadLastUpdated(thread); };
        DiscussionThread::changeNotifications().onPrepareUpdateLatestMessageCreated = [this](auto& thread) { this->onPrepareUpdateDiscussionThreadLatestMessageCreated(thread); };
        DiscussionThread::changeNotifications().onPrepareUpdateMessageCount         = [this](auto& thread) { this->onPrepareUpdateDiscussionThreadMessageCount(thread); };
        DiscussionThread::changeNotifications().onPrepareUpdatePinDisplayOrder      = [this](auto& thread) { this->onPrepareUpdateDiscussionThreadPinDisplayOrder(thread); };

        DiscussionThread::changeNotifications().onUpdateName                 = [this](auto& thread) { this->onUpdateDiscussionThreadName(thread); };
        DiscussionThread::changeNotifications().onUpdateLastUpdated          = [this](auto& thread) { this->onUpdateDiscussionThreadLastUpdated(thread); };
        DiscussionThread::changeNotifications().onUpdateLatestMessageCreated = [this](auto& thread) { this->onUpdateDiscussionThreadLatestMessageCreated(thread); };
        DiscussionThread::changeNotifications().onUpdateMessageCount         = [this](auto& thread) { this->onUpdateDiscussionThreadMessageCount(thread); };
        DiscussionThread::changeNotifications().onUpdatePinDisplayOrder      = [this](auto& thread) { this->onUpdateDiscussionThreadPinDisplayOrder(thread); };

        DiscussionTag::changeNotifications().onPrepareUpdateName         = [this](auto& tag) { this->onPrepareUpdateDiscussionTagName(tag); };
        DiscussionTag::changeNotifications().onPrepareUpdateThreadCount  = [this](auto& tag) { this->onPrepareUpdateDiscussionTagThreadCount(tag); };
        DiscussionTag::changeNotifications().onPrepareUpdateMessageCount = [this](auto& tag) { this->onPrepareUpdateDiscussionTagMessageCount(tag); };

        DiscussionTag::changeNotifications().onUpdateName         = [this](auto& tag) { this->onUpdateDiscussionTagName(tag); };
        DiscussionTag::changeNotifications().onUpdateThreadCount  = [this](auto& tag) { this->onUpdateDiscussionTagThreadCount(tag); };
        DiscussionTag::changeNotifications().onUpdateMessageCount = [this](auto& tag) { this->onUpdateDiscussionTagMessageCount(tag); };

        DiscussionCategory::changeNotifications().onPrepareUpdateName         = [this](auto& category) { this->onPrepareUpdateDiscussionCategoryName(category); };
        DiscussionCategory::changeNotifications().onPrepareUpdateMessageCount = [this](auto& category) { this->onPrepareUpdateDiscussionCategoryMessageCount(category); };
        DiscussionCategory::changeNotifications().onPrepareUpdateDisplayOrder = [this](auto& category) { this->onPrepareUpdateDiscussionCategoryDisplayOrder(category); };

        DiscussionCategory::changeNotifications().onUpdateName         = [this](auto& category) { this->onUpdateDiscussionCategoryName(category); };
        DiscussionCategory::changeNotifications().onUpdateMessageCount = [this](auto& category) { this->onUpdateDiscussionCategoryMessageCount(category); };
        DiscussionCategory::changeNotifications().onUpdateDisplayOrder = [this](auto& category) { this->onUpdateDiscussionCategoryDisplayOrder(category); };
    }

    void onPrepareUpdateUserAuth        (const User& user) { users_.prepareUpdateAuth(user.pointer()); }
    void onPrepareUpdateUserName        (const User& user) { users_.prepareUpdateName(user.pointer()); }
    void onPrepareUpdateUserLastSeen    (const User& user) { if ( ! batchInsertInProgress_) users_.prepareUpdateLastSeen(user.pointer()); }
    void onPrepareUpdateUserThreadCount (const User& user) { if ( ! batchInsertInProgress_) users_.prepareUpdateThreadCount(user.pointer()); }
    void onPrepareUpdateUserMessageCount(const User& user) { if ( ! batchInsertInProgress_) users_.prepareUpdateMessageCount(user.pointer()); }

    void onUpdateUserAuth        (const User& user) { users_.updateAuth(user.pointer()); }
    void onUpdateUserName        (const User& user) { users_.updateName(user.pointer()); }
    void onUpdateUserLastSeen    (const User& user) { if ( ! batchInsertInProgress_) users_.updateLastSeen(user.pointer()); }
    void onUpdateUserThreadCount (const User& user) { if ( ! batchInsertInProgress_) users_.updateThreadCount(user.pointer()); }
    void onUpdateUserMessageCount(const User& user) { if ( ! batchInsertInProgress_) users_.updateMessageCount(user.pointer()); }

    void discussionThreadAction(const DiscussionThread& constThread,
                                void (DiscussionThreadCollectionBase::*fn)(DiscussionThreadPtr))
    {
        auto& thread = const_cast<DiscussionThread&>(constThread);
        DiscussionThreadPtr threadPtr = thread.pointer();

        (threads_.*fn)(threadPtr);
        (thread.createdBy().threads().*fn)(threadPtr);

        for (UserPtr user : thread.subscribedUsers())
        {
            assert(user);
            (user->subscribedThreads().*fn)(threadPtr);
        }

        for (DiscussionTagPtr tag : thread.tags())
        {
            assert(tag);
            (tag->threads().*fn)(threadPtr);
        }

        for (DiscussionCategoryPtr category : thread.categories())
        {
            assert(category);
            (category->threads().*fn)(threadPtr);
        }
    }

    void onPrepareUpdateDiscussionThreadName(const DiscussionThread& thread)                 { discussionThreadAction(thread, &DiscussionThreadCollectionBase::prepareUpdateName); }
    void onPrepareUpdateDiscussionThreadLastUpdated(const DiscussionThread& thread)          { if ( ! batchInsertInProgress_) discussionThreadAction(thread, &DiscussionThreadCollectionBase::prepareUpdateLastUpdated); }
    void onPrepareUpdateDiscussionThreadLatestMessageCreated(const DiscussionThread& thread) { if ( ! batchInsertInProgress_) discussionThreadAction(thread, &DiscussionThreadCollectionBase::prepareUpdateLatestMessageCreated); }
    void onPrepareUpdateDiscussionThreadMessageCount(const DiscussionThread& thread)         { if ( ! batchInsertInProgress_) discussionThreadAction(thread, &DiscussionThreadCollectionBase::prepareUpdateMessageCount); }
    void onPrepareUpdateDiscussionThreadPinDisplayOrder(const DiscussionThread& thread)      { if ( ! batchInsertInProgress_) discussionThreadAction(thread, &DiscussionThreadCollectionBase::prepareUpdatePinDisplayOrder); }

    void onUpdateDiscussionThreadName(const DiscussionThread& thread)                 { discussionThreadAction(thread, &DiscussionThreadCollectionBase::updateName); }
    void onUpdateDiscussionThreadLastUpdated(const DiscussionThread& thread)          { if ( ! batchInsertInProgress_) discussionThreadAction(thread, &DiscussionThreadCollectionBase::updateLastUpdated); }
    void onUpdateDiscussionThreadLatestMessageCreated(const DiscussionThread& thread) { if ( ! batchInsertInProgress_) discussionThreadAction(thread, &DiscussionThreadCollectionBase::updateLatestMessageCreated); }
    void onUpdateDiscussionThreadMessageCount(const DiscussionThread& thread)         { if ( ! batchInsertInProgress_) discussionThreadAction(thread, &DiscussionThreadCollectionBase::updateMessageCount); }
    void onUpdateDiscussionThreadPinDisplayOrder(const DiscussionThread& thread)      { if ( ! batchInsertInProgress_) discussionThreadAction(thread, &DiscussionThreadCollectionBase::updatePinDisplayOrder); }

    void onPrepareUpdateDiscussionTagName        (const DiscussionTag& tag) { tags_.prepareUpdateName(tag.pointer()); }
    void onPrepareUpdateDiscussionTagThreadCount (const DiscussionTag& tag) { }
    void onPrepareUpdateDiscussionTagMessageCount(const DiscussionTag& tag) { if ( ! batchInsertInProgress_) tags_.prepareUpdateMessageCount(tag.pointer()); }

    void onUpdateDiscussionTagName        (const DiscussionTag& tag) { tags_.updateName(tag.pointer()); }
    void onUpdateDiscussionTagThreadCount (const DiscussionTag& tag) { /*TODO add retrieval of tags sorted by thread count*/ }
    void onUpdateDiscussionTagMessageCount(const DiscussionTag& tag) { if ( ! batchInsertInProgress_) tags_.updateMessageCount(tag.pointer()); }

    void onPrepareUpdateDiscussionCategoryName        (const DiscussionCategory& category) { categories_.prepareUpdateName(category.pointer()); }
    void onPrepareUpdateDiscussionCategoryMessageCount(const DiscussionCategory& category) { if ( ! batchInsertInProgress_) categories_.prepareUpdateMessageCount(category.pointer()); }
    void onPrepareUpdateDiscussionCategoryDisplayOrder(const DiscussionCategory& category) { if ( ! batchInsertInProgress_) categories_.prepareUpdateDisplayOrderRootPriority(category.pointer()); }

    void onUpdateDiscussionCategoryName        (const DiscussionCategory& category) { categories_.updateName(category.pointer()); }
    void onUpdateDiscussionCategoryMessageCount(const DiscussionCategory& category) { if ( ! batchInsertInProgress_) categories_.updateMessageCount(category.pointer()); }
    void onUpdateDiscussionCategoryDisplayOrder(const DiscussionCategory& category) { if ( ! batchInsertInProgress_) categories_.updateDisplayOrderRootPriority(category.pointer()); }

    void toggleBatchInsert(bool activate)
    {
        if ( ! activate) assert(batchInsertInProgress_);

        if (activate)
        {
            Context::setBatchInsertInProgres(batchInsertInProgress_ = activate);
            return;
        }

        users_.stopBatchInsert();

        std::future<void> futures[] =
        {
            std::async(std::launch::async, [this]()
            {
                for (UserPtr user : this->users_.byId())
                {
                    user->threadMessages().stopBatchInsert();
                }
            }),

            std::async(std::launch::async, [this]()
            {
                for (DiscussionThreadPtr thread : this->threads_.byId())
                {
                    thread->messages().stopBatchInsert();
                }
            }),

            std::async(std::launch::async, [this]()
            {
                for (UserPtr user : this->users_.byId())
                {
                    user->threads().stopBatchInsert();
                }
            }),

            std::async(std::launch::async, [this]()
            {
                for (UserPtr user : this->users_.byId())
                {
                    user->subscribedThreads().stopBatchInsert();
                }
            }),

            std::async(std::launch::async, [this]()
            {
                for (DiscussionTagPtr tag : this->tags_.byId())
                {
                    tag->threads().stopBatchInsert();
                }
            }),

            std::async(std::launch::async, [this]()
            {
                for (DiscussionCategoryPtr category : this->categories_.byId())
                {
                    category->stopBatchInsert();
                }
            }),

            std::async(std::launch::async, [this]()
            {
                this->threads_.stopBatchInsert();
                this->threadMessages_.stopBatchInsert();
                this->tags_.stopBatchInsert();
                this->categories_.stopBatchInsert();
            })
        };

        for (auto& future : futures)
        {
            future.get();
        }

        Context::setBatchInsertInProgres(batchInsertInProgress_ = activate);
    }
};

static UserPtr anonymousUser_;
static IdType anonymousUserId_;

UserPtr Forum::Entities::anonymousUser()
{
    return anonymousUser_;
}

IdType Forum::Entities::anonymousUserId()
{
    return anonymousUserId_;
}

EntityCollection::EntityCollection()
{
    impl_ = new Impl();

    Private::setGlobalEntityCollection(this);
    impl_->setEventListeners();

    anonymousUser_ = UserPtr(static_cast<UserPtr::IndexType>(impl_->managedEntities.users.add("<anonymous>")));
    anonymousUserId_ = anonymousUser_->id();
}

EntityCollection::~EntityCollection()
{
    Private::setGlobalEntityCollection(nullptr);

    if (impl_) delete impl_;
}

const GrantedPrivilegeStore& EntityCollection::grantedPrivileges() const
{
    return impl_->grantedPrivileges_;
}

GrantedPrivilegeStore& EntityCollection::grantedPrivileges()
{
    return impl_->grantedPrivileges_;
}

std::unique_ptr<User>* EntityCollection::getUserPoolRoot()
{
    return impl_->managedEntities.users.data();
}

std::unique_ptr<DiscussionThread>* EntityCollection::getDiscussionThreadPoolRoot()
{
    return impl_->managedEntities.threads.data();
}

std::unique_ptr<DiscussionThreadMessage>* EntityCollection::getDiscussionThreadMessagePoolRoot()
{
    return impl_->managedEntities.threadMessages.data();
}

std::unique_ptr<DiscussionTag>* EntityCollection::getDiscussionTagPoolRoot()
{
    return impl_->managedEntities.tags.data();
}

std::unique_ptr<DiscussionCategory>* EntityCollection::getDiscussionCategoryPoolRoot()
{
    return impl_->managedEntities.categories.data();
}

std::unique_ptr<MessageComment>* EntityCollection::getMessageCommentPoolRoot()
{
    return impl_->managedEntities.messageComments.data();
}

UserPtr EntityCollection::createUser(IdType id, User::NameType&& name, Timestamp created, VisitDetails creationDetails)
{
    auto result = UserPtr(static_cast<UserPtr::IndexType>(
        impl_->managedEntities.users.add(id, std::move(name), created, std::move(creationDetails))));
    result->pointer_ = result;
    return result;
}

DiscussionThreadPtr EntityCollection::createDiscussionThread(IdType id, User& createdBy, DiscussionThread::NameType&& name,
                                                             Timestamp created, VisitDetails creationDetails)
{
    auto result = DiscussionThreadPtr(static_cast<DiscussionThreadPtr::IndexType>(impl_->managedEntities.threads.add(
        id, createdBy, std::move(name), created, creationDetails)));
    result->pointer_ = result;
    return result;
}

DiscussionThreadMessagePtr EntityCollection::createDiscussionThreadMessage(IdType id, User& createdBy,
                                                                           Timestamp created, VisitDetails creationDetails)
{
    return DiscussionThreadMessagePtr(static_cast<DiscussionThreadMessagePtr::IndexType>(
        impl_->managedEntities.threadMessages.add(id, createdBy, created, creationDetails)));
}

DiscussionTagPtr EntityCollection::createDiscussionTag(IdType id, DiscussionTag::NameType&& name, Timestamp created,
                                                       VisitDetails creationDetails)
{
    auto result = DiscussionTagPtr(static_cast<DiscussionTagPtr::IndexType>(impl_->managedEntities.tags.add(
        id, std::move(name), created, creationDetails, *this)));
    result->pointer_ = result;
    return result;
}

DiscussionCategoryPtr EntityCollection::createDiscussionCategory(IdType id, DiscussionCategory::NameType&& name,
                                                                 Timestamp created, VisitDetails creationDetails)
{
    auto result = DiscussionCategoryPtr(static_cast<DiscussionCategoryPtr::IndexType>(impl_->managedEntities.categories.add(
        id, std::move(name), created, creationDetails, *this)));
    result->pointer_ = result;
    return result;
}

MessageCommentPtr EntityCollection::createMessageComment(IdType id, DiscussionThreadMessage& message, User& createdBy,
                                                         Timestamp created, VisitDetails creationDetails)
{
    return MessageCommentPtr(static_cast<MessageCommentPtr::IndexType>(impl_->managedEntities.messageComments.add(
        id, message, createdBy, created, creationDetails)));
}


const UserCollection& EntityCollection::users() const
{
    return impl_->users_;
}

UserCollection& EntityCollection::users()
{
    return impl_->users_;
}

const DiscussionThreadCollectionWithHashedId& EntityCollection::threads() const
{
    return impl_->threads_;
}

DiscussionThreadCollectionWithHashedId& EntityCollection::threads()
{
    return impl_->threads_;
}

const DiscussionThreadMessageCollection& EntityCollection::threadMessages() const
{
    return impl_->threadMessages_;
}

DiscussionThreadMessageCollection& EntityCollection::threadMessages()
{
    return impl_->threadMessages_;
}

const DiscussionTagCollection& EntityCollection::tags() const
{
    return impl_->tags_;
}

DiscussionTagCollection& EntityCollection::tags()
{
    return impl_->tags_;
}

const DiscussionCategoryCollection& EntityCollection::categories() const
{
    return impl_->categories_;
}

DiscussionCategoryCollection& EntityCollection::categories()
{
    return impl_->categories_;
}

const MessageCommentCollection& EntityCollection::messageComments() const
{
    return impl_->messageComments_;
}

MessageCommentCollection& EntityCollection::messageComments()
{
    return impl_->messageComments_;
}

void EntityCollection::insertUser(UserPtr user)
{
    impl_->insertUser(user);
}

void EntityCollection::deleteUser(UserPtr user)
{
    impl_->deleteUser(user);
}

void EntityCollection::insertDiscussionThread(DiscussionThreadPtr thread)
{
    impl_->insertDiscussionThread(thread);
}

void EntityCollection::deleteDiscussionThread(DiscussionThreadPtr thread)
{
    impl_->deleteDiscussionThread(thread);
}

void EntityCollection::insertDiscussionThreadMessage(DiscussionThreadMessagePtr message)
{
    impl_->insertDiscussionThreadMessage(message);
}

void EntityCollection::deleteDiscussionThreadMessage(DiscussionThreadMessagePtr message)
{
    impl_->deleteDiscussionThreadMessage(message);
}

void EntityCollection::insertDiscussionTag(DiscussionTagPtr tag)
{
    impl_->insertDiscussionTag(tag);
}

void EntityCollection::deleteDiscussionTag(DiscussionTagPtr tag)
{
    impl_->deleteDiscussionTag(tag);
}

void EntityCollection::insertDiscussionCategory(DiscussionCategoryPtr category)
{
    impl_->insertDiscussionCategory(category);
}

void EntityCollection::deleteDiscussionCategory(DiscussionCategoryPtr category)
{
    impl_->deleteDiscussionCategory(category);
}

void EntityCollection::insertMessageComment(MessageCommentPtr comment)
{
    impl_->insertMessageComment(comment);
}

void EntityCollection::deleteMessageComment(MessageCommentPtr comment)
{
    impl_->deleteMessageComment(comment);
}

void EntityCollection::startBatchInsert()
{
    impl_->toggleBatchInsert(true);
}

void EntityCollection::stopBatchInsert()
{
    impl_->toggleBatchInsert(false);
}
