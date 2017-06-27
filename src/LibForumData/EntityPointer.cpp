#include "EntityPointer.h"
#include "EntityCollection.h"

using namespace Forum::Entities;
using namespace Forum::Entities::Private;

/**
 * Only one EntityCollection is used at a time during the runtime of the application or it's tests
 * Thus memory can be saved by keeping only an index to entities stored in vector collections.
 * 
 * Memory management also becomes easier than with std::shared/weak_ptrs
 */
static EntityCollection* globalEntityCollection = nullptr;

EntityCollection& Private::getGlobalEntityCollection()
{
    if (nullptr == globalEntityCollection)
    {
        throw std::runtime_error("Global EntityCollection is empty");
    }
    return *globalEntityCollection;
}

void Private::setGlobalEntityCollection(EntityCollection* collection)
{
    globalEntityCollection = collection;
}

template<>
User& Private::getEntityFromGlobalCollection<User>(size_t index)
{
    return *getGlobalEntityCollection().getUserPoolRoot()[index];
}

template<>
DiscussionThread& Private::getEntityFromGlobalCollection<DiscussionThread>(size_t index)
{
    return *getGlobalEntityCollection().getDiscussionThreadPoolRoot()[index];
}

template<>
DiscussionThreadMessage& Private::getEntityFromGlobalCollection<DiscussionThreadMessage>(size_t index)
{
    return *getGlobalEntityCollection().getDiscussionThreadMessagePoolRoot()[index];
}

template<>
DiscussionTag& Private::getEntityFromGlobalCollection<DiscussionTag>(size_t index)
{
    return *getGlobalEntityCollection().getDiscussionTagPoolRoot()[index];
}

template<>
DiscussionCategory& Private::getEntityFromGlobalCollection<DiscussionCategory>(size_t index)
{
    return *getGlobalEntityCollection().getDiscussionCategoryPoolRoot()[index];
}

template<>
MessageComment& Private::getEntityFromGlobalCollection<MessageComment>(size_t index)
{
    return *getGlobalEntityCollection().getMessageCommentPoolRoot()[index];
}
