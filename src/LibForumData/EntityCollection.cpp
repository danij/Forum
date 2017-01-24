#include "EntityCollection.h"

#include "StateHelpers.h"

using namespace Forum::Entities;
using namespace Forum::Helpers;

const UserRef Forum::Entities::AnonymousUser = std::make_shared<User>("<anonymous>");
const IdType Forum::Entities::AnonymousUserId = AnonymousUser->id();

void UserCollectionBase::modifyUser(UserCollection::iterator iterator,
                                    const std::function<void(User&)>& modifyFunction)
{
    if (iterator == users_.end())
    {
        return;
    }
    users_.modify(iterator, [&modifyFunction](const UserRef& user)
    {
        if (user)
        {
            modifyFunction(*user);
        }
    });
}

void UserCollectionBase::modifyUserById(const IdType& id, const std::function<void(User&)>& modifyFunction)
{
    return modifyUser(users_.get<UserCollectionById>().find(id), modifyFunction);
}

/**
 * Used to prevent the individual removal of threads from a user's created threads collection when deleting a user
 */
static thread_local bool alsoDeleteThreadsFromUser = true;
/**
 * Used to prevent the individual removal of message from a user's created messages collection when deleting a user
 */
static thread_local bool alsoDeleteMessagesFromUser = true;
/**
 * Used to prevent the individual removal of message from a thread's created messages collection when deleting a thread
 */
static thread_local bool alsoDeleteMessagesFromThread = true;

void UserCollectionBase::deleteUser(UserCollection::iterator iterator)
{
    if (iterator == users_.end())
    {
        return;
    }
    users_.erase(iterator);
}

void EntityCollection::deleteUser(UserCollection::iterator iterator)
{
    if (iterator == users_.end())
    {
        return;
    }
    {
        //no need to delete the message from the user as we're deleting the whole user anyway
        BoolTemporaryChanger changer(alsoDeleteMessagesFromUser, false);
        for (auto& message : (*iterator)->messages())
        {
            //Each discussion message holds a reference to the user that created it and the parent thread
            //As such, delete the discussion message before deleting the thread and the user
            deleteDiscussionThreadMessageById(message->id());
        }
    }
    {
        //no need to delete the thread from the user as we're deleting the whole user anyway
        BoolTemporaryChanger changer(alsoDeleteThreadsFromUser, false);
        for (auto& thread : (*iterator)->threads())
        {
            //Each discussion thread holds a reference to the user that created it
            //As such, delete the discussion thread before deleting the user
            deleteDiscussionThreadById(thread->id());
        }
    }
    users_.erase(iterator);
}

void UserCollectionBase::deleteUserById(const IdType& id)
{
    deleteUser(users_.get<UserCollectionById>().find(id));
}

//Discussion Threads

void DiscussionThreadCollectionBase::modifyDiscussionThread(DiscussionThreadCollection::iterator iterator,
                                                            const std::function<void(DiscussionThread&)>& modifyFunction)
{
    if (iterator == threads_.end())
    {
        return;
    }
    threads_.modify(iterator, [&modifyFunction](const DiscussionThreadRef& thread)
    {
        if (thread)
        {
            modifyFunction(*thread);
        }
    });
}

void EntityCollection::modifyDiscussionThread(DiscussionThreadCollection::iterator iterator,
                                              const std::function<void(DiscussionThread&)>& modifyFunction)
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
            thread->createdBy().modifyDiscussionThreadById(thread->id(), modifyFunction);
        }
    });
}

void DiscussionThreadCollectionBase::modifyDiscussionThreadById(const IdType& id,
                                                                const std::function<void(DiscussionThread&)>& modifyFunction)
{
    modifyDiscussionThread(threads_.get<DiscussionThreadCollectionById>().find(id), modifyFunction);
}

void DiscussionThreadCollectionBase::deleteDiscussionThread(DiscussionThreadCollection::iterator iterator)
{
    if (iterator == threads_.end())
    {
        return;
    }
    threads_.erase(iterator);
}

void EntityCollection::deleteDiscussionThread(DiscussionThreadCollection::iterator iterator)
{
    if (iterator == threads_.end())
    {
        return;
    }
    {
        //no need to delete the message from the thread as we're deleting the whole thread anyway
        BoolTemporaryChanger changer(alsoDeleteMessagesFromThread, false);
        for (auto& message : (*iterator)->messages())
        {
            //Each discussion message holds a reference to the user that created it and the parent thread
            //As such, delete the discussion message before deleting the thread
            deleteDiscussionThreadMessageById(message->id());
        }
    }
    if (alsoDeleteThreadsFromUser)
    {
        (*iterator)->createdBy().deleteDiscussionThreadById((*iterator)->id());
    }
    threads_.erase(iterator);
}

void DiscussionThreadCollectionBase::deleteDiscussionThreadById(const IdType& id)
{
    deleteDiscussionThread(threads_.get<DiscussionThreadCollectionById>().find(id));
}

//Discussion Messages

void DiscussionThreadMessageCollectionBase::modifyDiscussionThreadMessage(DiscussionThreadMessageCollection::iterator iterator,
                                                                          const std::function<void(DiscussionThreadMessage&)>& modifyFunction)
{
    if (iterator == messages_.end())
    {
        return;
    }
    messages_.modify(iterator, [&modifyFunction](const DiscussionMessageRef& message)
    {
        if (message)
        {
            modifyFunction(*message);
        }
    });
}

void EntityCollection::modifyDiscussionThreadMessage(DiscussionThreadMessageCollection::iterator iterator,
                                                     const std::function<void(DiscussionThreadMessage&)>& modifyFunction)
{
    if (iterator == messages_.end())
    {
        return;
    }
    //allow reindexing of the collection that includes all messages
    messages_.modify(iterator, [&modifyFunction](const DiscussionMessageRef& message)
    {
        if (message)
        {
            //allow reindexing of the subcollection containing only the messages of the current user
            message->createdBy().modifyDiscussionThreadMessageById(message->id(), [&modifyFunction](auto& messageToModify)
            {
                messageToModify.parentThread().modifyDiscussionThreadMessageById(messageToModify.id(), modifyFunction);
            });
        }
    });
}

void DiscussionThreadMessageCollectionBase::modifyDiscussionThreadMessageById(const IdType& id,
                                                                              const std::function<void(DiscussionThreadMessage&)>& modifyFunction)
{
    modifyDiscussionThreadMessage(messages_.get<DiscussionThreadMessageCollectionById>().find(id), modifyFunction);
}

void DiscussionThreadMessageCollectionBase::deleteDiscussionThreadMessage(DiscussionThreadMessageCollection::iterator iterator)
{
    if (iterator == messages_.end())
    {
        return;
    }
    messages_.erase(iterator);
}

void EntityCollection::deleteDiscussionThreadMessage(DiscussionThreadMessageCollection::iterator iterator)
{
    if (iterator == messages_.end())
    {
        return;
    }
    if (alsoDeleteMessagesFromUser)
    {
        (*iterator)->createdBy().deleteDiscussionThreadMessageById((*iterator)->id());
    }
    if (alsoDeleteMessagesFromThread)
    {
        modifyDiscussionThreadById((*iterator)->parentThread().id(),
                                   [&](DiscussionThread& thread)
        {
            thread.deleteDiscussionThreadMessageById((*iterator)->id());
            thread.resetVisitorsSinceLastEdit();
        });
    }
    messages_.erase(iterator);
}

void DiscussionThreadMessageCollectionBase::deleteDiscussionThreadMessageById(const IdType& id)
{
    deleteDiscussionThreadMessage(messages_.get<DiscussionThreadMessageCollectionById>().find(id));
}
