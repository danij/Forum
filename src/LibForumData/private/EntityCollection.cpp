#include "EntityCollection.h"

#include "StateHelpers.h"
#include "ContextProviders.h"

using namespace Forum::Entities;
using namespace Forum::Helpers;

const UserRef Forum::Entities::AnonymousUser = std::make_shared<User>("<anonymous>");
const IdType Forum::Entities::AnonymousUserId = AnonymousUser->id();

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
                    userShared->subscribedThreads().modifyDiscussionThreadById(thread->id());
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
                            tag.modifyDiscussionThreadById(thread.id());
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
                            category.modifyDiscussionThreadById(thread.id());
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
