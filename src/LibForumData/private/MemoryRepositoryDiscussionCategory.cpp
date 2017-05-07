#include "MemoryRepositoryDiscussionCategory.h"

#include "Configuration.h"
#include "ContextProviders.h"
#include "EntitySerialization.h"
#include "OutputHelpers.h"
#include "RandomGenerator.h"
#include "StateHelpers.h"
#include "StringHelpers.h"

using namespace Forum;
using namespace Forum::Configuration;
using namespace Forum::Entities;
using namespace Forum::Helpers;
using namespace Forum::Repository;
using namespace Forum::Authorization;

MemoryRepositoryDiscussionCategory::MemoryRepositoryDiscussionCategory(MemoryStoreRef store, 
                                                                       DiscussionCategoryAuthorizationRef authorization)
    : MemoryRepositoryBase(std::move(store)), authorization_(std::move(authorization))
{
    if ( ! authorization_)
    {
        throw std::runtime_error("Authorization implementation not provided");
    }
}

StatusCode MemoryRepositoryDiscussionCategory::getDiscussionCategories(OutStream& output,
                                                                       RetrieveDiscussionCategoriesBy by) const
{
    StatusWriter status(output, StatusCode::OK);
    PerformedByWithLastSeenUpdateGuard performedBy;
    AuthorizationStatus authorizationStatus;

    collection().read([&](const EntityCollection& collection)
    {
        auto& currentUser = performedBy.get(collection, store());

        authorizationStatus = authorization_->getDiscussionCategories(currentUser);
        if (AuthorizationStatus::OK != authorizationStatus)
        {
            status = authorizationStatus;
            return;
        }

        if (Context::getDisplayContext().sortOrder == Context::SortOrder::Ascending)
        {
            switch (by)
            {
            case RetrieveDiscussionCategoriesBy::Name: 
                writeSingleValueSafeName(output, "categories", Json::enumerate(collection.categoriesByName().begin(), 
                                                                               collection.categoriesByName().end()));
                status.disable();
                break;
            case RetrieveDiscussionCategoriesBy::MessageCount: 
                writeSingleValueSafeName(output, "categories", Json::enumerate(collection.categoriesByMessageCount().begin(), 
                                                                               collection.categoriesByMessageCount().end()));
                status.disable();
                break;
            }
        }
        else
        {
            switch (by)
            {
            case RetrieveDiscussionCategoriesBy::Name: 
                writeSingleValueSafeName(output, "categories", Json::enumerate(collection.categoriesByName().rbegin(), 
                                                                               collection.categoriesByName().rend()));
                status.disable();
                break;
            case RetrieveDiscussionCategoriesBy::MessageCount: 
                writeSingleValueSafeName(output, "categories", Json::enumerate(collection.categoriesByMessageCount().rbegin(), 
                                                                               collection.categoriesByMessageCount().rend()));
                status.disable();
                break;
            }
        }

        readEvents().onGetDiscussionCategories(createObserverContext(currentUser));
    });
    return status;
}

StatusCode MemoryRepositoryDiscussionCategory::getDiscussionCategoriesFromRoot(OutStream& output) const
{
    StatusWriter status(output, StatusCode::OK);

    PerformedByWithLastSeenUpdateGuard performedBy;
    AuthorizationStatus authorizationStatus;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, store());

                          authorizationStatus = authorization_->getDiscussionCategoriesFromRoot(currentUser);
                          if (AuthorizationStatus::OK != authorizationStatus)
                          {
                              status = authorizationStatus;
                              return;
                          }

                          auto index = collection.categoriesByDisplayOrderRootPriority();
                          auto indexBegin = index.begin();
                          auto indexEnd = index.end();

                          auto indexRootEnd = indexBegin;
                          while ((indexRootEnd != indexEnd) && ((*indexRootEnd)->isRootCategory()))
                          {
                              ++indexRootEnd;
                          }
                          
                          BoolTemporaryChanger _(serializationSettings.showDiscussionCategoryChildren, true);

                          status.disable();
                          writeSingleValueSafeName(output, "categories", Json::enumerate(indexBegin, indexRootEnd));

                          readEvents().onGetRootDiscussionCategories(createObserverContext(currentUser));
                      });
    return status;
}

StatusCode MemoryRepositoryDiscussionCategory::getDiscussionCategoryById(const IdType& id, OutStream& output) const
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;
    AuthorizationStatus authorizationStatus;

    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, store());

                          const auto& index = collection.categoriesById();
                          auto it = index.find(id);
                          if (it == index.end())
                          {
                              status = StatusCode::NOT_FOUND;
                              return;
                          }

                          authorizationStatus = authorization_->getDiscussionCategoryById(currentUser, **it);
                          if (AuthorizationStatus::OK != authorizationStatus)
                          {
                              status = authorizationStatus;
                              return;
                          }

                          status.disable();
                          BoolTemporaryChanger _(serializationSettings.showDiscussionCategoryChildren, true);
                          writeSingleValueSafeName(output, "category", **it);

                          readEvents().onGetDiscussionCategory(createObserverContext(currentUser), **it);
                      });
    return status;
}


StatusCode MemoryRepositoryDiscussionCategory::addNewDiscussionCategory(StringView name, const IdType& parentId,
                                                                        OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);

    auto config = getGlobalConfig();
    auto validationCode = validateString(name, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                         config->discussionCategory.minNameLength, 
                                         config->discussionCategory.maxNameLength,
                                         &MemoryRepositoryBase::doesNotContainLeadingOrTrailingWhitespace);
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;
    AuthorizationStatus authorizationStatus;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto nameString = toString(name);
                           
                           auto& indexByName = collection.categories().get<EntityCollection::DiscussionCategoryCollectionByName>();
                           if (indexByName.find(nameString) != indexByName.end())
                           {
                               status = StatusCode::ALREADY_EXISTS;
                               return;
                           }

                           auto& indexById = collection.categories().get<EntityCollection::DiscussionCategoryCollectionById>();
                           auto parentIt = indexById.find(parentId);

                           authorizationStatus = authorization_->addNewDiscussionCategory(*currentUser, name, *parentIt);
                           if (AuthorizationStatus::OK != authorizationStatus)
                           {
                               status = authorizationStatus;
                               return;
                           }
                 
                           auto category = std::make_shared<DiscussionCategory>(collection);
                           category->notifyChange() = collection.notifyCategoryChange();
                           category->id() = generateUUIDString();
                           category->name() = std::move(nameString);
                           updateCreated(*category);
                 
                           auto setParentId = IdType::empty;
                           if (parentId)
                           {
                               if (parentIt != indexById.end())
                               {
                                   collection.modifyDiscussionCategory(parentIt, [&category](DiscussionCategory& parent)
                                                                                 {
                                                                                     parent.addChild(category);
                                                                                 });
                                   category->parentWeak() = *parentIt;
                                   setParentId = parentId;
                               }
                           }
                 
                           collection.categories().insert(category);

                           writeEvents().onAddNewDiscussionCategory(createObserverContext(*currentUser), *category);
                 
                           status.writeNow([&](auto& writer)
                                           {
                                               writer << Json::propertySafeName("id", category->id());
                                               writer << Json::propertySafeName("name", category->name());
                                               writer << Json::propertySafeName("parentId", setParentId);
                                           });
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionCategory::changeDiscussionCategoryName(const IdType& id, StringView newName, 
                                                                            OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id )
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    auto config = getGlobalConfig();
    auto validationCode = validateString(newName, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                         config->discussionCategory.minNameLength,
                                         config->discussionCategory.maxNameLength,
                                         &MemoryRepositoryBase::doesNotContainLeadingOrTrailingWhitespace);
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;
    AuthorizationStatus authorizationStatus;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.categories().get<EntityCollection::DiscussionCategoryCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           auto newNameString = toString(newName);

                           auto& indexByName = collection.categories().get<EntityCollection::DiscussionCategoryCollectionByName>();
                           if (indexByName.find(newNameString) != indexByName.end())
                           {
                               status = StatusCode::ALREADY_EXISTS;
                               return;
                           }
                                  
                           authorizationStatus = authorization_->changeDiscussionCategoryName(*currentUser, **it, newName);
                           if (AuthorizationStatus::OK != authorizationStatus)
                           {
                               status = authorizationStatus;
                               return;
                           }

                           collection.modifyDiscussionCategory(it, [&newNameString, &currentUser](DiscussionCategory& category)
                                                                   {
                                                                       category.name() = std::move(newNameString);
                                                                       updateLastUpdated(category, currentUser);
                                                                   });
                           writeEvents().onChangeDiscussionCategory(createObserverContext(*currentUser), **it,
                                                                    DiscussionCategory::ChangeType::Name);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionCategory::changeDiscussionCategoryDescription(const IdType& id,
                                                                                   StringView newDescription,
                                                                                   OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id )
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    auto maxLength = static_cast<int_fast32_t>(getGlobalConfig()->discussionCategory.maxDescriptionLength);
    if (countUTF8Characters(newDescription) > maxLength)
    {
        return status = StatusCode::VALUE_TOO_LONG;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;
    AuthorizationStatus authorizationStatus;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.categories().get<EntityCollection::DiscussionCategoryCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           authorizationStatus = authorization_->changeDiscussionCategoryDescription(*currentUser, **it,
                                                                                                     newDescription);
                           if (AuthorizationStatus::OK != authorizationStatus)
                           {
                               status = authorizationStatus;
                               return;
                           }

                           collection.modifyDiscussionCategory(it, [&newDescription](DiscussionCategory& category)
                                                                   {
                                                                       category.description() = toString(newDescription);
                                                                   });

                           writeEvents().onChangeDiscussionCategory(createObserverContext(*currentUser),
                                                                    **it, DiscussionCategory::ChangeType::Description);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionCategory::changeDiscussionCategoryParent(const IdType& id, const IdType& newParentId, 
                                                                              OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if (( ! id) || (id == newParentId))
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;
    AuthorizationStatus authorizationStatus;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.categories().get<EntityCollection::DiscussionCategoryCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                 
                           auto& threadRef = *it;
                           auto& thread = **it;
                           auto newParentIt = indexById.find(newParentId);
                           DiscussionCategoryRef newParentRef; //might be empty
                 
                           if (newParentIt != indexById.end())
                           {
                               newParentRef = *newParentIt;
                               //check that the new parent is not a child of the current category
                               if (newParentRef->hasAncestor(threadRef))
                               {
                                   status = StatusCode::CIRCULAR_REFERENCE_NOT_ALLOWED;
                                   return;
                               }
                           }

                           IdType currentParentId;

                           if (auto currentParent = thread.parentWeak().lock())
                           {
                               if (currentParent->id() == newParentId)
                               {
                                   status = StatusCode::NO_EFFECT;
                                   return;
                               }
                               currentParentId = currentParent->id();
                           }
                           
                           authorizationStatus = authorization_->changeDiscussionCategoryParent(*currentUser, **it, newParentRef);
                           if (AuthorizationStatus::OK != authorizationStatus)
                           {
                               status = authorizationStatus;
                               return;
                           }

                           if (currentParentId)
                           {
                               collection.modifyDiscussionCategoryById(currentParentId,
                                   [&threadRef](DiscussionCategory& parent)
                               {
                                   //remove the current category from it's parent child list
                                   parent.removeChild(threadRef);
                               });
                           }
                           
                           collection.modifyDiscussionCategory(it, [&newParentRef, &currentUser](DiscussionCategory& category)
                                                                   {
                                                                       category.parentWeak() = newParentRef;
                                                                       updateLastUpdated(category, currentUser);
                                                                   });
                 
                           //changing a parent requires updating totals
                           //until there's a visible performance penalty, simply update all totals
                           for (auto& category : collection.categories())
                           {
                               category->resetTotals();
                           }
                           for (auto& category : collection.categories())
                           {
                               category->recalculateTotals();
                           }

                           writeEvents().onChangeDiscussionCategory(createObserverContext(*currentUser), thread,
                                                                    DiscussionCategory::ChangeType::Parent);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionCategory::changeDiscussionCategoryDisplayOrder(const IdType& id, 
                                                                                    int_fast16_t newDisplayOrder,
                                                                                    OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id )
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    if (newDisplayOrder < 0)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;
    AuthorizationStatus authorizationStatus;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.categories().get<EntityCollection::DiscussionCategoryCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                 
                           authorizationStatus = authorization_->changeDiscussionCategoryDisplayOrder(*currentUser, **it, 
                                                                                                      newDisplayOrder);
                           if (AuthorizationStatus::OK != authorizationStatus)
                           {
                               status = authorizationStatus;
                               return;
                           }

                           collection.modifyDiscussionCategory(it, 
                                                               [&newDisplayOrder, &currentUser](DiscussionCategory& category)
                                                               {
                                                                   category.displayOrder() = newDisplayOrder;
                                                                   updateLastUpdated(category, currentUser);
                                                               });

                           writeEvents().onChangeDiscussionCategory(createObserverContext(*currentUser), **it,
                                                                    DiscussionCategory::ChangeType::DisplayOrder);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionCategory::deleteDiscussionCategory(const IdType& id, OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;
    AuthorizationStatus authorizationStatus;

    collection().write([&](EntityCollection& collection)
                       {
                           auto currentUser = performedBy.getAndUpdate(collection);

                           auto& indexById = collection.categories().get<EntityCollection::DiscussionCategoryCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }

                           authorizationStatus = authorization_->deleteDiscussionCategory(*currentUser, **it);
                           if (AuthorizationStatus::OK != authorizationStatus)
                           {
                               status = authorizationStatus;
                               return;
                           }

                           //make sure the category is not deleted before being passed to the observers
                           writeEvents().onDeleteDiscussionCategory(createObserverContext(*currentUser), **it);

                           collection.deleteDiscussionCategory(it);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionCategory::addDiscussionTagToCategory(const IdType& tagId, const IdType& categoryId,
                                                                          OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! tagId || ! categoryId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;
    AuthorizationStatus authorizationStatus;

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
                 
                           auto& categoryIndexById = collection.categories().get<EntityCollection::DiscussionCategoryCollectionById>();
                           auto categoryIt = categoryIndexById.find(categoryId);
                           if (categoryIt == categoryIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           
                           auto& tagRef = *tagIt;
                           auto& tag = **tagIt;
                           auto& categoryRef = *categoryIt;
                           auto& category = **categoryIt;
                 
                           authorizationStatus = authorization_->addDiscussionTagToCategory(*currentUser, tag, category);
                           if (AuthorizationStatus::OK != authorizationStatus)
                           {
                               status = authorizationStatus;
                               return;
                           }

                           //the number of categories associated to a tag is smaller than 
                           //the number of tags associated to a category, so search the category in the tag
                           if ( ! tag.addCategory(categoryRef))
                           {
                               //actually already added, but return ok
                               status = StatusCode::OK;
                               return;
                           }
                                  
                           collection.modifyDiscussionCategory(categoryIt, [&tagRef, &currentUser](auto& category)
                                                                           {
                                                                               category.addTag(tagRef);
                                                                               updateLastUpdated(category, currentUser);
                                                                           });

                           writeEvents().onAddDiscussionTagToCategory(createObserverContext(*currentUser), 
                                                                      tag, category);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionCategory::removeDiscussionTagFromCategory(const IdType& tagId, 
                                                                               const IdType& categoryId, 
                                                                               OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! tagId || ! categoryId)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;
    AuthorizationStatus authorizationStatus;

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
                 
                           auto& categoryIndexById = collection.categories().get<EntityCollection::DiscussionCategoryCollectionById>();
                           auto categoryIt = categoryIndexById.find(categoryId);
                           if (categoryIt == categoryIndexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                 
                           auto& tagRef = *tagIt;
                           auto& tag = **tagIt;
                           auto& categoryRef = *categoryIt;
                           auto& category = **categoryIt;
                           
                           authorizationStatus = authorization_->removeDiscussionTagFromCategory(*currentUser, tag, category);
                           if (AuthorizationStatus::OK != authorizationStatus)
                           {
                               status = authorizationStatus;
                               return;
                           }

                           //the number of categories associated to a tag is smaller than 
                           //the number of tags associated to a category, so search the category in the tag
                           if ( ! tag.removeCategory(categoryRef))
                           {
                               status = StatusCode::NO_EFFECT;
                               return;
                           }
                                 
                           collection.modifyDiscussionCategory(categoryIt, [&tagRef, &currentUser](auto& category)
                                                                           {
                                                                               category.removeTag(tagRef);
                                                                               updateLastUpdated(category, currentUser);
                                                                           });
                           writeEvents().onRemoveDiscussionTagFromCategory(createObserverContext(*currentUser), 
                                                                           tag, category);
                       });
    return status;
}
