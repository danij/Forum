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
        auto& currentUser = performedBy.get(collection);

        if ( ! (status = authorization_->getDiscussionTags(currentUser)))
        {
            return;
        }

        SerializationRestriction restriction(collection.grantedPrivileges(), currentUser.id(), Context::getCurrentTime());

        if (Context::getDisplayContext().sortOrder == Context::SortOrder::Ascending)
        {
            switch (by)
            {
            case RetrieveDiscussionTagsBy::Name:
                writeArraySafeName(output, "tags", collection.tags().byName().begin(), collection.tags().byName().end(),
                                   restriction);
                status.disable();
                break;
            case RetrieveDiscussionTagsBy::MessageCount:
                writeArraySafeName(output, "tags", collection.tags().byMessageCount().begin(),
                                   collection.tags().byMessageCount().end(), restriction);
                status.disable();
                break;
            }
        }
        else
        {
            switch (by)
            {
            case RetrieveDiscussionTagsBy::Name:
                writeArraySafeName(output, "tags", collection.tags().byName().rbegin(), collection.tags().byName().rend(),
                                   restriction);
                status.disable();
                break;
            case RetrieveDiscussionTagsBy::MessageCount:
                writeArraySafeName(output, "tags", collection.tags().byMessageCount().rbegin(),
                                   collection.tags().byMessageCount().rend(), restriction);
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

                           auto statusWithResource = addNewDiscussionTag(collection, generateUniqueId(), name);
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

StatusWithResource<DiscussionTagPtr> MemoryRepositoryDiscussionTag::addNewDiscussionTag(EntityCollection& collection,
                                                                                        IdTypeRef id, StringView name)
{
    DiscussionTag::NameType nameString(name);

    auto& indexByName = collection.tags().byName();
    if (indexByName.find(nameString) != indexByName.end())
    {
        return StatusCode::ALREADY_EXISTS;
    }

    //IdType id, Timestamp created, VisitDetails creationDetails
    auto tag = collection.createDiscussionTag(id, std::move(nameString), Context::getCurrentTime(), 
                                              { Context::getCurrentUserIpAddress() });
    collection.insertDiscussionTag(tag);

    return tag;
}

StatusCode MemoryRepositoryDiscussionTag::changeDiscussionTagName(IdTypeRef id, StringView newName,
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

                           auto& indexById = collection.tags().byId();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           DiscussionTag::NameType newNameString(newName);

                           auto& indexByName = collection.tags().byName();
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

    auto& indexById = collection.tags().byId();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    DiscussionTag::NameType newNameString(newName);

    DiscussionTagPtr tagPtr = *it;
    DiscussionTag& tag = *tagPtr;

    tag.updateName(std::move(newNameString));
    updateLastUpdated(tag, currentUser);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionTag::changeDiscussionTagUiBlob(IdTypeRef id, StringView blob,
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

                           auto& indexById = collection.tags().byId();
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
    auto& indexById = collection.tags().byId();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    DiscussionTagPtr tagPtr = *it;
    tagPtr->uiBlob() = toString(blob);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionTag::deleteDiscussionTag(IdTypeRef id, OutStream& output)
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

                           auto& indexById = collection.tags().byId();
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
    auto& indexById = collection.tags().byId();
    auto it = indexById.find(id);
    if (it == indexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    collection.deleteDiscussionTag(*it);
    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionTag::addDiscussionTagToThread(IdTypeRef tagId, IdTypeRef threadId,
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

                           auto& tagIndexById = collection.tags().byId();
                           auto tagIt = tagIndexById.find(tagId);
                           if (tagIt == tagIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           auto& threadIndexById = collection.threads().byId();
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
    auto& tagIndexById = collection.tags().byId();
    auto tagIt = tagIndexById.find(tagId);
    if (tagIt == tagIndexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    auto& threadIndexById = collection.threads().byId();
    auto threadIt = threadIndexById.find(threadId);
    if (threadIt == threadIndexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    DiscussionTagPtr tagPtr = *tagIt;
    DiscussionTag& tag = *tagPtr;
    DiscussionThreadPtr threadPtr = *threadIt;
    DiscussionThread& thread = *threadPtr;

    auto currentUser = getCurrentUser(collection);

    //the number of tags associated to a thread is much smaller than
    //the number of threads associated to a tag, so search the tag in the thread
    if ( ! thread.addTag(tagPtr))
    {
        //actually already added, but return ok
        return StatusCode::OK;
    }

    tag.insertDiscussionThread(threadPtr);
    updateThreadLastUpdated(thread, currentUser);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionTag::removeDiscussionTagFromThread(IdTypeRef tagId, IdTypeRef threadId,
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

                           auto& tagIndexById = collection.tags().byId();
                           auto tagIt = tagIndexById.find(tagId);
                           if (tagIt == tagIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           auto& threadIndexById = collection.threads().byId();
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

    auto& tagIndexById = collection.tags().byId();
    auto tagIt = tagIndexById.find(tagId);
    if (tagIt == tagIndexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    auto& threadIndexById = collection.threads().byId();
    auto threadIt = threadIndexById.find(threadId);
    if (threadIt == threadIndexById.end())
    {
        return StatusCode::NOT_FOUND;
    }

    DiscussionTagPtr tagPtr = *tagIt;
    DiscussionTag& tag = *tagPtr;
    DiscussionThreadPtr threadPtr = *threadIt;
    DiscussionThread& thread = *threadPtr;

    auto currentUser = getCurrentUser(collection);

    if ( ! thread.removeTag(tagPtr))
    {
        //tag was not added to the thread
        return StatusCode::NO_EFFECT;
    }

    tag.deleteDiscussionThread(threadPtr);
    updateThreadLastUpdated(thread, currentUser);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionTag::mergeDiscussionTags(IdTypeRef fromId, IdTypeRef intoId,
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

                           auto& indexById = collection.tags().byId();
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
    auto& indexById = collection.tags().byId();
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

    DiscussionTagPtr tagFromPtr = *itFrom;
    DiscussionTag& tagFrom = *tagFromPtr;
    DiscussionTagPtr tagIntoRef = *itInto;
    DiscussionTag& tagInto = *tagIntoRef;

    auto currentUser = getCurrentUser(collection);

    for (DiscussionThreadPtr thread : tagFrom.threads().byId())
    {
        assert(thread);
        thread->addTag(tagIntoRef);

        updateThreadLastUpdated(*thread, currentUser);

        tagInto.insertDiscussionThread(thread);
    }

    for (DiscussionCategoryPtr category : tagFrom.categories())
    {
        assert(category);
        category->addTag(tagIntoRef);

        updateLastUpdated(*category, currentUser);
    }

    updateLastUpdated(tagInto, currentUser);

    collection.deleteDiscussionTag(tagFromPtr);

    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionTag::getDiscussionThreadMessageRequiredPrivileges(IdTypeRef tagId, 
                                                                                       OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection);

                          const auto& index = collection.tags().byId();
                          auto it = index.find(tagId);
                          if (it == index.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          auto& tag = **it;

                          if ( ! (status = authorization_->getDiscussionTagById(currentUser, tag)))
                          {
                              return;
                          }

                          status.disable();

                          writeDiscussionThreadMessageRequiredPrivileges(tag, output);

                          readEvents().onGetDiscussionThreadMessageRequiredPrivilegesFromTag(
                                  createObserverContext(currentUser), tag);
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionTag::getDiscussionThreadRequiredPrivileges(IdTypeRef tagId, 
                                                                                OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection);

                          const auto& index = collection.tags().byId();
                          auto it = index.find(tagId);
                          if (it == index.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          auto& tag = **it;

                          if ( ! (status = authorization_->getDiscussionTagById(currentUser, tag)))
                          {
                              return;
                          }

                          status.disable();

                          writeDiscussionThreadRequiredPrivileges(tag, output);

                          readEvents().onGetDiscussionThreadRequiredPrivilegesFromTag(
                                  createObserverContext(currentUser), tag);
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionTag::getDiscussionTagRequiredPrivileges(IdTypeRef tagId, 
                                                                             OutStream& output) const
{
    StatusWriter status(output);

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection);

                          const auto& index = collection.tags().byId();
                          auto it = index.find(tagId);
                          if (it == index.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          auto& tag = **it;

                          if ( ! (status = authorization_->getDiscussionTagById(currentUser, tag)))
                          {
                              return;
                          }

                          status.disable();

                          writeDiscussionTagRequiredPrivileges(tag, output);

                          readEvents().onGetDiscussionTagRequiredPrivilegesFromTag(
                                  createObserverContext(currentUser), tag);
                      });
    return status;
}