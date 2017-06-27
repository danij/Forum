#include "EntityCollection.h"
#include "VectorWithFreeQueue.h"

#include "StateHelpers.h"
#include "ContextProviders.h"

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

UserRef EntityCollection::deleteUser(UserIdIteratorType iterator)
{
    UserRef user;
    if ( ! ((user = UserCollectionBase::deleteUser(iterator))))
    {
        return user;
    }
    //delete all votes of this user
    {
        for (auto& messageWeak : user->votedMessages())
        {
            if (auto message = messageWeak.lock())
            {
                message->removeVote(user);
            }
        }
    }
    //delete all comments of this user
    {
        for (auto& commentRef : user->messageComments())
        {
            if (commentRef)
            {
                commentRef->executeActionWithParentMessageIfAvailable([&](DiscussionThreadMessage& message)
                {
                    if (commentRef->solved())
                    {
                        message.solvedCommentsCount() -= 1;
                    }
                    message.deleteMessageCommentById(commentRef->id());
                });
            }
        }
    }
    //delete all subscriptions of this user
    {
        for (auto& threadRef : user->subscribedThreads().threads())
        {
            threadRef->subscribedUsers().erase(user);
        }
    }
    {
        //no need to delete the message from the user as we're deleting the whole user anyway
        BoolTemporaryChanger changer(alsoDeleteMessagesFromUser, false);
        for (auto& message : user->messages())
        {
            //Each discussion message holds a reference to the user that created it and the parent thread
            //As such, delete the discussion message before deleting the thread and the user
            deleteDiscussionThreadMessageById(message->id());
        }
    }
    {
        //no need to delete the thread from the user as we're deleting the whole user anyway
        BoolTemporaryChanger changer(alsoDeleteThreadsFromUser, false);
        for (auto& thread : user->threads())
        {
            //Each discussion thread holds a reference to the user that created it
            //As such, delete the discussion thread before deleting the user
            deleteDiscussionThreadById(thread->id());
        }
    }
    return user;
}

//
//
//Discussion Threads
//
//

void EntityCollection::modifyDiscussionThread(ThreadIdIteratorType iterator,
                                              std::function<void(DiscussionThread&)>&& modifyFunction)
{
    if (iterator == threads_.end())
    {
        return;
    }
    //allow reindexing of the collection that includes all threads
    threads_.modify(iterator, [&modifyFunction](const DiscussionThreadRef& thread)
    {
        if (thread)
        {
            //allow reindexing of the subcollection containing only the threads of the current user
            thread->createdBy().modifyDiscussionThreadById(thread->id(),
                                                           std::forward<std::function<void(DiscussionThread&)>>(modifyFunction));
            for (auto& userWeak : thread->subscribedUsers())
            {
                if (auto userShared = userWeak.lock())
                {
                    userShared->subscribedThreads().modifyDiscussionThreadById(thread->id(), {});
                }
            }
        }
    });
}

DiscussionThreadRef EntityCollection::deleteDiscussionThread(ThreadIdIteratorType iterator)
{
    DiscussionThreadRef thread;
    if ( ! ((thread = DiscussionThreadCollectionBase::deleteDiscussionThread(iterator))))
    {
        return thread;
    }
    thread->aboutToBeDeleted() = true;
    {
        for (auto& message : thread->messages())
        {
            //Each discussion message holds a reference to the user that created it and the parent thread
            //As such, delete the discussion message before deleting the thread
            deleteDiscussionThreadMessageById(message->id());
        }
    }
    if (alsoDeleteThreadsFromUser)
    {
        modifyUserById(thread->createdBy().id(), [&](User& user)
        {
            user.deleteDiscussionThreadById(thread->id());
        });
    }
    for (auto& categoryWeak : thread->categoriesWeak())
    {
        if (auto categoryShared = categoryWeak.lock())
        {
            categoryShared->deleteDiscussionThreadById(thread->id());
        }
    }
    for (auto& tagWeak : thread->tagsWeak())
    {
        if (auto tagShared = tagWeak.lock())
        {
            tagShared->deleteDiscussionThreadById(thread->id());
        }
    }
    for (auto& userWeak : thread->subscribedUsers())
    {
        if (auto userShared = userWeak.lock())
        {
            userShared->subscribedThreads().deleteDiscussionThreadById(thread->id());
        }
    }
    return thread;
}

//
//
//Discussion Messages
//
//

void EntityCollection::modifyDiscussionThreadMessage(MessageIdIteratorType iterator,
                                                     std::function<void(DiscussionThreadMessage&)>&& modifyFunction)
{
    if (iterator == messages_.end())
    {
        return;
    }
    //allow reindexing of the collection that includes all messages
    messages_.modify(iterator, [&modifyFunction](const DiscussionThreadMessageRef& message)
    {
        if (message)
        {
            //allow reindexing of the subcollection containing only the messages of the current user
            message->createdBy().modifyDiscussionThreadMessageById(message->id(), [&modifyFunction](auto& messageToModify)
            {
                messageToModify.executeActionWithParentThreadIfAvailable([&](auto& parentThread)
                {
                    parentThread.modifyDiscussionThreadMessageById(messageToModify.id(),
                                                                   std::forward<std::function<void(DiscussionThreadMessage&)>>(modifyFunction));
                });
            });
        }
    });
}

DiscussionThreadMessageRef EntityCollection::deleteDiscussionThreadMessage(MessageIdIteratorType iterator)
{
    DiscussionThreadMessageRef message;
    if ( ! ((message = DiscussionThreadMessageCollectionBase::deleteDiscussionThreadMessage(iterator))))
    {
        return message;
    }
    if (alsoDeleteMessagesFromUser)
    {
        modifyUserById(message->createdBy().id(), [&](User& user)
        {
            user.deleteDiscussionThreadMessageById(message->id());
        });
    }

    message->executeActionWithParentThreadIfAvailable([&](auto& parentThread)
    {
        if ( ! parentThread.aboutToBeDeleted())
        {
            this->modifyDiscussionThreadById(parentThread.id(), [&](DiscussionThread& thread)
            {
                thread.deleteDiscussionThreadMessageById(message->id());
                thread.resetVisitorsSinceLastEdit();
                thread.latestVisibleChange() = Context::getCurrentTime();

                for (auto& tagWeak : thread.tagsWeak())
                {
                    if (auto tagShared = tagWeak.lock())
                    {
                        this->modifyDiscussionTagById(tagShared->id(), [&thread](auto& tag)
                        {
                            tag.messageCount() -= 1;
                            //notify the thread collection of each tag that the thread has fewer messages
                            tag.modifyDiscussionThreadById(thread.id(), {});
                        });
                    }
                }
                for (auto& categoryWeak : thread.categoriesWeak())
                {
                    if (auto categoryShared = categoryWeak.lock())
                    {
                        auto threadShared = thread.shared_from_this();
                        this->modifyDiscussionCategoryById(categoryShared->id(), [&thread, &threadShared](auto& category)
                        {
                            category.updateMessageCount(threadShared, -1);
                            //notify the thread collection of each category that the thread has fewer messages
                            category.modifyDiscussionThreadById(thread.id(), {});
                        });
                    }
                }
            });
        }
    });
    return message;
}

//
//
//Discussion Tags
//
//

DiscussionTagRef EntityCollection::deleteDiscussionTag(TagIdIteratorType iterator)
{
    DiscussionTagRef tag;
    if ( ! ((tag = DiscussionTagCollectionBase::deleteDiscussionTag(iterator))))
    {
        return tag;
    }
    tag->aboutToBeDeleted();
    for (auto& categoryWeak : tag->categoriesWeak())
    {
        if (auto category = categoryWeak.lock())
        {
            category->removeTag(tag);
        }
    }
    for (auto& thread : tag->threads().get<DiscussionThreadCollectionById>())
    {
        if (thread)
        {
            thread->removeTag(tag);
        }
    }
    return tag;
}

//
//
//Discussion Categories
//
//

DiscussionCategoryRef EntityCollection::deleteDiscussionCategory(CategoryIdIteratorType iterator)
{
    DiscussionCategoryRef category;
    if ( ! ((category = DiscussionCategoryCollectionBase::deleteDiscussionCategory(iterator))))
    {
        return category;
    }
    for (auto& tag : category->tags())
    {
        if (tag)
        {
            tag->removeCategory(category);
        }
    }
    return category;
}

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

    void setEventListeners()
    {
        User::changeNotifications().onUpdateAuth         = [this](auto& user) { this->onUpdateUserAuth(user); };
        User::changeNotifications().onUpdateName         = [this](auto& user) { this->onUpdateUserName(user); };
        User::changeNotifications().onUpdateLastSeen     = [this](auto& user) { this->onUpdateUserLastSeen(user); };
        User::changeNotifications().onUpdateThreadCount  = [this](auto& user) { this->onUpdateUserThreadCount(user); };
        User::changeNotifications().onUpdateMessageCount = [this](auto& user) { this->onUpdateUserMessageCount(user); };

        DiscussionThread::changeNotifications().onUpdateName                 = [this](auto& thread) { this->onUpdateDiscussionThreadName(thread); };
        DiscussionThread::changeNotifications().onUpdateLastUpdated          = [this](auto& thread) { this->onUpdateDiscussionThreadLastUpdated(thread); };
        DiscussionThread::changeNotifications().onUpdateLatestMessageCreated = [this](auto& thread) { this->onUpdateDiscussionThreadLatestMessageCreated(thread); };
        DiscussionThread::changeNotifications().onUpdateMessageCount         = [this](auto& thread) { this->onUpdateDiscussionThreadMessageCount(thread); };
        DiscussionThread::changeNotifications().onUpdatePinDisplayOrder      = [this](auto& thread) { this->onUpdateDiscussionThreadPinDisplayOrder(thread); };

        DiscussionTag::changeNotifications().onUpdateName         = [this](auto& tag) { this->onUpdateDiscussionTagName(tag); };
        DiscussionTag::changeNotifications().onUpdateThreadCount  = [this](auto& tag) { this->onUpdateDiscussionTagThreadCount(tag); };
        DiscussionTag::changeNotifications().onUpdateMessageCount = [this](auto& tag) { this->onUpdateDiscussionTagMessageCount(tag); };

        DiscussionCategory::changeNotifications().onUpdateName         = [this](auto& category) { this->onUpdateDiscussionCategoryName(category); };
        DiscussionCategory::changeNotifications().onUpdateMessageCount = [this](auto& category) { this->onUpdateDiscussionCategoryMessageCount(category); };
        DiscussionCategory::changeNotifications().onUpdateDisplayOrder = [this](auto& category) { this->onUpdateDiscussionCategoryDisplayOrder(category); };
    }

    void onUpdateUserAuth(const User& user)
    {
        
    }

    void onUpdateUserName(const User& user)
    {
        
    }

    void onUpdateUserLastSeen(const User& user)
    {
        
    }

    void onUpdateUserThreadCount(const User& user)
    {
        
    }

    void onUpdateUserMessageCount(const User& user)
    {
        
    }

    void onUpdateDiscussionThreadName(const DiscussionThread& thread)
    {
        
    }

    void onUpdateDiscussionThreadLastUpdated(const DiscussionThread& thread)
    {

    }

    void onUpdateDiscussionThreadLatestMessageCreated(const DiscussionThread& thread)
    {

    }

    void onUpdateDiscussionThreadMessageCount(const DiscussionThread& thread)
    {

    }

    void onUpdateDiscussionThreadPinDisplayOrder(const DiscussionThread& thread)
    {

    }

    void onUpdateDiscussionTagName(const DiscussionTag& tag)
    {

    }

    void onUpdateDiscussionTagThreadCount(const DiscussionTag& tag)
    {

    }

    void onUpdateDiscussionTagMessageCount(const DiscussionTag& tag)
    {

    }

    void onUpdateDiscussionCategoryName(const DiscussionCategory& category)
    {

    }

    void onUpdateDiscussionCategoryMessageCount(const DiscussionTag& category)
    {

    }

    void onUpdateDiscussionCategoryDisplayOrder(const DiscussionTag& category)
    {

    }
};

EntityCollection::EntityCollection()
{
    impl_ = new Impl();

    Private::setGlobalEntityCollection(this);
    impl_->setEventListeners();
    
    notifyTagChange_ = [this](auto& tag)
    {
        this->modifyDiscussionTagById(tag.id(), [](auto&) {});
    };
    notifyCategoryChange_ = [this](auto& category)
    {
        this->modifyDiscussionCategoryById(category.id(), [](auto&) {});
    };
}

EntityCollection::~EntityCollection()
{
    Private::setGlobalEntityCollection(nullptr);

    if (impl_) delete impl_;
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

UserPtr EntityCollection::createAndAddUser(IdType id, Timestamp created)
{
    return UserPtr(static_cast<UserPtr::IndexType>(impl_->managedEntities.users.add(id, created)));
}

DiscussionThreadPtr EntityCollection::createAndAddDiscussionThread()
{
    return DiscussionThreadPtr(static_cast<DiscussionThreadPtr::IndexType>(impl_->managedEntities.threads.add()));
}

DiscussionThreadMessagePtr EntityCollection::createAndAddDiscussionThreadMessage()
{
    return DiscussionThreadMessagePtr(static_cast<DiscussionThreadMessagePtr::IndexType>(
        impl_->managedEntities.threadMessages.add()));
}

DiscussionTagPtr EntityCollection::createAndAddDiscussionTag()
{
    return DiscussionTagPtr(static_cast<DiscussionTagPtr::IndexType>(impl_->managedEntities.tags.add()));
}

DiscussionCategoryPtr EntityCollection::createAndAddDiscussionCategory()
{
    return DiscussionCategoryPtr(static_cast<DiscussionCategoryPtr::IndexType>(impl_->managedEntities.categories.add()));
}

const UserCollection& EntityCollection::users() const
{
    return impl_->users_;
}

const DiscussionThreadCollectionWithHashedId& EntityCollection::threads() const
{
    return impl_->threads_;
}

const DiscussionThreadMessageCollection& EntityCollection::threadMessages() const
{
    return impl_->threadMessages_;
}

const DiscussionTagCollection& EntityCollection::tags() const
{
    return impl_->tags_;
}

const DiscussionCategoryCollection& EntityCollection::categories() const
{
    return impl_->categories_;
}

const MessageCommentCollection& EntityCollection::messageComments() const
{
    return impl_->messageComments_;
}

const UserPtr Forum::Entities::AnonymousUser = std::make_shared<User>("<anonymous>");
const IdType Forum::Entities::AnonymousUserId = AnonymousUser->id();