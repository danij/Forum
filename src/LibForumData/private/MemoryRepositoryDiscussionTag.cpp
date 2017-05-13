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
                           
                           auto nameString = toString(name);

                           auto& indexByName = collection.tags().get<EntityCollection::DiscussionTagCollectionByName>();
                           if (indexByName.find(nameString) != indexByName.end())
                           {
                               status = StatusCode::ALREADY_EXISTS;
                               return;
                           }
                 
                           if ( ! (status = authorization_->addNewDiscussionTag(*currentUser, name)))
                           {
                               return;
                           }

                           auto tag = std::make_shared<DiscussionTag>(collection);
                           tag->notifyChange() = collection.notifyTagChange();
                           tag->id() = generateUUIDString();
                           tag->name() = std::move(nameString);
                           updateCreated(*tag);
                 
                           collection.tags().insert(tag);

                           writeEvents().onAddNewDiscussionTag(createObserverContext(*currentUser), *tag);
                 
                           status.writeNow([&](auto& writer)
                                           {
                                               writer << Json::propertySafeName("id", tag->id());
                                               writer << Json::propertySafeName("name", tag->name());
                                           });
                       });
    return status;
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

                           auto newNameString = toString(newName);

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

                           collection.modifyDiscussionTag(it, [&newNameString, &currentUser](DiscussionTag& tag)
                                                              {
                                                                  tag.name() = std::move(newNameString);
                                                                  updateLastUpdated(tag, currentUser);
                                                              });
                           writeEvents().onChangeDiscussionTag(createObserverContext(*currentUser), **it,
                                                               DiscussionTag::ChangeType::Name);
                       });
    return status;
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

                           collection.modifyDiscussionTag(it, [&blob](DiscussionTag& tag)
                                                              {
                                                                  tag.uiBlob() = toString(blob);
                                                              });
                           writeEvents().onChangeDiscussionTag(createObserverContext(*currentUser),
                                                               **it, DiscussionTag::ChangeType::UIBlob);
                       });
    return status;
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

                           collection.deleteDiscussionTag(it);
                       });
    return status;
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
                 
                           auto& tagRef = *tagIt;
                           auto& tag = **tagIt;
                           auto& threadRef = *threadIt;
                           auto& thread = **threadIt;
                 
                           if ( ! (status = authorization_->addDiscussionTagToThread(*currentUser, tag, thread)))
                           {
                               return;
                           }

                           //the number of tags associated to a thread is much smaller than 
                           //the number of threads associated to a tag, so search the tag in the thread
                           if ( ! thread.addTag(tagRef))
                           {
                               //actually already added, but return ok
                               status = StatusCode::OK;
                               return;
                           }
                 
                           tag.insertDiscussionThread(threadRef);
                           updateLastUpdated(thread, currentUser);

                           writeEvents().onAddDiscussionTagToThread(createObserverContext(*currentUser), tag, thread);
                       });
    return status;
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
                 
                           auto& tagRef = *tagIt;
                           auto& tag = **tagIt;
                           auto& thread = **threadIt;
                 
                           if ( ! (status = authorization_->removeDiscussionTagFromThread(*currentUser, tag, thread)))
                           {
                               return;
                           }
                           
                           if ( ! thread.removeTag(tagRef))
                           {
                               //tag was not added to the thread
                               status = StatusCode::NO_EFFECT;
                               return;
                           }
                 
                           tag.deleteDiscussionThreadById(threadId);
                           updateLastUpdated(thread, currentUser);

                           writeEvents().onRemoveDiscussionTagFromThread(createObserverContext(*currentUser), tag, thread);
                       });
    return status;
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
                           auto& tagIntoRef = *itInto;
                           auto& tagInto = **itInto;
                   
                           if ( ! (status = authorization_->mergeDiscussionTags(*currentUser, tagFrom, tagInto)))
                           {
                               return;
                           }

                           //make sure the tag is not deleted before being passed to the observers
                           writeEvents().onMergeDiscussionTags(createObserverContext(*currentUser), tagFrom, tagInto);
                           
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
                       });
    return status;
}
