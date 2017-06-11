#include "MemoryRepositoryDiscussionTag.h"

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"
#include "RandomGenerator.h"
#include "StringHelpers.h"

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;
using namespace Forum::Authorization;

MemoryRepositoryDiscussionTag::MemoryRepositoryDiscussionTag(MemoryStoreRef store,
                                                             DiscussionTagAuthorizationRef authorization)
    : MemoryRepositoryBase(std::move(store)), authorization_(std::move(authorization))
{
    if ( ! authorization_)
    {
        throw std::runtime_error("Authorization implementation not provided");
    }
}


StatusCode MemoryRepositoryDiscussionTag::getDiscussionTags(OutStream& output, RetrieveDiscussionTagsBy by) const
{
    StatusWriter status(output);
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
    {
        auto& currentUser = performedBy.get(collection, store());

        if ( ! (status = authorization_->getDiscussionTags(currentUser)))
        {
            return;
        }

        SerializationRestriction restriction(collection.grantedPrivileges(), currentUser, Context::getCurrentTime());

        if (Context::getDisplayContext().sortOrder == Context::SortOrder::Ascending)
        {
            switch (by)
            {
            case RetrieveDiscussionTagsBy::Name:
                writeArraySafeName(output, "tags", collection.tagsByName().begin(), collection.tagsByName().end(),
                                   restriction);
                status.disable();
                break;
            case RetrieveDiscussionTagsBy::MessageCount:
                writeArraySafeName(output, "tags", collection.tagsByMessageCount().begin(),
                                   collection.tagsByMessageCount().end(), restriction);
                status.disable();
                break;
            }
        }
        else
        {
            switch (by)
            {
            case RetrieveDiscussionTagsBy::Name:
                writeArraySafeName(output, "tags", collection.tagsByName().rbegin(), collection.tagsByName().rend(),
                                   restriction);
                status.disable();
                break;
            case RetrieveDiscussionTagsBy::MessageCount:
                writeArraySafeName(output, "tags", collection.tagsByMessageCount().rbegin(),
                                   collection.tagsByMessageCount().rend(), restriction);
                status.disable();
                break;
            }
        }

        readEvents().onGetDiscussionTags(createObserverContext(currentUser));
    });
    return status;
}


StatusCode MemoryRepositoryDiscussionTag::addNewDiscussionTag(StringView name, OutStream& output)
{
    StatusWriter status(output);

    auto config = getGlobalConfig();
    auto validationCode = validateString(name, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                         config->discussionTag.minNameLength,
                                         config->discussionTag.maxNameLength,
                                         &MemoryRepositoryBase::doesNotContainLeadingOrTrailingWhitespace);
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           if ( ! (status = authorization_->addNewDiscussionTag(*currentUser, name)))
                           {
                               return;
                           }

                           auto statusWithResource = addNewDiscussionTag(collection, name);
                           auto& tag = statusWithResource.resource;
                           if ( ! (status = statusWithResource.status)) return;

                           writeEvents().onAddNewDiscussionTag(createObserverContext(*currentUser), *tag);

                           status.writeNow([&](auto& writer)
                                           {
                                               writer << Json::propertySafeName("id", tag->id());
                                               writer << Json::propertySafeName("name", tag->name().string());
                                           });
                       });
    return status;
}

StatusWithResource<DiscussionTagRef> MemoryRepositoryDiscussionTag::addNewDiscussionTag(EntityCollection& collection,
                                                                                        StringView name)
{
    StringWithSortKey nameString(name);

    auto& indexByName = collection.tags().get<EntityCollection::DiscussionTagCollectionByName>();
    if (indexByName.find(nameString) != indexByName.end())
    {
        return StatusCode::ALREADY_EXISTS;
    }

    auto tag = std::make_shared<DiscussionTag>(collection);
    tag->notifyChange() = collection.notifyTagChange();
    tag->id() = generateUUIDString();
    tag->name() = std::move(nameString);
    updateCreated(*tag);

    collection.tags().insert(tag);

    return tag;
}

StatusCode MemoryRepositoryDiscussionTag::changeDiscussionTagName(const IdType& id, StringView newName,
                                                                  OutStream& output)
{
    StatusWriter status(output);
    if ( ! id )
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    auto config = getGlobalConfig();
    auto validationCode = validateString(newName, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                         config->discussionTag.minNameLength,
                                         config->discussionTag.maxNameLength,
                                         &MemoryRepositoryBase::doesNotContainLeadingOrTrailingWhitespace);
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.tags().get<EntityCollection::DiscussionTagCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           StringWithSortKey newNameString(newName);

                           auto& indexByName = collection.tags().get<EntityCollection::DiscussionTagCollectionByName>();
                           if (indexByName.find(newNameString) != indexByName.end())
                           {
                               status = StatusCode::ALREADY_EXISTS;
                               return;
                           }

                           if ( ! (status = authorization_->changeDiscussionTagName(*currentUser, **it, newName)))
                           {
                               return;
                           }

                           if ( ! (status = changeDiscussionTagName(collection, id, newName))) return;

                           writeEvents().onChangeDiscussionTag(createObserverContext(*currentUser), **it,
                                                               DiscussionTag::ChangeType::Name);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionTag::changeDiscussionTagName(EntityCollection& collection, IdTypeRef id,
                                                                  StringView newName)
{
    auto currentUser = getCurrentUser(collection);

    auto& indexById = collection.tags().get<EntityCollection::DiscussionTagCollectionById>();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    StringWithSortKey newNameString(newName);

    collection.modifyDiscussionTag(it, [&newNameString, &currentUser](DiscussionTag& tag)
                                        {
                                            tag.name() = std::move(newNameString);
                                            updateLastUpdated(tag, currentUser);
                                        });
    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionTag::changeDiscussionTagUiBlob(const IdType& id, StringView blob,
                                                                    OutStream& output)
{
    StatusWriter status(output);
    if ( ! id )
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    if (blob.size() > static_cast<decltype(blob.size())>(getGlobalConfig()->discussionTag.maxUiBlobSize))
    {
        return status = StatusCode::VALUE_TOO_LONG;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.tags().get<EntityCollection::DiscussionTagCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           if ( ! (status = authorization_->changeDiscussionTagUiBlob(*currentUser, **it, blob)))
                           {
                               return;
                           }

                           if ( ! (status = changeDiscussionTagUiBlob(collection, id, blob))) return;

                           writeEvents().onChangeDiscussionTag(createObserverContext(*currentUser),
                                                               **it, DiscussionTag::ChangeType::UIBlob);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionTag::changeDiscussionTagUiBlob(EntityCollection& collection, IdTypeRef id, StringView blob)
{
    auto currentUser = getCurrentUser(collection);

    auto& indexById = collection.tags().get<EntityCollection::DiscussionTagCollectionById>();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    collection.modifyDiscussionTag(it, [&blob](DiscussionTag& tag)
                                       {
                                           tag.uiBlob() = toString(blob);
                                       });
    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionTag::deleteDiscussionTag(const IdType& id, OutStream& output)
{
    StatusWriter status(output);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.tags().get<EntityCollection::DiscussionTagCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           if ( ! (status = authorization_->deleteDiscussionTag(*currentUser, **it)))
                           {
                               return;
                           }

                           //make sure the tag is not deleted before being passed to the observers
                           writeEvents().onDeleteDiscussionTag(createObserverContext(*currentUser), **it);

                           status = deleteDiscussionTag(collection, id);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionTag::deleteDiscussionTag(EntityCollection& collection, IdTypeRef id)
{
    auto& indexById = collection.tags().get<EntityCollection::DiscussionTagCollectionById>();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    collection.deleteDiscussionTag(it);
    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionTag::addDiscussionTagToThread(const IdType& tagId, const IdType& threadId,
                                                                   OutStream& output)
{
    StatusWriter status(output);
    if ( ! tagId || ! threadId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& tagIndexById = collection.tags().get<EntityCollection::DiscussionTagCollectionById>();
                           auto tagIt = tagIndexById.find(tagId);
                           if (tagIt == tagIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           auto& threadIndexById = collection.threads().get<EntityCollection::DiscussionThreadCollectionById>();
                           auto threadIt = threadIndexById.find(threadId);
                           if (threadIt == threadIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           auto& tag = **tagIt;
                           auto& thread = **threadIt;

                           if ( ! (status = authorization_->addDiscussionTagToThread(*currentUser, tag, thread)))
                           {
                               return;
                           }

                           if ( ! (status = addDiscussionTagToThread(collection, tagId, threadId))) return;

                           writeEvents().onAddDiscussionTagToThread(createObserverContext(*currentUser), tag, thread);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionTag::addDiscussionTagToThread(EntityCollection& collection, IdTypeRef tagId,
                                                                   IdTypeRef threadId)
{
    auto& tagIndexById = collection.tags().get<EntityCollection::DiscussionTagCollectionById>();
    auto tagIt = tagIndexById.find(tagId);
    if (tagIt == tagIndexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    auto& threadIndexById = collection.threads().get<EntityCollection::DiscussionThreadCollectionById>();
    auto threadIt = threadIndexById.find(threadId);
    if (threadIt == threadIndexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    auto& tagRef = *tagIt;
    auto& tag = **tagIt;
    auto& threadRef = *threadIt;
    auto& thread = **threadIt;

    auto currentUser = getCurrentUser(collection);

    //the number of tags associated to a thread is much smaller than
    //the number of threads associated to a tag, so search the tag in the thread
    if ( ! thread.addTag(tagRef))
    {
        //actually already added, but return ok
        return StatusCode::OK;
    }

    tag.insertDiscussionThread(threadRef);
    updateLastUpdated(thread, currentUser);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionTag::removeDiscussionTagFromThread(const IdType& tagId, const IdType& threadId,
                                                                        OutStream& output)
{
    StatusWriter status(output);
    if ( ! tagId || ! threadId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& tagIndexById = collection.tags().get<EntityCollection::DiscussionTagCollectionById>();
                           auto tagIt = tagIndexById.find(tagId);
                           if (tagIt == tagIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           auto& threadIndexById = collection.threads().get<EntityCollection::DiscussionThreadCollectionById>();
                           auto threadIt = threadIndexById.find(threadId);
                           if (threadIt == threadIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           auto& tag = **tagIt;
                           auto& thread = **threadIt;

                           if ( ! (status = authorization_->removeDiscussionTagFromThread(*currentUser, tag, thread)))
                           {
                               return;
                           }

                           if ( ! (status = removeDiscussionTagFromThread(collection, tagId, threadId))) return;

                           writeEvents().onRemoveDiscussionTagFromThread(createObserverContext(*currentUser), tag, thread);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionTag::removeDiscussionTagFromThread(EntityCollection& collection,
                                                                        IdTypeRef tagId, IdTypeRef threadId)
{

    auto& tagIndexById = collection.tags().get<EntityCollection::DiscussionTagCollectionById>();
    auto tagIt = tagIndexById.find(tagId);
    if (tagIt == tagIndexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    auto& threadIndexById = collection.threads().get<EntityCollection::DiscussionThreadCollectionById>();
    auto threadIt = threadIndexById.find(threadId);
    if (threadIt == threadIndexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    auto& tagRef = *tagIt;
    auto& tag = **tagIt;
    auto& thread = **threadIt;

    auto currentUser = getCurrentUser(collection);

    if ( ! thread.removeTag(tagRef))
    {
        //tag was not added to the thread
        return StatusCode::NO_EFFECT;
    }

    tag.deleteDiscussionThreadById(threadId);
    updateLastUpdated(thread, currentUser);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionTag::mergeDiscussionTags(const IdType& fromId, const IdType& intoId,
                                                              OutStream& output)
{
    StatusWriter status(output);
    if ( ! fromId || ! intoId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    if (fromId == intoId)
    {
        return status = StatusCode::NO_EFFECT;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.tags().get<EntityCollection::DiscussionTagCollectionById>();
                           auto itFrom = indexById.find(fromId);
                           if (itFrom == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           auto itInto = indexById.find(intoId);
                           if (itInto == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           auto& tagFrom = **itFrom;
                           auto& tagInto = **itInto;

                           if ( ! (status = authorization_->mergeDiscussionTags(*currentUser, tagFrom, tagInto)))
                           {
                               return;
                           }

                           //make sure the tag is not deleted before being passed to the observers
                           writeEvents().onMergeDiscussionTags(createObserverContext(*currentUser), tagFrom, tagInto);

                           status = mergeDiscussionTags(collection, fromId, intoId);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionTag::mergeDiscussionTags(EntityCollection& collection, IdTypeRef fromId,
                                                              IdTypeRef intoId)
{
    auto& indexById = collection.tags().get<EntityCollection::DiscussionTagCollectionById>();
    auto itFrom = indexById.find(fromId);
    if (itFrom == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }
    auto itInto = indexById.find(intoId);
    if (itInto == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    auto& tagFrom = **itFrom;
    auto& tagIntoRef = *itInto;
    auto& tagInto = **itInto;

    auto currentUser = getCurrentUser(collection);

    for (auto& thread : tagFrom.threads())
    {
        thread->addTag(tagIntoRef);
        updateLastUpdated(*thread, currentUser);
        tagInto.insertDiscussionThread(thread);
    }
    for (auto& categoryWeak : tagFrom.categoriesWeak())
    {
        if (auto category = categoryWeak.lock())
        {
            category->addTag(tagIntoRef);
            updateLastUpdated(*category, currentUser);
        }
    }

    updateLastUpdated(tagInto, currentUser);

    collection.deleteDiscussionTag(itFrom);

    return StatusCode::OK;
}
