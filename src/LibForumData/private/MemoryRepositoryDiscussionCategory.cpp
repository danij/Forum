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

MemoryRepositoryDiscussionCategory::MemoryRepositoryDiscussionCategory(MemoryStoreRef store)
    : MemoryRepositoryBase(std::move(store)),
    validDiscussionCategoryNameRegex(boost::make_u32regex("^[^[:space:]]+.*[^[:space:]]+$"))
{
}

StatusCode MemoryRepositoryDiscussionCategory::getDiscussionCategories(OutStream& output,
                                                                       RetrieveDiscussionCategoriesBy by) const
{
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().read([&](const EntityCollection& collection)
    {
        auto& currentUser = performedBy.get(collection, store());

        if (Context::getDisplayContext().sortOrder == Context::SortOrder::Ascending)
        {
            switch (by)
            {
            case RetrieveDiscussionCategoriesBy::Name: 
                writeSingleValueSafeName(output, "categories", Json::enumerate(collection.categoriesByName().begin(), 
                                                                               collection.categoriesByName().end()));
                break;
            case RetrieveDiscussionCategoriesBy::MessageCount: 
                writeSingleValueSafeName(output, "categories", Json::enumerate(collection.categoriesByMessageCount().begin(), 
                                                                               collection.categoriesByMessageCount().end()));
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
                break;
            case RetrieveDiscussionCategoriesBy::MessageCount: 
                writeSingleValueSafeName(output, "categories", Json::enumerate(collection.categoriesByMessageCount().rbegin(), 
                                                                               collection.categoriesByMessageCount().rend()));
                break;
            }
        }

        if ( ! Context::skipObservers()) readEvents().onGetDiscussionCategories(createObserverContext(currentUser));
    });
    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionCategory::getDiscussionCategoriesFromRoot(OutStream& output) const
{
    PerformedByWithLastSeenUpdateGuard performedBy;
    collection().read([&](const EntityCollection& collection)
                      {
                          auto& currentUser = performedBy.get(collection, store());
                          auto index = collection.categoriesByDisplayOrderRootPriority();
                          auto indexBegin = index.begin();
                          auto indexEnd = index.end();

                          auto indexRootEnd = indexBegin;
                          while ((indexRootEnd != indexEnd) && ((*indexRootEnd)->isRootCategory()))
                          {
                              ++indexRootEnd;
                          }
                          
                          BoolTemporaryChanger _(serializationSettings.showDiscussionCategoryChildren, true);

                          writeSingleValueSafeName(output, "categories", Json::enumerate(indexBegin, indexRootEnd));

                          if ( ! Context::skipObservers())
                              readEvents().onGetRootDiscussionCategories(createObserverContext(currentUser));
                      });
    return StatusCode::OK;
}

StatusCode MemoryRepositoryDiscussionCategory::getDiscussionCategoryById(const IdType& id, OutStream& output) const
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id)
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

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
                          else
                          {
                              status.disable();
                              BoolTemporaryChanger _(serializationSettings.showDiscussionCategoryChildren, true);
                              writeSingleValueSafeName(output, "category", **it);
                          }

                          if ( ! Context::skipObservers())
                              readEvents().onGetDiscussionCategory(createObserverContext(currentUser), **it);
                      });
    return status;
}


StatusCode MemoryRepositoryDiscussionCategory::addNewDiscussionCategory(const std::string& name, const IdType& parentId,
                                                                        OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);

    auto config = getGlobalConfig();
    auto validationCode = validateString(name, validDiscussionCategoryNameRegex, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                         config->discussionCategory.minNameLength, 
                                         config->discussionCategory.maxNameLength);
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }

    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           const auto& createdBy = performedBy.getAndUpdate(collection);
                           
                           auto& indexByName = collection.categories().get<EntityCollection::DiscussionCategoryCollectionByName>();
                           if (indexByName.find(name) != indexByName.end())
                           {
                               status = StatusCode::ALREADY_EXISTS;
                               return;
                           }
                 
                           auto category = std::make_shared<DiscussionCategory>();
                           category->notifyChange() = collection.notifyCategoryChange();
                           category->id() = generateUUIDString();
                           category->name() = name;
                           updateCreated(*category);
                 
                           auto setParentId = IdType::empty;
                           if (parentId)
                           {
                               auto& indexById = collection.categories().get<EntityCollection::DiscussionCategoryCollectionById>();
                               auto it = indexById.find(parentId);
                               if (it != indexById.end())
                               {
                                   collection.modifyDiscussionCategory(it, [&category](DiscussionCategory& parent)
                                   {
                                       parent.addChild(category);
                                   });
                                   category->parentWeak() = *it;
                                   setParentId = parentId;
                               }
                           }
                 
                           collection.categories().insert(category);

                           if ( ! Context::skipObservers())
                               writeEvents().onAddNewDiscussionCategory(createObserverContext(*createdBy), *category);
                 
                           status.addExtraSafeName("id", category->id());
                           status.addExtraSafeName("name", category->name());
                           status.addExtraSafeName("parentId", setParentId);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionCategory::changeDiscussionCategoryName(const IdType& id, const std::string& newName, 
                                                                            OutStream& output)
{
    StatusWriter status(output, StatusCode::OK);
    if ( ! id )
    {
        return status = StatusCode::INVALID_PARAMETERS;
    }

    auto config = getGlobalConfig();
    auto validationCode = validateString(newName, validDiscussionCategoryNameRegex, INVALID_PARAMETERS_FOR_EMPTY_STRING,
                                         config->discussionCategory.minNameLength,
                                         config->discussionCategory.maxNameLength);
    if (validationCode != StatusCode::OK)
    {
        return status = validationCode;
    }
    PerformedByWithLastSeenUpdateGuard performedBy;

    collection().write([&](EntityCollection& collection)
                       {
                           auto& indexById = collection.categories().get<EntityCollection::DiscussionCategoryCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           auto& indexByName = collection.categories().get<EntityCollection::DiscussionCategoryCollectionByName>();
                           if (indexByName.find(newName) != indexByName.end())
                           {
                               status = StatusCode::ALREADY_EXISTS;
                               return;
                           }
                 
                           auto user = performedBy.getAndUpdate(collection);
                 
                           collection.modifyDiscussionCategory(it, [&newName, &user](DiscussionCategory& category)
                                                                   {
                                                                       category.name() = newName;
                                                                       updateLastUpdated(category, user);
                                                                   });
                           if ( ! Context::skipObservers())
                               writeEvents().onChangeDiscussionCategory(createObserverContext(*user), **it,
                                                                        DiscussionCategory::ChangeType::Name);
                       });
    return status;
}

StatusCode MemoryRepositoryDiscussionCategory::changeDiscussionCategoryDescription(const IdType& id,
                                                                                   const std::string& newDescription,
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

    collection().write([&](EntityCollection& collection)
                       {
                           auto& indexById = collection.categories().get<EntityCollection::DiscussionCategoryCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           collection.modifyDiscussionCategory(it, [&newDescription](DiscussionCategory& category)
                                                                   {
                                                                       category.description() = newDescription;
                                                                   });

                           if ( ! Context::skipObservers())
                               writeEvents().onChangeDiscussionCategory(createObserverContext(*performedBy.getAndUpdate(collection)),
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

    collection().write([&](EntityCollection& collection)
                       {
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
                           if (auto currentParent = thread.parentWeak().lock())
                           {
                               if (currentParent->id() == newParentId)
                               {
                                   status = StatusCode::NO_EFFECT;
                                   return;
                               }
                               collection.modifyDiscussionCategoryById(currentParent->id(), 
                                   [&threadRef](DiscussionCategory& parent)
                                   {
                                       //remove the current category from it's parent child list
                                       parent.removeChild(threadRef);
                                   });
                           }
                           
                           auto user = performedBy.getAndUpdate(collection);
                           
                           collection.modifyDiscussionCategory(it, [&newParentRef, &user](DiscussionCategory& category)
                                                                   {
                                                                       category.parentWeak() = newParentRef;
                                                                       updateLastUpdated(category, user);
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

                           if ( ! Context::skipObservers())
                               writeEvents().onChangeDiscussionCategory(createObserverContext(*user), thread,
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

    collection().write([&](EntityCollection& collection)
                       {
                           auto& indexById = collection.categories().get<EntityCollection::DiscussionCategoryCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                 
                           auto user = performedBy.getAndUpdate(collection);
                 
                           collection.modifyDiscussionCategory(it, [&newDisplayOrder, &user](DiscussionCategory& category)
                           {
                               category.displayOrder() = newDisplayOrder;
                               updateLastUpdated(category, user);
                           });

                           if ( ! Context::skipObservers())
                               writeEvents().onChangeDiscussionCategory(createObserverContext(*user), **it,
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

    collection().write([&](EntityCollection& collection)
                       {
                           auto& indexById = collection.categories().get<EntityCollection::DiscussionCategoryCollectionById>();
                           auto it = indexById.find(id);
                           if (it == indexById.end())
                           {
                               status = StatusCode::NOT_FOUND;
                               return;
                           }
                           //make sure the category is not deleted before being passed to the observers
                           if ( ! Context::skipObservers())
                               writeEvents().onDeleteDiscussionCategory(
                                       createObserverContext(*performedBy.getAndUpdate(collection)), **it);

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

    collection().write([&](EntityCollection& collection)
                       {
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
                 
                           //the number of categories associated to a tag is smaller than 
                           //the number of tags associated to a category, so search the category in the tag
                           if ( ! tag.addCategory(categoryRef))
                           {
                               //actually already added, but return ok
                               status = StatusCode::OK;
                               return;
                           }
                 
                           auto user = performedBy.getAndUpdate(collection);
                 
                           collection.modifyDiscussionCategory(categoryIt, [&tagRef, &user](auto& category)
                                                                           {
                                                                               category.addTag(tagRef);
                                                                               updateLastUpdated(category, user);
                                                                           });

                           if ( ! Context::skipObservers())
                               writeEvents().onAddDiscussionTagToCategory(createObserverContext(*user), tag, category);
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

    collection().write([&](EntityCollection& collection)
                       {
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
                           
                           //the number of categories associated to a tag is smaller than 
                           //the number of tags associated to a category, so search the category in the tag
                           if ( ! tag.removeCategory(categoryRef))
                           {
                               status = StatusCode::NO_EFFECT;
                               return;
                           }
                 
                           auto user = performedBy.getAndUpdate(collection);
                 
                           collection.modifyDiscussionCategory(categoryIt, [&tagRef, &user](auto& category)
                                                                           {
                                                                               category.removeTag(tagRef);
                                                                               updateLastUpdated(category, user);
                                                                           });
                           if ( ! Context::skipObservers())
                               writeEvents().onRemoveDiscussionTagFromCategory(createObserverContext(*user), tag, category);
                       });
    return status;
}
