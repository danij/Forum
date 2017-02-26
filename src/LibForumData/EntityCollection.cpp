#include "EntityCollection.h"

#include "StateHelpers.h"
#include "ContextProviders.h"

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

UserRef UserCollectionBase::deleteUser(UserCollection::iterator iterator)
{
    UserRef result;
    if (iterator == users_.end())
    {
        return result;
    }
    result = *iterator;
    users_.erase(iterator);
    return result;
}

UserRef EntityCollection::deleteUser(UserCollection::iterator iterator)
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

UserRef UserCollectionBase::deleteUserById(const IdType& id)
{
    return deleteUser(users_.get<UserCollectionById>().find(id));
}

//
//
//Discussion Threads
//
//

bool DiscussionThreadCollectionBase::insertDiscussionThread(const DiscussionThreadRef& thread)
{
    return std::get<1>(threads_.insert(thread));
}

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

DiscussionThreadRef DiscussionThreadCollectionBase::deleteDiscussionThread(DiscussionThreadCollection::iterator iterator)
{
    DiscussionThreadRef result;
    if (iterator == threads_.end())
    {
        return result;
    }
    result = *iterator;
    threads_.erase(iterator);
    return result;
}

DiscussionThreadRef EntityCollection::deleteDiscussionThread(DiscussionThreadCollection::iterator iterator)
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
        thread->createdBy().deleteDiscussionThreadById(thread->id());
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
    return thread;
}

DiscussionThreadRef DiscussionThreadCollectionBase::deleteDiscussionThreadById(const IdType& id)
{
    return deleteDiscussionThread(threads_.get<DiscussionThreadCollectionById>().find(id));
}

//
//
//Discussion Messages
//
//

void DiscussionThreadMessageCollectionBase::modifyDiscussionThreadMessage(DiscussionThreadMessageCollection::iterator iterator,
                                                                          const std::function<void(DiscussionThreadMessage&)>& modifyFunction)
{
    if (iterator == messages_.end())
    {
        return;
    }
    messages_.modify(iterator, [&modifyFunction](const DiscussionThreadMessageRef& message)
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
    messages_.modify(iterator, [&modifyFunction](const DiscussionThreadMessageRef& message)
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

DiscussionThreadMessageRef DiscussionThreadMessageCollectionBase::deleteDiscussionThreadMessage(DiscussionThreadMessageCollection::iterator iterator)
{
    DiscussionThreadMessageRef result;
    if (iterator == messages_.end())
    {
        return result;
    }
    result = *iterator;
    messages_.erase(iterator);
    return result;
}

DiscussionThreadMessageRef EntityCollection::deleteDiscussionThreadMessage(DiscussionThreadMessageCollection::iterator iterator)
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
    if ( ! message->parentThread().aboutToBeDeleted())
    {
        modifyDiscussionThreadById(message->parentThread().id(),
                                   [&](DiscussionThread& thread)
        {
            thread.deleteDiscussionThreadMessageById(message->id());
            thread.resetVisitorsSinceLastEdit();
            thread.latestVisibleChange() = Context::getCurrentTime();

            for (auto& tagWeak : thread.tagsWeak())
            {
                if (auto tagShared = tagWeak.lock())
                {
                    modifyDiscussionTagById(tagShared->id(), [&thread](auto& tag)
                    {
                        tag.messageCount() -= 1;
                        //notify the thread collection of each tag that the thread has fewer messages
                        tag.modifyDiscussionThreadById(thread.id(), [](auto& _) {});
                    });
                }
            }
            for (auto& categoryWeak : thread.categoriesWeak())
            {
                if (auto categoryShared = categoryWeak.lock())
                {
                    auto threadShared = thread.shared_from_this();
                    modifyDiscussionCategoryById(categoryShared->id(), [&thread, &threadShared](auto& category)
                    {
                        category.updateMessageCount(threadShared, -1);
                        //notify the thread collection of each category that the thread has fewer messages
                        category.modifyDiscussionThreadById(thread.id(), [](auto& _) {});
                    });
                }
            }
        });
    }
    return message;
}

DiscussionThreadMessageRef DiscussionThreadMessageCollectionBase::deleteDiscussionThreadMessageById(const IdType& id)
{
    return deleteDiscussionThreadMessage(messages_.get<DiscussionThreadMessageCollectionById>().find(id));
}

//
//
//Discussion Tags
//
//

void DiscussionTagCollectionBase::modifyDiscussionTag(DiscussionTagCollection::iterator iterator,
                                                      const std::function<void(DiscussionTag&)>& modifyFunction)
{
    if (iterator == tags_.end())
    {
        return;
    }
    tags_.modify(iterator, [&modifyFunction](const DiscussionTagRef& tag)
    {
        if (tag)
        {
            modifyFunction(*tag);
        }
    });
}

void DiscussionTagCollectionBase::modifyDiscussionTagById(const IdType& id,
    const std::function<void(DiscussionTag&)>& modifyFunction)
{
    modifyDiscussionTag(tags_.get<DiscussionTagCollectionById>().find(id), modifyFunction);
}

DiscussionTagRef DiscussionTagCollectionBase::deleteDiscussionTag(DiscussionTagCollection::iterator iterator)
{
    DiscussionTagRef result;
    if (iterator == tags_.end())
    {
        return result;
    }
    result = *iterator;
    tags_.erase(iterator);
    return result;
}

DiscussionTagRef EntityCollection::deleteDiscussionTag(DiscussionTagCollection::iterator iterator)
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


DiscussionTagRef DiscussionTagCollectionBase::deleteDiscussionTagById(const IdType& id)
{
    return deleteDiscussionTag(tags_.get<DiscussionTagCollectionById>().find(id));
}

//
//
//Discussion Categories
//
//

void DiscussionCategoryCollectionBase::modifyDiscussionCategory(DiscussionCategoryCollection::iterator iterator,
    const std::function<void(DiscussionCategory&)>& modifyFunction)
{
    if (iterator == categories_.end())
    {
        return;
    }
    categories_.modify(iterator, [&modifyFunction](const DiscussionCategoryRef& category)
    {
        if (category)
        {
            modifyFunction(*category);
        }
    });
}

void DiscussionCategoryCollectionBase::modifyDiscussionCategoryById(const IdType& id,
    const std::function<void(DiscussionCategory&)>& modifyFunction)
{
    modifyDiscussionCategory(categories_.get<DiscussionCategoryCollectionById>().find(id), modifyFunction);
}

DiscussionCategoryRef DiscussionCategoryCollectionBase::deleteDiscussionCategory(DiscussionCategoryCollection::iterator iterator)
{
    DiscussionCategoryRef result;
    if (iterator == categories_.end())
    {
        return result;
    }
    result = *iterator;
    categories_.erase(iterator);
    return result;
}

DiscussionCategoryRef EntityCollection::deleteDiscussionCategory(DiscussionCategoryCollection::iterator iterator)
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

DiscussionCategoryRef DiscussionCategoryCollectionBase::deleteDiscussionCategoryById(const IdType& id)
{
    return deleteDiscussionCategory(categories_.get<DiscussionCategoryCollectionById>().find(id));
}
