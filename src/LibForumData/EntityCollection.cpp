#include "EntityCollection.h"

#include "StateHelpers.h"

using namespace Forum::Entities;
using namespace Forum::Helpers;

const UserRef Forum::Entities::AnonymousUser = std::make_shared<User>("<anonymous>");

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
        BoolTemporaryChanger changer(alsoDeleteMessagesFromUser, false);
        for (auto& message : (*iterator)->messages())
        {
            //Each discussion message holds a reference to the user that created it and the parent thread
            //As such, delete the discussion message before deleting the thread and the user
            static_cast<DiscussionMessageCollectionBase*>(this)->deleteDiscussionMessageById(message->id());
        }
    }
    {
        BoolTemporaryChanger changer(alsoDeleteThreadsFromUser, false);
        for (auto& thread : (*iterator)->threads())
        {
            //Each discussion thread holds a reference to the user that created it
            //As such, delete the discussion thread before deleting the user
            static_cast<DiscussionThreadCollectionBase*>(this)->deleteDiscussionThreadById(thread->id());
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
    threads_.modify(iterator, [&modifyFunction](const DiscussionThreadRef& thread)
    {
        if (thread)
        {
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
        BoolTemporaryChanger changer(alsoDeleteMessagesFromThread, false);
        for (auto& message : (*iterator)->messages())
        {
            //Each discussion message holds a reference to the user that created it and the parent thread
            //As such, delete the discussion message before deleting the thread
            static_cast<DiscussionMessageCollectionBase*>(this)->deleteDiscussionMessageById(message->id());
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

void DiscussionMessageCollectionBase::modifyDiscussionMessage(DiscussionMessageCollection::iterator iterator,
                                                              const std::function<void(DiscussionMessage&)>& modifyFunction)
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

void EntityCollection::modifyDiscussionMessage(DiscussionMessageCollection::iterator iterator,
                                               const std::function<void(DiscussionMessage&)>& modifyFunction)
{
    if (iterator == messages_.end())
    {
        return;
    }
    messages_.modify(iterator, [&modifyFunction](const DiscussionMessageRef& message)
    {
        if (message)
        {
            message->createdBy().modifyDiscussionMessageById(message->id(), [&modifyFunction](auto& message)
            {
                message.parentThread().modifyDiscussionMessageById(message.id(), modifyFunction);
            });
        }
    });
}

void DiscussionMessageCollectionBase::modifyDiscussionMessageById(const IdType& id,
                                                                  const std::function<void(DiscussionMessage&)>& modifyFunction)
{
    modifyDiscussionMessage(messages_.get<DiscussionMessageCollectionById>().find(id), modifyFunction);
}

void DiscussionMessageCollectionBase::deleteDiscussionMessage(DiscussionMessageCollection::iterator iterator)
{
    if (iterator == messages_.end())
    {
        return;
    }
    messages_.erase(iterator);
}

void EntityCollection::deleteDiscussionMessage(DiscussionMessageCollection::iterator iterator)
{
    if (iterator == messages_.end())
    {
        return;
    }
    if (alsoDeleteMessagesFromUser)
    {
        (*iterator)->createdBy().deleteDiscussionMessageById((*iterator)->id());
    }
    if (alsoDeleteMessagesFromThread)
    {
        modifyDiscussionThreadById((*iterator)->parentThread().id(),
                                   [&](DiscussionThread& thread)
        {
            thread.deleteDiscussionMessageById((*iterator)->id());
        });
    }
    messages_.erase(iterator);
}

void DiscussionMessageCollectionBase::deleteDiscussionMessageById(const IdType& id)
{
    deleteDiscussionMessage(messages_.get<DiscussionMessageCollectionById>().find(id));
}
